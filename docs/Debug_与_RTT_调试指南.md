# Debug 与 SEGGER RTT 调试指南

本文档说明如何在 STM32L475VET6 + ST-Link V2 开发板上进行调试和查看 RTT 日志。

---

## 1. 环境概览

| 组件 | 详情 |
|------|------|
| **MCU** | STM32L475VET6 (Cortex-M4F, 512 KB Flash, 128 KB RAM) |
| **调试器** | ST-Link V2 (FW: V2J47M34) |
| **IDE** | VS Code + cortex-debug 1.12.1 |
| **构建工具** | CMake 4.2.3 + GCC arm-none-eabi 14.3.1 |
| **RTT 库** | SEGGER RTT (Middlewares/Third_Party/SeggerRTT/) |
| **OpenOCD** | 0.12.0 (手动下载) |
| **pyOCD** | 0.45.0 (pip 安装) |

### UART / 日志通道分配

| 通道 | 物理接口 | 波特率 | 用途 |
|------|----------|--------|------|
| USART1 | PA9 (TX) / PA10 (RX) | 115200 | 日志输出 (debug_log) |
| USART2 | — | 115200 DMA | ESP8266 WiFi |
| USART3 | — | 115200 DMA | Modbus |
| RTT Ch.0 | 调试器读 RAM | — | SEGGER RTT 日志 (与 USART1 同步输出) |

### RAM 布局

| 区域 | 地址 | 大小 |
|------|------|------|
| SRAM1 | `0x20000000` | 96 KB |
| SRAM2 | `0x10000000` | 32 KB |

> RTT 控制块位于 SRAM1 中，搜索范围 `0x20000000` ~ `0x20018000`。

---

## 2. VS Code 调试配置

配置文件: [.vscode/launch.json](../.vscode/launch.json)

### 2.1 Debug with OpenOCD（推荐日常调试）

- **RTT**: 已禁用
- **特点**: 最稳定，开箱即用
- **注意**: ST-Link V2 **不支持** OpenOCD 的 `monitor rtt start` 命令，会报 `Protocol error with Rcmd: 01`

> 固件中 `SEGGER_RTT_Init()` 仍正常执行（`NO_BLOCK_SKIP` 模式，缓冲区满时丢弃数据不阻塞），日志同时通过 USART1 输出。

### 2.2 Debug with pyOCD + RTT (Auto)

- **RTT**: 已启用
- **特点**: cortex-debug 1.12.1 + pyOCD 0.45.0 偶发启动超时，备用
- **工具路径**: `C:/Users/Administrator/.pyenv/pyenv-win/versions/3.10.11/Scripts/pyocd.exe`

### 2.3 Debug with pyOCD + RTT (Manual)

- **RTT**: 已启用
- **类型**: `attach` (手动启动 pyocd gdbserver 后连接 GDB)
- **用法**: 终端中先启动 `pyocd gdbserver --target stm32l475xg --port 50000`，再 F5 启动此配置

### 2.4 STM32Cube: Launch ST-Link GDB Server

- **RTT**: 不支持
- **特点**: STM32Cube IDE 自带调试器，无额外依赖

---

## 3. 查看 RTT 日志

### 3.1 pyOCD 自带 RTT Viewer（推荐）

```bash
# 完整命令（指定 RAM 搜索范围）
C:/Users/Administrator/.pyenv/pyenv-win/versions/3.10.11/Scripts/pyocd.exe rtt \
  --target stm32l475xg \
  -a 0x20000000 \
  -s 0x18000
```

> **必须指定 `-a` 和 `-s`**，否则 pyocd 默认搜索范围找不到 RTT 控制块。

输出示例：

```
0001869 I 3 up channels and 3 down channels found [rtt_cmd]
0001869 I Reading from up channel 0 ("Terminal") [rtt_cmd]

========================================
  Debug Log System Initialized
  Output: USART1 + SEGGER RTT
  [RTT]   Ready - view with J-Link RTT Viewer
========================================
[I] System initialized, starting tasks...
[I] CH395: init chip...
[I] CH395: MAC: 00:01:02:03:04:05
```

按 `Ctrl+C` 退出。

### 3.2 USART1 串口日志

当 ST-Link 被调试器占用时，可通过 USART1 查看日志：

- **引脚**: PA9 (TX) + GND
- **参数**: 115200 baud, 8 data bits, 1 stop bit, no parity
- **工具**: 任意串口助手 (PuTTY, Tera Term, SerialTool 等)

---

## 4. ST-Link 占用冲突

**ST-Link V2 同一时间只能被一个进程占用**。调试器（OpenOCD / pyocd gdbserver）和 RTT Viewer 无法同时使用。

### 典型工作流

| 场景 | 步骤 |
|------|------|
| **只看日志** | 运行 `pyocd rtt` 命令 |
| **调试 + 断点** | 1. 停掉 `pyocd rtt`<br>2. VS Code F5 → "Debug with OpenOCD"<br>3. 日志通过 USART1 串口查看 |
| **ST-Link 不响应** | 拔掉 ST-Link USB 线，等 3 秒，重新插上 |

---

## 5. 常见问题

### 5.1 Protocol error with Rcmd: 01

ST-Link V2 不支持 OpenOCD 的 `monitor rtt start`。已在 "Debug with OpenOCD" 配置中禁用 RTT。

### 5.2 Failed to launch PyOCD GDB Server: Timeout

cortex-debug 1.12.1 与 pyOCD 0.45.0 的兼容性问题，偶发超时。使用 Manual 模式或改用 OpenOCD 配置。

### 5.3 pyocd: No available debug probes

ST-Link 未被检测到：
1. 检查 USB 线连接
2. 拔掉重插 ST-Link USB
3. 确认没有其他进程占用（OpenOCD / ST-Link GDB Server）

### 5.4 Control block not found

RTT 控制块未找到：
1. 确认固件已烧录（含 SEGGER RTT 代码）
2. 确认 `CONFIG_LOG_OUTPUT_RTT = 1` (在 `APP/config.h`)
3. 指定 RAM 搜索范围: `-a 0x20000000 -s 0x18000`

---

## 6. 相关文件

| 文件 | 说明 |
|------|------|
| [.vscode/launch.json](../.vscode/launch.json) | VS Code 调试配置 |
| [APP/config.h](../APP/config.h) | `CONFIG_LOG_OUTPUT_RTT` 开关 |
| [APP/debug_log.c](../APP/debug_log.c) | 日志输出实现 (USART1 + RTT 双通道) |
| [Middlewares/Third_Party/SeggerRTT/](../Middlewares/Third_Party/SeggerRTT/) | SEGGER RTT 库 |
| [CLAUDE.md](../.claude/CLAUDE.md) | 项目构建与架构说明 |
