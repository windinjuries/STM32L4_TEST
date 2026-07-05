# STM32L475 + W5500 + lwIP 移植说明

> 文档版本：1.0  
> 适用工程：`STM32L4_TEST-feature-cmake_gcc`  
> MCU：STM32L475VET6（512KB Flash / 128KB RAM）  
> 构建系统：CMake + STM32CubeMX 生成代码  
> RTOS：FreeRTOS + CMSIS-RTOS v1  

---

## 1. 需求概述

### 1.1 目标

在现有 STM32L4 工程中，通过 **SPI1** 连接 **W5500** 以太网模块，移植 **lwIP 2.2.1** 协议栈，实现有线以太网通信能力。

### 1.2 已确认的需求选项

| 项目 | 选择 |
|------|------|
| 硬件接口 | SPI1 + W5500 |
| SPI 速率 | 10 MHz |
| 应用方向 | 自定义 TCP/UDP 应用（HTTP/MQTT 等可后续扩展） |
| IP 配置 | **DHCP** 自动获取 |
| W5500 驱动 | WIZnet **ioLibrary_Driver 3.2.0** |
| 收包方式 | **INT 引脚中断** + 专用收包任务 |
| LVGL | 已关闭（`CONFIG_MODULE_LVGL_ENABLE = 0`） |
| Modbus RTU | **保留**（USART3），不做 Modbus TCP |
| ESP8266 WiFi | **保留**（USART2），后续由用户自行处理 |
| 联调验收 | **TCP 客户端**连接外网/PC 服务器 |
| CubeMX | 当前**直接改源码**；后续用户自行完善 `.ioc` |

### 1.3 非目标（本次不做）

- Modbus TCP 从站
- 用 W5500 硬件 TCP/IP 栈替代 lwIP（本次采用 MACRAW + lwIP 标准架构）
- 替换或移除 ESP8266 模块逻辑
- IPv6、PPP、SNMP 等 lwIP 高级特性

---

## 2. 系统架构

### 2.1 分层结构

```
┌─────────────────────────────────────────┐
│  应用层：tcp_client_demo / 用户业务      │
├─────────────────────────────────────────┤
│  lwIP：TCP / UDP / IP / DHCP / DNS      │
├─────────────────────────────────────────┤
│  netif：ethernetif（W5500 MACRAW）       │
├─────────────────────────────────────────┤
│  ioLibrary：wizchip_conf / w5500 / socket│
├─────────────────────────────────────────┤
│  硬件抽象：w5500_hw（SPI1 / CS / RST / INT）│
├─────────────────────────────────────────┤
│  STM32 HAL：SPI1 / GPIO / EXTI           │
└─────────────────────────────────────────┘
         W5500 芯片 ── RJ45
```

### 2.2 关键设计说明

**W5500 MACRAW 模式**

- W5500 的 **Socket 0** 配置为 `Sn_MR_MACRAW`，仅作为以太网 MAC/PHY 使用。
- **IP 地址、DHCP、TCP/UDP** 全部由 **lwIP** 软件栈处理。
- 不使用 ioLibrary 自带的 `Internet/DHCP/dhcp.c`（那是给 W5500 硬件栈用的）。

**FreeRTOS 多任务模型**

| 任务/线程 | 说明 |
|-----------|------|
| `tcpip` | lwIP 内核线程（`tcpip_init` 创建） |
| `ethif` | 以太网收包线程（等待 INT 信号量，调用 `ethernetif_input`） |
| `netTask` | 应用网络任务（初始化 → 等 DHCP → 跑 TCP demo） |
| 其他 | `wifiTask`、Modbus、FreeMaster 等保持原样 |

**中断与线程协作**

1. W5500 INT 引脚（PB1）下降沿触发 `EXTI1_IRQHandler`。
2. ISR 中调用 `ethernetif_notify_rx()`，释放二值信号量。
3. `ethif` 任务被唤醒，在 `LOCK_TCPIP_CORE()` 保护下读取 MACRAW 数据并送入 lwIP。

---

## 3. 硬件连接

### 3.1 引脚定义（临时，可在 CubeMX 中调整）

定义文件：`Middlewares/Third_Party/LwIP/port/w5500_hw.h`

| 信号 | STM32 引脚 | 说明 |
|------|-----------|------|
| SPI1_SCK | PA5 | SPI 时钟 |
| SPI1_MISO | PA6 | SPI 数据入 |
| SPI1_MOSI | PA7 | SPI 数据出 |
| W5500_CS | PA4 | 片选，软件控制 |
| W5500_RST | PB0 | 复位，低有效 |
| W5500_INT | PB1 | 中断，下降沿，EXTI1 |

### 3.2 SPI 参数

| 参数 | 值 |
|------|-----|
| 模式 | Mode 0（CPOL=0, CPHA=0） |
| 数据位 | 8 bit |
| 位序 | MSB First |
| 速率 | 10 MHz（系统 80 MHz，预分频 `/8`） |
| NSS | 软件 CS（PA4） |

### 3.3 引脚冲突检查

当前工程已占用资源（移植时需注意）：

- **SPI3**：PC11 / PB3 / PB5（疑为 LCD 等外设）
- **USART2**：PA2 / PA3（ESP8266）
- **USART3**：PB10 / PB11（Modbus RTU）
- **USART1**：PA9 / PA10（FreeMaster 等）
- **USB**：PA11 / PA12
- **SWD**：PA13 / PA14

SPI1 使用 PA4~PA7、PB0~PB1，与上述资源无冲突。

---

## 4. 软件目录与文件清单

### 4.1 第三方库（已有，未改内容）

| 路径 | 说明 |
|------|------|
| `Middlewares/Third_Party/lwip-STABLE-2_2_1_RELEASE/` | lwIP 2.2.1 源码 |
| `Middlewares/Third_Party/ioLibrary_Driver-3.2.0/` | WIZnet 官方 ioLibrary |

### 4.2 新增：lwIP 移植层

目录：`Middlewares/Third_Party/LwIP/port/`

| 文件 | 作用 |
|------|------|
| `lwipopts.h` | lwIP 功能裁剪、内存池、线程参数 |
| `arch/cc.h` | 编译器/类型/断言适配 |
| `arch/sys_arch.h` | FreeRTOS 同步原语类型定义 |
| `w5500_hw.h` / `w5500_hw.c` | SPI1 初始化、CS/RST/INT GPIO、ioLibrary SPI 回调注册 |
| `ethernetif.h` / `ethernetif.c` | lwIP netif 驱动（MACRAW 收发、收包任务） |

复用 lwIP 官方 FreeRTOS 移植（未复制，CMake 直接引用）：

- `lwip-STABLE-2_2_1_RELEASE/contrib/ports/freertos/sys_arch.c`

### 4.3 新增：应用层网络代码

目录：`APP/`

| 文件 | 作用 |
|------|------|
| `net_init.h` / `net_init.c` | `tcpip_init`、netif 注册、DHCP 启动、`net_task` |
| `tcp_client_demo.h` / `tcp_client_demo.c` | TCP 客户端联调 demo |

### 4.4 修改：工程与底层

| 文件 | 修改内容 |
|------|----------|
| `user.cmake` | 添加 lwIP / ioLibrary / port / APP 源文件与头文件路径 |
| `Core/Src/main.c` | 启动时调用 `w5500_spi1_init()` |
| `Core/Src/stm32l4xx_hal_msp.c` | 添加 SPI1 MSP（PA5/6/7） |
| `Core/Src/stm32l4xx_it.c` | EXTI1 中断 + `HAL_GPIO_EXTI_Callback` |
| `Core/Inc/FreeRTOSConfig.h` | `configTOTAL_HEAP_SIZE` 3000 → **24576** |
| `APP/app_main.c` | 创建 `netTask` 线程 |
| `APP/app_main.h` | 声明 `net_task()` |

---

## 5. 修改方式说明

### 5.1 为何直接改源码而非仅 CubeMX

按需求约定：

1. **当前阶段**：直接在 `main.c`、`stm32l4xx_hal_msp.c`、`stm32l4xx_it.c` 等文件中添加 SPI1 / GPIO / 中断代码，保证功能快速打通。
2. **后续阶段**：用户在 CubeMX 中配置 SPI1、PA4/PB0/PB1，重新生成后需将 USER CODE 区代码与 `w5500_hw.h` 引脚定义对齐。

建议 CubeMX 配置项：

- SPI1：Master，Full-Duplex，8-bit，Prescaler=8，Mode 0
- PA4：GPIO_Output（CS，初始高）
- PB0：GPIO_Output（RST）
- PB1：GPIO_EXTI，Falling edge，Pull-up
- NVIC：EXTI line1 interrupt，优先级建议 ≥ 6（低于 FreeRTOS 可调用 API 的阈值 5）

### 5.2 CMake 集成方式

所有用户侧源文件通过 **`user.cmake`** 统一管理，不修改 CubeMX 生成的 `cmake/stm32cubemx/CMakeLists.txt`。

`user.cmake` 中新增三组源文件：

- `USER_SOURCES_LWIP_CORE`：lwIP 核心 + IPv4 + API + `ethernet.c`
- `USER_SOURCES_LWIP_PORT`：port 层 + `sys_arch.c`
- `USER_SOURCES_W5500`：`wizchip_conf.c`、`w5500.c`、`socket.c`

头文件搜索路径新增：

```
Middlewares/Third_Party/LwIP/port
Middlewares/Third_Party/LwIP/port/arch
Middlewares/Third_Party/lwip-STABLE-2_2_1_RELEASE/src/include
Middlewares/Third_Party/lwip-STABLE-2_2_1_RELEASE/contrib
Middlewares/Third_Party/lwip-STABLE-2_2_1_RELEASE/contrib/ports/freertos/include
Middlewares/Third_Party/ioLibrary_Driver-3.2.0/Ethernet
Middlewares/Third_Party/ioLibrary_Driver-3.2.0/Ethernet/W5500
```

### 5.3 初始化时序

```
main()
  ├─ HAL_Init()
  ├─ SystemClock_Config()
  ├─ MX_GPIO_Init()
  ├─ MX_DMA_Init()
  ├─ MX_SPI3_Init()
  ├─ w5500_spi1_init()          ← SPI1 硬件就绪
  ├─ ... 其他外设 ...
  ├─ app_init()
  └─ osKernelStart()
        └─ app_task_init()
              └─ osThreadCreate(netTask)
                    └─ net_init()
                          ├─ w5500_hw_init()    ← GPIO/RST/回调/INT 配置
                          └─ tcpip_init()
                                └─ tcpip_init_done()
                                      ├─ netif_add + DHCP
                                      └─ ethernetif_start_input_task()
                    └─ 等待 DHCP 成功
                    └─ tcp_client_demo_run()
```

---

## 6. 关键配置参数

### 6.1 lwIP（`lwipopts.h`）

| 宏 | 值 | 说明 |
|----|-----|------|
| `NO_SYS` | 0 | 使用 RTOS |
| `LWIP_DHCP` | 1 | DHCP 客户端 |
| `LWIP_IPV6` | 0 | 关闭 IPv6 省 RAM |
| `MEM_SIZE` | 8192 | lwIP 堆 |
| `PBUF_POOL_SIZE` | 8 | 接收缓冲池 |
| `MEMP_NUM_TCP_PCB` | 4 | 最大 TCP 连接数 |
| `TCPIP_THREAD_STACKSIZE` | 1024 | tcpip 线程栈（words） |
| `LWIP_TCPIP_CORE_LOCKING` | 1 | 允许非 tcpip 线程加锁访问 |

### 6.2 FreeRTOS

| 宏 | 修改前 | 修改后 |
|----|--------|--------|
| `configTOTAL_HEAP_SIZE` | 3000 | **24576** |

`netTask` 栈：512 words；`ethif` 栈：512 words（在 `ethernetif.c` 中创建）。

### 6.3 TCP 联调参数（`tcp_client_demo.h`）

```c
#define TCP_DEMO_SERVER_IP      "192.168.1.100"   // 改为实际服务器 IP
#define TCP_DEMO_SERVER_PORT    8080
#define TCP_DEMO_SEND_MSG       "Hello from STM32 W5500\r\n"
```

---

## 7. 各模块职责详解

### 7.1 `w5500_hw.c`

- 实现 ioLibrary 所需的 SPI 回调：`reg_wizchip_spi_cbfunc`、`reg_wizchip_cs_cbfunc`、`reg_wizchip_cris_cbfunc`。
- SPI 访问使用 **FreeRTOS 互斥锁**保护，避免多任务竞争。
- `w5500_spi1_init()`：配置 SPI1 为 10 MHz。
- `w5500_hw_init()`：配置 CS/RST/INT GPIO，执行硬件复位。

### 7.2 `ethernetif.c`

- `low_level_init()`：`CW_INIT_WIZCHIP` 分配 Socket0 全部 16KB 缓冲，打开 MACRAW。
- `low_level_output()`：通过 `sendto()` 发送以太网帧。
- `low_level_input()`：通过 `recvfrom()` 读取以太网帧并封装为 `pbuf`。
- `ethernetif_start_input_task()`：创建 `ethif` 任务并使能 INT 中断。

### 7.3 `net_init.c`

- 调用 `tcpip_init()` 异步启动 lwIP。
- 在 `tcpip_init_done` 回调中：`netif_add` → `dhcp_start` → 启动收包任务。
- `netif_status_callback`：检测到有效 IP 后设置 `s_net_ready = 1`。
- `net_task`：阻塞等待网络就绪，然后调用 TCP demo。

### 7.4 `tcp_client_demo.c`

- 使用 lwIP **BSD socket API**（`socket` / `connect` / `send` / `recv`）。
- 连接失败每 2 秒重试，成功后发送测试字符串并尝试接收响应。

---

## 8. 联调步骤

### 8.1 硬件准备

1. W5500 模块按引脚表连接 STM32。
2. 网线接入路由器/交换机，确认链路 LED 正常。

### 8.2 软件准备

1. 修改 `APP/tcp_client_demo.h` 中的 `TCP_DEMO_SERVER_IP` 和端口。
2. 在 PC 或服务器上启动 TCP 监听，例如：

   ```bash
   # Linux / macOS
   nc -lk 8080

   # 或使用 Python
   python -m http.server 8080
   ```

3. 编译、下载固件。

### 8.3 验证顺序

| 步骤 | 验证项 | 预期 |
|------|--------|------|
| 1 | 编译 | 无 error |
| 2 | 运行 | 无 HardFault |
| 3 | DHCP | 路由器 DHCP 列表出现新设备（MAC: 02:00:00:12:34:56 或 SHAR 中设置的值） |
| 4 | Ping | PC 能 ping 通板子获取到的 IP |
| 5 | TCP | 服务器收到 `"Hello from STM32 W5500\r\n"` |

### 8.4 常见问题排查

| 现象 | 可能原因 |
|------|----------|
| SPI 读写失败 / 无法打开 MACRAW | 接线错误、SPI 模式不对、CS 时序问题 |
| 无 DHCP | 网线/链路未 up、INT 未触发收包、DHCP Discover 未发出 |
| DHCP 成功但 TCP 连不上 | 服务器 IP/端口错误、防火墙、跨网段 |
| HardFault | FreeRTOS 堆不足、栈溢出，可增大 `configTOTAL_HEAP_SIZE` 或任务栈 |
| INT 无响应 | PB1 未配 EXTI、NVIC 未使能、W5500 IMR 未开 |

---

## 9. 内存与扩展建议

### 9.1 当前 RAM 占用概览

STM32L475 共 128KB RAM，主要消耗：

- FreeRTOS 堆：24 KB
- lwIP `MEM_SIZE`：8 KB
- 各任务栈：defaultTask(128) + netTask(512) + ethif(512) + tcpip(1024) + 其他任务
- W5500 Socket0 缓冲：16KB TX + 16KB RX（在 W5500 芯片内部 SRAM，不占 MCU RAM）

若后续增加 HTTP/MQTT，建议：

- 适当增大 `MEM_SIZE`、`MEMP_NUM_TCP_PCB`
- 监控 `xPortGetFreeHeapSize()` 防止堆耗尽
- 考虑关闭 `user.cmake` 中未使用的 LVGL 源文件编译以节省 Flash

### 9.2 扩展 TCP/UDP 应用

在 `net_is_ready()` 返回 1 后，可直接使用 lwIP socket API：

```c
#include "lwip/sockets.h"

int sock = socket(AF_INET, SOCK_STREAM, 0);
// connect / send / recv ...
```

或启用 lwIP `netconn` API，按需打开 `LWIP_NETCONN`。

---

## 10. 后续工作清单

- [ ] 在 CubeMX 中正式配置 SPI1 / GPIO / EXTI，与 `w5500_hw.h` 保持一致
- [ ] 按实际硬件修改引脚定义
- [ ] 修改 `TCP_DEMO_SERVER_IP` 并完成联调
- [ ] 按需扩展 HTTP / MQTT / UDP 业务
- [ ] 评估是否与 ESP8266 做路由/优先级策略
- [ ] 可选：增加链路检测（`ctlwizchip(CW_GET_PHYLINK)`）与 `netif_set_link_up/down`

---

## 11. 参考

- lwIP 2.2.1：`Middlewares/Third_Party/lwip-STABLE-2_2_1_RELEASE/`
- WIZnet ioLibrary：`Middlewares/Third_Party/ioLibrary_Driver-3.2.0/`
- W5500 数据手册：MACRAW 模式、SPI 时序
- lwIP FreeRTOS port：`contrib/ports/freertos/sys_arch.c`

---

*文档随工程代码同步维护。引脚、IP、端口等参数以 `w5500_hw.h`、`tcp_client_demo.h`、`lwipopts.h` 源文件为准。*
