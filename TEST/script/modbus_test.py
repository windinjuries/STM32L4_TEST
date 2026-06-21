#!/usr/bin/env python3
"""
Modbus RTU 读写速度与成功率测试脚本
===================================
测试对象：STM32L4 Modbus RTU Slave (USART3, 115200 8N1)
从站地址: 1

依赖安装：
    pip install pymodbus pyserial

用法：
    python modbus_test.py                  # 自动扫描常见串口
    python modbus_test.py COM3             # 指定串口
    python modbus_test.py COM3 --baud 9600 # 指定波特率
"""

import argparse
import sys
import time
import csv
import statistics
from datetime import datetime
from dataclasses import dataclass, field, asdict
from typing import Optional

# ---------------------------------------------------------------------------
# 配置参数（与设备端一致）
# ---------------------------------------------------------------------------
SLAVE_ADDR = 1
TIMEOUT_S = 0.5           # 单次操作超时
BURST_COUNT = 100         # 每种测试的重复次数
REG_QUANTITIES = [1, 10, 50, 125]  # 批量读测试的寄存器数量

# ---------------------------------------------------------------------------
# 尝试导入 pymodbus
# ---------------------------------------------------------------------------
try:
    from pymodbus.client import ModbusSerialClient as ModbusClient
    from pymodbus.exceptions import ModbusException, ConnectionException
    HAVE_PYMODBUS = True
    # 自动检测 pymodbus 版本兼容的参数名（slave / unit / device_id）
    import inspect
    _test_instance = ModbusClient(port="dummy")
    _sig = inspect.signature(_test_instance.read_holding_registers)
    for _kw in ("slave", "unit", "device_id"):
        if _kw in _sig.parameters:
            MODBUS_ADDR_KW = _kw
            break
    else:
        MODBUS_ADDR_KW = "slave"  # fallback
    del _test_instance, _sig, _kw
except ImportError:
    HAVE_PYMODBUS = False
    try:
        import serial
        HAVE_SERIAL = True
    except ImportError:
        HAVE_SERIAL = False


# ---------------------------------------------------------------------------
# 数据结构
# ---------------------------------------------------------------------------
@dataclass
class TestResult:
    """单次测试结果"""
    operation: str          # 操作名称
    quantity: int           # 寄存器数量
    success: bool           # 是否成功
    elapsed_ms: float       # 耗时 (ms)
    error_msg: str = ""     # 错误信息


@dataclass
class TestSummary:
    """测试汇总"""
    operation: str
    quantity: int
    total: int
    success: int
    failed: int
    success_rate: float
    avg_ms: float
    min_ms: float
    max_ms: float
    median_ms: float
    stdev_ms: float
    ops_per_sec: float
    regs_per_sec: float
    top_errors: list = field(default_factory=list)  # (错误信息, 出现次数)


# ---------------------------------------------------------------------------
# Modbus 客户端封装
# ---------------------------------------------------------------------------
class ModbusTester:
    """Modbus RTU 测试器 —— 支持 pymodbus 和 raw-serial 两种后端"""

    def __init__(self, port: str, baud: int = 115200, timeout: float = TIMEOUT_S):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.client = None

    # -------- pymodbus 后端 ------------------------------------------------

    def _connect_pymodbus(self) -> str:
        try:
            self.client = ModbusClient(
                port=self.port,
                baudrate=self.baud,
                bytesize=8,
                parity="N",
                stopbits=1,
                timeout=self.timeout,
            )
            ok = self.client.connect()
            if ok:
                self.client_ver = "pymodbus"
                return ""
            return "连接失败"
        except Exception as e:
            return str(e)

    def _read_holding_regs_py(self, addr: int, count: int) -> tuple[bool, Optional[list], str]:
        try:
            resp = self.client.read_holding_registers(addr, count=count, **{MODBUS_ADDR_KW: SLAVE_ADDR})
            if resp.isError():
                return False, None, str(resp)
            return True, resp.registers, ""
        except Exception as e:
            return False, None, str(e)

    def _read_input_regs_py(self, addr: int, count: int) -> tuple[bool, Optional[list], str]:
        try:
            resp = self.client.read_input_registers(addr, count=count, **{MODBUS_ADDR_KW: SLAVE_ADDR})
            if resp.isError():
                return False, None, str(resp)
            return True, resp.registers, ""
        except Exception as e:
            return False, None, str(e)

    def _write_single_reg_py(self, addr: int, value: int) -> tuple[bool, str]:
        try:
            resp = self.client.write_register(addr, value, **{MODBUS_ADDR_KW: SLAVE_ADDR})
            if resp.isError():
                return False, str(resp)
            return True, ""
        except Exception as e:
            return False, str(e)

    def _write_multiple_regs_py(self, addr: int, values: list) -> tuple[bool, str]:
        try:
            resp = self.client.write_registers(addr, values, **{MODBUS_ADDR_KW: SLAVE_ADDR})
            if resp.isError():
                return False, str(resp)
            return True, ""
        except Exception as e:
            return False, str(e)

    # -------- 统一 API ----------------------------------------------------

    def connect(self) -> Optional[str]:
        """连接设备，返回 None 成功，str 失败原因"""
        if HAVE_PYMODBUS:
            return self._connect_pymodbus()
        return "未找到可用的 Modbus 库 (pip install pymodbus)"

    def disconnect(self):
        if self.client and hasattr(self.client, "close"):
            self.client.close()

    def read_holding_registers(self, addr: int, count: int) -> tuple[bool, Optional[list], str]:
        return self._read_holding_regs_py(addr, count)

    def read_input_registers(self, addr: int, count: int) -> tuple[bool, Optional[list], str]:
        return self._read_input_regs_py(addr, count)

    def write_single_register(self, addr: int, value: int) -> tuple[bool, str]:
        return self._write_single_reg_py(addr, value)

    def write_multiple_registers(self, addr: int, values: list) -> tuple[bool, str]:
        return self._write_multiple_regs_py(addr, values)

    # -------- 测试方法 ----------------------------------------------------

    def _run_test(self, operation: str, quantity: int, func, count: int = BURST_COUNT) -> list[TestResult]:
        results: list[TestResult] = []

        for i in range(count):
            t0 = time.perf_counter()
            result = func(i)
            # 兼容 2 值 (success, err) 或 3 值 (success, data, err) 返回
            if isinstance(result, tuple) and len(result) >= 2:
                success, err = result[0], result[-1]
            else:
                success, err = bool(result), ""
            elapsed = (time.perf_counter() - t0) * 1000  # ms

            results.append(TestResult(
                operation=operation,
                quantity=quantity,
                success=success,
                elapsed_ms=round(elapsed, 2),
                error_msg=err if not success else "",
            ))

            # 进度显示
            if (i + 1) % 25 == 0 or i == count - 1:
                ok = sum(1 for r in results if r.success)
                print(f"\r  [{i+1}/{count}] 成功 {ok} 失败 {count-ok}", end="", flush=True)

        print()
        return results

    def test_read_holding(self, addr: int = 0, count: int = BURST_COUNT) -> list[TestResult]:
        print(f"  >> 读保持寄存器 (地址={addr}, 每次 1 个寄存器, {count} 次)")
        return self._run_test("读保持寄存器", 1,
                              lambda i: self.read_holding_registers(addr, 1),
                              count=count)

    def test_write_single(self, addr: int = 0, count: int = BURST_COUNT) -> list[TestResult]:
        print(f"  >> 写单个寄存器 (地址={addr}, {count} 次)")
        test_val = 0xAAAA
        return self._run_test("写单个寄存器", 1,
                              lambda i: self.write_single_register(addr, (test_val + i) & 0xFFFF),
                              count=count)

    def test_burst_read(self, quantity: int, addr: int = 0, count: int = BURST_COUNT) -> list[TestResult]:
        print(f"  >> 批量读保持寄存器 (地址={addr}, {quantity}个/次, {count} 次)")
        return self._run_test("批量读保持寄存器", quantity,
                              lambda i: self.read_holding_registers(addr, quantity),
                              count=count)

    def test_burst_write(self, quantity: int, addr: int = 0, count: int = BURST_COUNT) -> list[TestResult]:
        print(f"  >> 批量写保持寄存器 (地址={addr}, {quantity}个/次, {count} 次)")
        values = [0x1234 + i for i in range(quantity)]
        return self._run_test("批量写保持寄存器", quantity,
                              lambda i: self.write_multiple_registers(addr, values),
                              count=count)

    def test_read_input(self, addr: int = 0, count: int = BURST_COUNT) -> list[TestResult]:
        print(f"  >> 读输入寄存器 (地址={addr}, 每次 1 个, {count} 次)")
        return self._run_test("读输入寄存器", 1,
                              lambda i: self.read_input_registers(addr, 1),
                              count=count)

    def test_write_then_read(self, addr: int = 0, count: int = BURST_COUNT) -> list[TestResult]:
        """写后即读验证 —— 写一个值，立即读回确认"""
        print(f"  >> 写后读验证 (地址={addr}, {count} 次)")
        results: list[TestResult] = []
        test_val = 0x1234

        for i in range(count):
            t0 = time.perf_counter()
            val = (test_val + i) & 0xFFFF

            # 写
            ok_w, err_w = self.write_single_register(addr, val)
            if not ok_w:
                elapsed = (time.perf_counter() - t0) * 1000
                results.append(TestResult("写后读验证", 1, False, round(elapsed, 2), f"写失败: {err_w}"))
                continue

            # 读
            ok_r, data, err_r = self.read_holding_registers(addr, 1)
            elapsed = (time.perf_counter() - t0) * 1000

            if not ok_r:
                results.append(TestResult("写后读验证", 1, False, round(elapsed, 2), f"读失败: {err_r}"))
            elif data and data[0] != val:
                results.append(TestResult("写后读验证", 1, False, round(elapsed, 2),
                                          f"数据不匹配: 写入 {val:#06x} 读到 {data[0]:#06x}"))
            else:
                results.append(TestResult("写后读验证", 1, True, round(elapsed, 2)))

            if (i + 1) % 25 == 0 or i == count - 1:
                ok = sum(1 for r in results if r.success)
                print(f"\r  [{i+1}/{count}] 成功 {ok} 失败 {count-ok}", end="", flush=True)

        print()
        return results


# ---------------------------------------------------------------------------
# 统计分析
# ---------------------------------------------------------------------------
def summarize(results: list[TestResult]) -> TestSummary:
    """对单组测试结果做统计分析"""
    successes = [r for r in results if r.success]
    failed = [r for r in results if not r.success]

    elapsed_list = [r.elapsed_ms for r in successes]
    qty = results[0].quantity if results else 0

    if elapsed_list:
        avg_ms = statistics.mean(elapsed_list)
        min_ms = min(elapsed_list)
        max_ms = max(elapsed_list)
        median_ms = statistics.median(elapsed_list)
        stdev_ms = statistics.stdev(elapsed_list) if len(elapsed_list) > 1 else 0.0
    else:
        avg_ms = min_ms = max_ms = median_ms = stdev_ms = 0.0

    total = len(results)
    ok = len(successes)
    rate = (ok / total * 100) if total > 0 else 0.0

    # 统计错误信息分布
    from collections import Counter
    err_counter = Counter(r.error_msg for r in failed if r.error_msg)
    top_errors = err_counter.most_common(5)  # [(msg, count), ...]

    # 每秒操作数、寄存器数
    avg_s = avg_ms / 1000 if avg_ms > 0 else 0
    ops_per_sec = 1.0 / avg_s if avg_s > 0 else 0.0
    regs_per_sec = ops_per_sec * qty

    return TestSummary(
        operation=results[0].operation if results else "",
        quantity=qty,
        total=total,
        success=ok,
        failed=total - ok,
        success_rate=round(rate, 2),
        avg_ms=round(avg_ms, 2),
        min_ms=round(min_ms, 2),
        max_ms=round(max_ms, 2),
        median_ms=round(median_ms, 2),
        stdev_ms=round(stdev_ms, 2),
        ops_per_sec=round(ops_per_sec, 1),
        regs_per_sec=round(regs_per_sec, 1),
        top_errors=top_errors,
    )


def print_summary(summary: TestSummary):
    """打印单组汇总"""
    print(f"  {'='*55}")
    print(f"  操作: {summary.operation}")
    if summary.quantity > 1:
        print(f"  批量: {summary.quantity} 个寄存器/次")
    print(f"  总次数: {summary.total} | 成功: {summary.success} | 失败: {summary.failed}")
    print(f"  成功率: {summary.success_rate}%")

    color = ""
    reset = ""
    if summary.success_rate < 100:
        color = "\033[91m"    # 红色
        reset = "\033[0m"

    print(f"  延迟统计 (仅成功):")
    print(f"    平均: {color}{summary.avg_ms:>8.2f} ms{reset}")
    print(f"    最小: {summary.min_ms:>8.2f} ms")
    print(f"    最大: {color}{summary.max_ms:>8.2f} ms{reset}")
    print(f"    中位数: {summary.median_ms:>8.2f} ms")
    print(f"    标准差: {summary.stdev_ms:>8.2f} ms")
    if summary.failed > 0 and summary.top_errors:
        print(f"  错误分布 (Top {len(summary.top_errors)}):")
        for err_msg, cnt in summary.top_errors:
            # 截断过长错误信息
            display = err_msg[:80] + "..." if len(err_msg) > 80 else err_msg
            print(f"    [{cnt}x] {display}")
    print(f"  吞吐量:")
    print(f"    {summary.ops_per_sec:>8.1f} 操作/秒")
    if summary.quantity > 1:
        print(f"    {summary.regs_per_sec:>8.1f} 寄存器/秒")
    print(f"  {'='*55}")


# ---------------------------------------------------------------------------
# CSV 导出
# ---------------------------------------------------------------------------
def export_csv(all_results: list[list[TestResult]], filename: str):
    """将详细结果导出为 CSV"""
    with open(filename, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["时间戳", "操作", "寄存器数量", "成功", "耗时(ms)", "错误信息"])
        for group in all_results:
            for r in group:
                w.writerow([datetime.now().isoformat(), r.operation,
                            r.quantity, r.success, r.elapsed_ms, r.error_msg])
    print(f"\n  详细结果已保存: {filename}")


# ---------------------------------------------------------------------------
# 串口自动扫描
# ---------------------------------------------------------------------------
def scan_ports() -> list[str]:
    """尝试列举可用串口"""
    ports = []
    try:
        import serial.tools.list_ports
        ports = [p.device for p in serial.tools.list_ports.comports()]
    except ImportError:
        # fallback: 常见串口名
        import glob
        if sys.platform.startswith("win"):
            for i in range(1, 33):
                ports.append(f"COM{i}")
        else:
            ports = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyS*")
    return ports


# ---------------------------------------------------------------------------
# 主菜单
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Modbus RTU 读写速度与成功率测试",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="示例:\n  %(prog)s COM3\n  %(prog)s COM3 --baud 9600 --count 50",
    )
    parser.add_argument("port", nargs="?", default=None, help="串口名 (如 COM3, /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="波特率 (默认 115200)")
    parser.add_argument("--count", type=int, default=BURST_COUNT, help="每种测试重复次数 (默认 100)")
    parser.add_argument("--csv", default=None, help="导出详细结果到 CSV 文件")
    parser.add_argument("--fast", action="store_true", help="快速模式：仅测试单次读/写")
    args = parser.parse_args()

    # ---- 确定串口 ----
    port = args.port
    if not port:
        print("[扫描] 正在扫描串口...")
        ports = scan_ports()
        if not ports:
            print("[失败] 未发现串口设备。请指定串口名，例如:")
            if sys.platform.startswith("win"):
                print("   python modbus_test.py COM3")
            else:
                print("   python modbus_test.py /dev/ttyUSB0")
            sys.exit(1)

        print(f"   发现串口: {', '.join(ports)}")
        # 取第一个
        port = ports[0]
        print(f"   自动选择: {port}")
    else:
        print(f"   使用指定串口: {port}")

    # ---- 库检查 ----
    if not HAVE_PYMODBUS:
        print("\n[失败] 请安装 pymodbus: pip install pymodbus")
        sys.exit(1)

    # ---- 连接 ----
    print(f"\n[链接] 正在连接 {port} (波特率 {args.baud})...")
    tester = ModbusTester(port, args.baud)
    err = tester.connect()
    if err:
        print(f"[失败] 连接失败: {err}")
        print("   请检查:")
        print("   - 串口是否正确")
        print("   - 设备是否已上电")
        print("   - 波特率是否匹配")
        print(f"   - 其他程序是否占用了 {port}")
        sys.exit(1)
    print("[完成] 连接成功！\n")

    # ---- 测试计划 ----
    count = args.count
    all_results: list[list[TestResult]] = []
    summaries: list[TestSummary] = []

    print(f"{'-- Modbus RTU 性能测试 --':^60}")
    print(f"{'-'*60}")
    print(f"  串口:      {port}")
    print(f"  波特率:    {args.baud}")
    print(f"  从站地址:  {SLAVE_ADDR}")
    print(f"  超时:      {TIMEOUT_S} s")
    print(f"  重复次数:  {count}")
    if args.fast:
        print(f"  模式:      快速测试")
    print(f"{'-'*60}\n")

    try:
        # 1. 读保持寄存器（单次）
        print("[1/4] 读保持寄存器（单次）")
        r = tester.test_read_holding(count=min(count, 20 if args.fast else count))
        all_results.append(r)
        summaries.append(summarize(r))
        print_summary(summaries[-1])

        # 2. 写单个寄存器
        print(f"\n[2/4] 写单个寄存器")
        r = tester.test_write_single(count=min(count, 20 if args.fast else count))
        all_results.append(r)
        summaries.append(summarize(r))
        print_summary(summaries[-1])

        # 3. 批量读（多种数量）
        print(f"\n[3/4] 批量读保持寄存器（不同批量大小）")
        burst_results = []
        for qty in REG_QUANTITIES:
            if args.fast and qty > 10:
                continue
            r = tester.test_burst_read(qty, count=min(count, 10 if args.fast else count))
            burst_results.extend(r)
            all_results.append(r)
            summaries.append(summarize(r))
            print_summary(summaries[-1])

        # 4. 写后读验证
        print(f"\n[4/4] 写后读验证（可靠性测试）")
        r = tester.test_write_then_read(count=min(count, 20 if args.fast else count))
        all_results.append(r)
        summaries.append(summarize(r))
        print_summary(summaries[-1])

    except KeyboardInterrupt:
        print("\n\n[警告]  测试被用户中断")
    finally:
        tester.disconnect()

    # ---- 总汇总 ----
    print(f"\n{'== 测试总汇总 ==':^60}")
    print(f"{'-'*60}")
    print(f"{'操作':<24} {'成功率':>8} {'平均(ms)':>10} {'操作/秒':>10}")
    print(f"{'-'*60}")
    for s in summaries:
        op = f"{s.operation}"
        if s.quantity > 1:
            op += f" (x{s.quantity})"
        print(f"{op:<24} {s.success_rate:>7.1f}% {s.avg_ms:>10.2f} {s.ops_per_sec:>10.1f}")
    print(f"{'-'*60}")

    # ---- 导出 ----
    if args.csv:
        export_csv(all_results, args.csv)

    print("\n[完成] 测试完成！")


if __name__ == "__main__":
    main()
