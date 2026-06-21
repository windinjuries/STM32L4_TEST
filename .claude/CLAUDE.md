# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 编译 / Build

本项目使用 **CMake + GCC arm-none-eabi** 构建（从 Keil MDK-ARM 迁移），当前分支 `feature/cmake_gcc`。

CMake 安装于 STM32Cube 工具链中，不在 PATH 中，需要使用绝对路径调用。

### 工具路径

| 工具 | 路径 |
|------|------|
| CMake | `~/AppData/Local/stm32cube/bundles/cmake/4.2.3+st.1/bin/cmake.exe` |
| GCC | `~/AppData/Local/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin/arm-none-eabi-gcc.exe` |
| GDB | `~/AppData/Local/stm32cube/bundles/gnu-gdb-for-stm32/14.3.1+st.2/bin/arm-none-eabi-gdb.exe` |
| 烧录 | `~/AppData/Local/stm32cube/bundles/programmer/2.22.0+st.1/bin/STM32_Programmer_CLI.exe` |
| GDB Server | `~/AppData/Local/stm32cube/bundles/stlink-gdbserver/7.13.0+st.3/bin/ST-LINK_gdbserver.exe` |

> `~` = 当前用户的目录

### 配置 & 构建

当前已配置好的构建目录为 `build/Debug`（Ninja generator），直接构建即可：

```bash
# 构建（在仓库根目录执行）
~/AppData/Local/stm32cube/bundles/cmake/4.2.3+st.1/bin/cmake.exe --build build/Debug
```

若需重新配置（通常不需要）：

```bash
~/AppData/Local/stm32cube/bundles/cmake/4.2.3+st.1/bin/cmake.exe \
  -B build/Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
```

### 工具链文件
- `cmake/gcc-arm-none-eabi.cmake` — GCC 交叉编译器，Cortex-M4 + FPv4-SP-D16 hard-float ABI。链接：nano.specs + `STM32L475XX_FLASH.ld`。Debug `-O0 -g3`，Release `-Os -g0`。
- `cmake/starm-clang.cmake` — 备选：STARM Clang 工具链。通过 `STARM_TOOLCHAIN_CONFIG` 切换三种配置：`STARM_HYBRID` / `STARM_NEWLIB` / `STARM_PICOLIBC`。

### 构建产物
- ELF: `build/Debug/TEST.elf`
- Map: `build/Debug/TEST.map`（自动输出内存使用摘要）
- `compile_commands.json` 已启用，供 clangd/LSP 使用

---

## 烧录 / Flash

使用 STM32CubeProgrammer + ST-Link（SWD 接口）。

### 检查连接

```bash
~/AppData/Local/stm32cube/bundles/programmer/2.22.0+st.1/bin/STM32_Programmer_CLI.exe \
  -c port=SWD freq=4000
```

### 烧录固件

```bash
~/AppData/Local/stm32cube/bundles/programmer/2.22.0+st.1/bin/STM32_Programmer_CLI.exe \
  -c port=SWD freq=1000 mode=UR reset=HWrst \
  -w build/Debug/TEST.elf -v -rst
```

参数说明：
- `freq=1000` — SWD 频率 1000 KHz（若通信不稳定时用较低频率）
- `mode=UR` — Under Reset 连接模式，更可靠
- `reset=HWrst` — 硬件复位
- `-w` — 写入固件
- `-v` — 写完后校验
- `-rst` — 烧录完成后复位 MCU

> 设备信息：ST-Link V2J47M34，STM32L475xx Rev 4，512 KB Flash

---

## 仿真调试 / Debug

使用 ST-LINK GDB Server + arm-none-eabi-gdb 两步走。

### 终端 1：启动 GDB Server（后台运行）

```bash
~/AppData/Local/stm32cube/bundles/stlink-gdbserver/7.13.0+st.3/bin/ST-LINK_gdbserver.exe \
  -p 61234 -v -d \
  -cp ~/AppData/Local/stm32cube/bundles/programmer/2.22.0+st.1/bin/
```

参数说明：
- `-p 61234` — 监听端口
- `-v` — 详细日志
- `-d` — SWD 模式
- `-cp` — CubeProgrammer bin 路径（供擦写 Flash 用）

### 终端 2：启动 GDB 并连接

```bash
~/AppData/Local/stm32cube/bundles/gnu-gdb-for-stm32/14.3.1+st.2/bin/arm-none-eabi-gdb.exe -q \
  build/Debug/TEST.elf \
  -ex "target remote :61234" \
  -ex "monitor reset halt" \
  -ex "load"
```

## 项目架构 / Project Architecture

### MCU 与内存
- **芯片**: STM32L475VET6（Cortex-M4F, LQFP100, 80 MHz SYSCLK，PLL 时钟源为 MSI 48 MHz）
- **Flash**: 共 512 KB — Bank 1（256 KB, 0x08000000）存放应用程序；Bank 2（256 KB, 0x08040000）通过 `bsp_flash.c` 作用户数据存储
- **RAM**: 共 128 KB — RAM1（96 KB @ 0x20000000）+ RAM2（32 KB @ 0x10000000）
- **链接脚本**: `STM32L475XX_FLASH.ld` — Flash 起始 0x08000000，长度 510 KB（2 KB 保留）。堆 0x200 字节，栈 0x400 字节，支持 TLS 段

### CMake 结构（三层）
1. **`CMakeLists.txt`**（根） — 项目定义、编译器标志，引入 `user.cmake` 和 `cmake/stm32cubemx/`
2. **`cmake/stm32cubemx/CMakeLists.txt`** — STM32CubeMX 生成的 HAL 驱动、CMSIS、FreeRTOS、USB Device Library，编译为 OBJECT 库。定义 `MX_Defines_Syms`（`USE_HAL_DRIVER`、`STM32L475xx`、`DEBUG`）、include 路径，链接 `stm32cubemx` INTERFACE 库
3. **`user.cmake`** — 用户空间源文件和头文件。定义 `USER_SOURCES`（FreeMaster、LVGL 核心+移植+应用、APP 层、BSP）、`USER_INCLUDES`、`USER_COMPILE_DEFINITIONS`

### FreeRTOS 任务布局（CMSIS-RTOS v1 API）

| 任务 | 栈大小 | 优先级 | 功能 |
|------|--------|--------|------|
| `defaultTask` | 128 words | Normal | USB CDC 初始化 + FreeMaster 轮询 + LED 闪烁 |
| `tc214bTask` | 256 words | Normal | 直流电机控制（TC214B） |
| `lvglTask` | 2048 words | Normal | LVGL GUI：每 30ms 调用 `lv_timer_handler()` |
| `wifiTask` | 512 words | Normal | ESP8266 WiFi 操作 |

LVGL 时基由 TIM3 周期回调中调用 `lv_tick_inc(1)` 驱动。
系统时基使用 TIM1 + `HAL_IncTick()` + FreeMaster 记录器。

### 关键中间件

- **LVGL v8.3.11** — GUI 库。配置文件：`Middlewares/Third_Party/LVGL/port/lv_conf.h`（RGB565，16 位色彩交换开启）。显示驱动（`lv_port_disp.c`）使用 SPI3。输入设备（`lv_port_indev.c`）处理 4 按键键盘。应用层在 `Middlewares/Third_Party/LVGL/app/`
- **FreeMaster** — NXP 实时调试监控器。使用 USART1（串口协议）。配置文件：`Middlewares/Third_Party/FreeMaster/src/port/freemaster_cfg.h`。在 `defaultTask` 中轮询
- **USB CDC** — STM32 USB Device Library，虚拟串口

### 应用层（`APP/`）

| 模块 | 文件 | 说明 |
|--------|------|------|
| 应用入口 | `app_main.c` | `app_init()` — 调用 `param_storage_init()` |
| WiFi | `esp8266_wifi.c` | ESP8266 AT 指令通过 UART 通信。NTP 时间同步、HTTP 获取时间、wttr.in 天气查询 |
| 电机控制 | `dc_control.c` | TC214B 电机驱动：正转/停止/反转，PWM 调速 |
| 参数存储 | `storage.c` | Flash Bank 2 参数存储 |

### BSP 层（`BSP/`）

- **`bsp_flash.c`** — 内部 Flash Bank 2 操作：页擦除、双字写入、缓冲区读写。测试区域在最后 4 页（0x0807F000）。Unity 测试入口 `bsp_flash_run_tests()`（当前通过 `#ifdef RUN_FLASH_TESTS` 禁用）

### GUI 架构（LVGL, 240×240 显示屏）

4 个页面，带滑动动画，每页有独立的 `lv_group_t` 用于按键导航：
- **首页** — 3 个菜单项（时间 / 天气 / 系统信息）
- **时间页** — RTC 日期/时间/星期（1 秒刷新），NTP 同步按钮
- **天气页** — 北京 wttr.in 温度，手动刷新按钮，每 10 分钟自动刷新
- **系统信息页** — MCU / RTOS / LVGL / 显示信息

导航机制：切页时 `lv_group_set_default()` + `lv_indev_set_group()`；`lv_group_focus_next()` 初始聚焦。

### 按键输入（4 按键）

| 按键 | 引脚 | 有效电平 | 上下拉 | LVGL 键值 |
|--------|------|----------|--------|-----------|
| WAKE_UP | PC13 | 高 | 下拉 | `LV_KEY_ESC` |
| KEY2 | PD8 | 低 | 上拉 | `LV_KEY_PREV` |
| KEY1 | PD9 | 低 | 上拉 | `LV_KEY_ENTER` |
| KEY0 | PD10 | 低 | 上拉 | `LV_KEY_NEXT` |

> 注意：`lv_port_indev.c` 中的按键映射与标准 LVGL 语义不同——WAKE_UP→ESC（非 UP），KEY0→NEXT（非 RIGHT），KEY1→ENTER（始终，非长按）。这是当前实际实现。

### 测试

- **Unity** 测试框架：`Test/unity/` 目录用于单元测试
- Flash 测试：`bsp_flash_run_tests()` 在 `BSP/bsp_flash.c`，通过在 `user.cmake` 定义 `RUN_FLASH_TESTS` 启用
- 测试源文件在 `user.cmake` 第 46 行被注释

### MCU 配置（STM32CubeMX）

- `TEST.ioc` 是 STM32CubeMX 项目文件——修改外设配置后用它重新生成代码
- `Core/Src/` 和 `Core/Inc/` 中生成的代码**不应手动修改**（`USER CODE BEGIN/END` 标记之间的区域可以安全编辑）
- 关键外设：USART1（FreeMaster）、USART2（ESP8266 WiFi，DMA 环形接收）、SPI3（显示屏）、TIM2（PWM CH1/CH2 电机控制）、TIM3（LVGL 时基）、RTC（时钟）、USB_OTG_FS

### UART 分配

| UART | 用途 | 波特率 |
|------|------|--------|
| USART1 | FreeMaster 调试监控 | 115200 |
| USART2 | ESP8266 WiFi（AT 指令） | 115200，DMA RX |
| USART3 | Modbus | 115200 DMA RX |

## 代码生成注意事项 / Code Generation Notes

STM32CubeMX 从 `TEST.ioc` 重新生成代码时，会覆盖大量文件。CMake 构建是安全的，因为 CubeMX 生成的源码在 `cmake/stm32cubemx/CMakeLists.txt` 中管理（与用户代码分离）。用户添加的内容应放在 `user.cmake` 中，不受代码生成影响。
