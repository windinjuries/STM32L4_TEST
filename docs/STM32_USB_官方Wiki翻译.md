# STM32 USB 官方 Wiki 介绍（中文翻译）

> **源页面**：<https://wiki.st.com/stm32mcu/wiki/USB_overview>
>
> 该 URL 实际会重定向到 **Introduction to USB with STM32**
> （`USB_overview` 是页内的锚点标题，并非独立页面）。本文按该页原始内容逐节翻译，
> 保留原文章节结构。
>
> 原文另含大量 "USB Host library overview" 与各类函数表格，本文只翻译与本项目
> （USB Device / 麦克风 / CDC）相关的内容，未译章节在第 12 节列出。
>
> 翻译时间：2026-10-20

---

## 目录

- [1. 什么是通用串行总线（USB）](#1-什么是通用串行总线usb)
- [2. STM32 与 USB 入门](#2-stm32-与-usb-入门)
- [3. 符合 USB 的 STM32](#3-符合-usb-的-stm32)
- [4. USB 设备库概览](#4-usb-设备库概览)
- [5. USB 设备核心模块](#5-usb-设备核心模块)
- [6. USB 设备库主要数据结构](#6-usb-设备库主要数据结构)
- [7. USB 设备硬件抽象层与低层](#7-usb-设备硬件抽象层与低层)
- [8. USB 设备应用实现](#8-usb-设备应用实现)
- [9. USB 设备组件组织](#9-usb-设备组件组织)
- [10. 与本项目的对照](#10-与本项目的对照)
- [11. 参考来源](#11-参考来源)
- [12. 未翻译的章节](#12-未翻译的章节)

---

## 1. 什么是通用串行总线（USB）

各类端口（并口、串口、MIDI、游戏手柄等）的多样性及其各自的特定要求，加上缺乏即插即用
能力，几乎是促使科技领域最知名的那些公司寻找替代方案的主要原因。这些公司制定了 USB
标准，并成立了 **USB 实施者论坛（USB Implementers Forum）**。

USB 提供了最为成功的串行接口，具有以下特性：

- **简单灵活**（即插即用）
- **双向性**
- **速度不断提升**
- **成本低廉**

自诞生以来，USB 一直在持续改进，同时始终与新技术的发展和要求保持兼容。

STM32 的 USB 硬件和软件符合 **USB 1.1 和 USB 2.0 规范**，以下所有章节讨论的都是符合
这些标准的设备和主机。

### 1.1 速度（Speed）

USB 2.0 支持三种速度：

| 速度 | 速率 | 主要用途 |
|---|---|---|
| **低速（LS）** | 1.5 Mb/s | 交互式设备（鼠标、键盘） |
| **全速（FS）** | 12 Mb/s | 电话和音频设备（**麦克风、扬声器**） |
| **高速（HS）** | 480 Mb/s | 视频和存储设备（打印机、摄像头） |

在协议层面，USB 保证了很高的兼容性，因此用户在处理不同速度时不会感到太大差异。

> 📌 **与本项目的关联**：原文明确把「麦克风」归到全速设备的典型应用。本项目
> `PCD_SPEED_FULL` + 内嵌 PHY 的配置正符合这一点；48 kHz × 2ch × 16bit = 192 KB/s
> 的码率在 12 Mb/s 总线带宽下余量很充足。

### 1.2 USB 互连的组成部件

USB 互连有三个主要组成部分：

- **主机（Host）或根集线器（Root Hub）**：每个 USB 系统只有一个。负责发起所有事务。
- **功能设备（Function / Device）**：互连的末端，实现用户要求的功能（如键盘、鼠标、
  **麦克风**）。
- **集线器（Hub）**：一个桥接装置，确保主机与多个设备之间的通信。它有一个上行端口，
  通过 USB 线直接（点对点）或间接（经由另一个集线器）连接到主机；还有多个下行端口，
  可直接或间接连接到 USB 功能设备。

上述部件之间通过 USB 线缆互连，线缆**最大长度为 5 米**。

USB 规范允许串联的集线器**最多 5 个**。因此，一个功能设备可以通过一个或多个集线器
连接到主机。

### 1.3 USB 互连拓扑

USB 物理互连的特征是**分层星形拓扑（tiered star topology）**。每个星形的中心是一个
集线器，它有一条上行连接（直接或间接连到主机），以及一条或多条下行连接（连到功能
设备或其他集线器）。

一个主机（根集线器）最多可连接 **127 个设备**（功能设备或集线器），其中串联的集线器
最多 5 个。

### 1.4 供电（Power）

通常，主机（根集线器）为直接连接的功能设备供电。某些集线器也可以为其直接连接的下行
功能设备供电。对于功能设备，有两种类型：

- **总线供电设备（Bus powered functions）**：完全依赖来自上游集线器的总线电源。
- **自供电设备（Self powered functions）**：能够独立于总线自行供电。

> 📌 这一点决定了配置描述符里的 `bmAttributes`。本项目在
> [`usbd_composite_builder.c`](Middlewares/ST/STM32_USB_Device_Library/Class/CompositeBuilder/Src/usbd_composite_builder.c)
> 的 `USBD_CMPSIT_AddConfDesc()` 中走 `#else` 分支填 `0x80`（bus powered），
> 符合「STM32 + ES8388 全部从 USB 取电」的常见板级设计。

### 1.5 系统配置（System configuration）

USB 系统有一个智能机制，可以随时检测设备的接入和拔出。

- **设备接入（Device attachment）**：主机通过持续查询所有已连接集线器端口的状态位，
  随时检测设备拔出。一旦有设备接入，主机便使能该端口，给设备分配一个唯一地址，然后
  与这个新连接的设备建立通信，以确定它是一个功能设备还是一个集线器。
- **设备拔出（Device detachment）**：设备一旦拔出，对应端口即被禁用。如果拔出一个
  集线器，那么挂在该集线器上的所有下行设备的端口都会被禁用，同时该集线器的上行端口
  也被禁用。
- **总线枚举（Bus enumeration）**：指一组硬件和软件事件，使主机能够持续管理新接入或
  断开设备的事件过程。它也包括确保设备被移除的一系列事件。

### 1.6 类（Class）

根据所需的 USB 设备功能和应用，设备和主机的特性各不相同。USB IF 定义了多种**类**，
以便按所需功能进行分类。每个类定义了一组关联：

- **数据**：包含必须存储在设备中、并在被请求时提供给主机的特征信息。
- **软件驱动**：存储在主 机中，在与设备协商并发现其特征之后加载。

USB IF 在 <https://www.usb.org/defined-class-codes> 提供了所有已定义 USB 类及其代码的
概览。

关于 STM32 USB **设备**类的更多细节，请参阅原文的 "USB Device Classes" 一节。

### 1.7 USB 数据传输（USB data transfers）

USB 通信基于四种主要的传输类型：

- **控制传输（Control transfer）**：主要用于新接入设备的配置数据。
- **批量传输（Bulk transfer）**：用于大量数据的发送或接收。
- **中断传输（Interrupt transfer）**：用于有限数据的传输，要求延迟最小。
- **等时传输（Isochronous transfer）**：用于有实时性要求的数据传输。

> 📌 **与本项目的精确对应**：
>
> | 本项目端点 | 传输类型 |
> |---|---|
> | Audio 等时 IN `0x81` | 等时传输（保带宽、不重传） |
> | CDC 批量 OUT `0x01` / 批量 IN `0x82` | 批量传输 |
> | CDC 中断 IN `0x83` | 中断传输（承载 `SET_CONTROL_LINE_STATE` 等类请求） |
> | EP0 | 控制传输 |

### 1.8 数据方向（Data direction）

如前所述，USB 只支持 4 种传输类型。对于所有传输，**始终是主机发起**与设备的通信。
实际上，USB 可以双向传输数据，而参考方向始终以主机为准：

- **OUT**：从主机传向设备的数据
- **IN**：从设备传入主机的数据

### 1.9 事务（Transactions）

每种传输，无论其类型如何，都基于一个或多个**事务**来完成主机与设备之间的信息交换。
每个事务本身最多由三个阶段（数据包）组成：

- **令牌包（Token）**：启动每个事务所必需。这是由主机发给设备的 USB 数据包，提供
  有关该事务的信息（如类型、目的地、数据阶段方向）。
- **数据包（Data）**：此阶段**可选**。其存在与否、方向和大小由令牌包指明。在此阶段，
  如果存在数据，由数据源发出指定的数据。
- **握手包（Handshake）**：此阶段**可选**。其存在与否和方向可以从令牌包推断。实际上，
  根据令牌包中指定的数据方向，数据的目的地发送此包以确认数据接收状态。

### 1.10 请求（Requests）

如前所述，主机总是通过发送**请求**来发起与设备的通信。所有 USB 请求都有共同的字段。
每个字段的大小、取值和含义对设备和主机都是已知的。总是由主机负责填充请求字段并发送；
另一方面，设备接收请求并解码它，为下一通信阶段做准备。

### 1.11 描述符（Descriptor）

它是存储在设备内存中、以设备和主机都知晓的确定方式组织的一组数据块。

某些描述符是所有设备都必需的，**主机无法与缺少必需描述符的设备继续通信**。另一些
描述符是可选的，并且会因设备功能不同而异。

每个描述符都包含一组关于设备的确定信息，并会作为对特定请求的应答发送给主机。

### 1.12 端点（Endpoint）

它是必须在设备侧实现的**数据缓冲区**（数据源或目的地）。从主机接收或发往主机的每一份
数据都会放进某个端点。

每个端点由**编号和方向**唯一表征。这意味着：对于一个给定编号，存在唯一的一对同类型
同编号的端点，但每个端点只处理一个方向的数据：

- **IN 端点**：将要传入主机的数据。
- **OUT 端点**：将要来自主机的数据。

一个设备总是有多个由编号标识的端点对：

- **端点零（Endpoint zero）**：一对 OUT 和 IN 端点，专用于控制数据传输，即控制端点。
  在其他端点尚未配置好之前，它是建立主机与设备之间首次通信传输所必需的。
- **其他端点**：在主机与设备协商之后配置。每个端点彼此独立，可以处理不同类型的传输。

### 1.13 地址（Address）

由主机分配的唯一值，取值范围 **1 到 127**。地址 0 总是给新接入的设备，在被主机编址
之前使用。

### 1.14 枚举（Enumeration）

它是确保设备状态变化控制以及任何设备接入 / 拔出实时管理的过程。在此步骤中，存在一系列
硬件和软件协商，使主机能够解码设备的本性。在设备软件层面，此过程确保主机请求被正确
接收和解码，然后相应地修改设备状态。

### 1.15 设备状态（Device status）

从完全拔出状态，到被 USB 主机完全识别并行使其功能，USB 设备经历以下连续阶段：

| 状态 | 含义 |
|---|---|
| **接入（Attached）** | 设备物理连接到 USB 主机但尚未供电的阶段。主要由硬件保证。 |
| **上电（Powered）** | 已接入 USB 主机并刚获得供电的设备。主要由硬件保证。 |
| **默认（Default）** | 接入的设备上电后被主机复位时到达。每次新接入、上电、复位，或已接入的旧设备收到复位时，其软件都会把设备置于此状态。此阶段设备以便于工作的速度（复位期间由硬件选择）运行，并具有默认地址 0。 |
| **已编址（Addressed）** | 从主机接收唯一地址（非 0）后到达。由设备软件正确处理主机请求后达到。 |
| **已配置（Configured）** | 收到带有非零配置编号的相应请求后到达。由设备软件正确处理主机请求后达到。 |
| **挂起（Suspended）** | 如果流量在某一依赖于速度的已知时间段内没有数据，设备必须进入此阶段。主机会根据速度以电气方式强制设备进入。检测到该电气指示时，USB 设备软件必须把其状态改为挂起。 |

> 📌 **与本项目的关联**：这些状态名在代码里都有对应常量
> `USBD_STATE_DEFAULT` / `USBD_STATE_ADDRESSED` / `USBD_STATE_CONFIGURED`。
> `MX_USB_DEVICE_Init()` 中的 `USBD_Init()` 先把 `dev_state` 设为
> `USBD_STATE_DEFAULT`，主机随后经 `SET_ADDRESS` → `SET_CONFIGURATION`
> 把它推进到 `CONFIGURED`，各类的 `Init()` 才在那一刻被调用。

---

## 2. STM32 与 USB 入门

本节回答开始使用 STM32 MCU 的 USB 时可能遇到的问题。

### 2.1 STM32 支持什么？

所有 STM32 系列（**并非所有型号**）都包含 USB IP。根据硬件不同，每个带 USB 的 STM32
MCU 可以支持：

- 仅 FS 速度的 Device（设备）
- FS 速度的 OTG（双角色：设备和主机）
- HS 速度的 OTG

某些 STM32 MCU 包含两个 USB 外设，同时支持 FS 和 HS 速度的 OTG。

### 2.2 STM32 MCU 在 USB 系统中能扮演什么角色？

一个基本的 USB 系统可以由一个主机和一个通过 USB 线连接的设备建立。

对于更复杂的 USB 系统，设备可以通过一个或多个集线器连接到主机。

### 2.3 如何为我的 USB 应用选择 STM32 MCU？

对于每个 USB 应用，选择具有所需 USB 角色（主机或设备）和速度（LS / FS / HS）的正确
STM32 很重要。但是有许多具有相同 USB 硬件实现的 STM32 MCU，该如何选择？

除了 USB 特性之外，STM32 MCU 还提供了大量具有丰富功能组合的外设。外设和功能的多样性
保证了所需应用实现的灵活性和易用性。事实上，**选择合适的 MCU 是 USB 应用成功的最大
秘诀之一**。

为选择合适的 MCU，必须根据应用需求考虑以下特性：

- **MCU 性能**：直接影响 STM32 系统中的数据传输和处理。
- **存储器（RAM 和 ROM）的可用性和容量**：对于处理大量数据的应用非常重要。
- **所需外设的可用性和特性**：选择时必须检查，因为外设版本和组合在不同 MCU 之间
  可能差异很大。
- **功耗**：对于某些应用是非常重要的要求。

### 2.4 在哪里可以找到关于所选 STM32 MCU 的 USB 更详细信息？

对于每个 STM32 MCU，都有一组文档提供关于所有集成外设的信息，其中就包括 USB。所有
STM32 MCU 都有一个页面提供概览，并直接链接到该 MCU 的数据手册。

实际上，所有 STM32 MCU 的 USB 外设信息主要集中在：

- **数据手册（datasheet）**——尤其是电气特性部分
- **参考手册（reference manual）**——包含该 MCU 所有外设的信息，因此提供关于集成
  USB 外设的详细信息

所有 STM32 MCU 技术文档都可以在 ST 官网的 STM32 页面找到。

### 2.5 有没有提供 STM32 USB 代码示例？

每个 STM32 系列都有一个包含该 MCU 驱动和外设示例的软件包，称为
**STM32Cube Firmware 包**。该包包含 USB Device 和 Host（如果 MCU 支持）驱动，以及
所有支持速度的代码示例。

要访问 USB 代码，需要先安装 STM32Cube Firmware 包。打开安装文件夹后，按以下步骤操作：

1. 打开 `Projects` 子文件夹
2. 打开以所选 MCU 对应板卡命名的文件夹
3. 打开 `Applications` 子文件夹
4. 选择 USB 相应的子文件夹（设备应用选 `USB_Device`，主机应用选 `USB_Host`）
5. 选择相应的应用
6. 选择相应的 IDE（IAR、STM32CubeIDE、KEIL）

此时，应用的代码可以被调试，也可以下载到 STM32 中运行，以验证 `readme.txt` 中描述的
功能。

---

## 3. 符合 USB 的 STM32

下表按 STM32 系列概览了 USB 硬件实现。如同下表所示，同一系列中的某些型号可能与其他
型号的 USB 硬件不同。

| STM32 系列 | 仅 USB Device | USB OTG FS | USB OTG HS | USB OTG FS&HS |
|---|---|---|---|---|
| STM32F0 | ✓ | | | |
| STM32F1 | ✓ | ✓ | | |
| STM32F2 | | ✓ | ✓ | ✓ |
| STM32F3 | ✓ | | | |
| STM32F4 | | ✓ | ✓ | ✓ |
| STM32F7 | | ✓ | ✓ | ✓ |
| STM32G4 | ✓ | | | |
| STM32H7 | | ✓ | ✓ | ✓ |
| STM32L0 | ✓ | | | |
| STM32L1 | ✓ | | | |
| **STM32L4** | ✓ | ✓ | | |
| STM32L4+ | | ✓ | | |
| STM32L5 | ✓ | | | |
| STM32WB | ✓ | | | |

> 📌 **与本项目的关联**：本项目板子 STM32L4 落在「仅 Device」和「OTG FS」两列。
> 代码里用 `USB_OTG_FS` + `PCD_PHY_EMBEDDED` + `PCD_SPEED_FULL`，正是
> 「仅 Device / FS」这一档的实现方式——没有使用 OTG 双角色，也没有外接 HS PHY。
>
> 另外，ST 的 [AN4879](https://www.st.com/content/st_com/en/technical-documents/an4879-introduction-to-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.html)
> 补充说明：**STM32L4x1 不含 USB**，选型时需注意。

---

## 4. USB 设备库概览

借助 STM32 MCU 和板卡，官方提供了一个完整且免费的开发环境 **STM32Cube**，包括两个
主要部分：

- 图形化软件工具
- 嵌入式软件包（STM32Cube firmware）

### 4.1 USB 设备库在 STM32Cube firmware 中的目录结构

在每个 STM32Cube firmware 包中，有一个 `Middlewares` 文件夹，包含所有 ST 及第三方的
中间件库。STM32 USB 设备库属于 `Middlewares/ST` 部分。

`STM32_USB_Device_Library` 包含：

- **Core** 模块：用于 USB 设备标准外设控制的 API
- **Classes** 模块：常用支持类的 API

### 4.2 USB 设备应用在 STM32Cube firmware 中的目录结构

在每个 STM32Cube firmware 包中，有一个专门放应用的文件夹，包含所选 STM32 板卡支持的
所有中间件列表，其中就包括 USB 设备应用。

---

## 5. USB 设备核心模块

`STM32_USB_Device_Library` 包含 Core 和 Classes 两个文件夹。

Core 文件夹包含为 USB 设备实现 USB 2.0 标准代码的文件。这些文件的 API 会在每个
USB 设备应用中被调用，无论所需功能是什么。它包括五个主要模块：

### 5.1 `usbd_core (.c, .h)`

包含 USB 核心状态机控制及各种事件处理的实现。

| 函数 | 说明 |
|---|---|
| `USBD_Init` | 初始化 Device Handler 和状态机 |
| `USBD_DeInit` | 反初始化 Device Handler |
| `USBD_RegisterClass` | 将设备结构句柄链接到类结构句柄 |
| `USBD_Start` | 启动设备核心 |
| `USBD_Stop` | 停止设备核心和类 |
| `USBD_SetClassConfig` | 用输入的配置初始化设备的类 |
| `USBD_ClrClassConfig` | 清除设备某个类的配置 |
| `USBD_LL_SetupStage` | 处理 setup 阶段请求 |
| `USBD_LL_DataOutStage` | 处理在相应端点上来自主机的数据接收 |
| `USBD_LL_DataInStage` | 处理在相应端点上向主机发送数据 |
| `USBD_LL_Reset` | 重新初始化 Device Handler |
| `USBD_LL_SetSpeed` | 设置设备速度 |
| `USBD_LL_Suspend` | 将设备状态改为挂起 |
| `USBD_LL_Resume` | 恢复设备挂起前的旧状态 |
| `USBD_LL_SOF` | 处理 Start Of Frame 事件 |
| `USBD_LL_IsoINIncomplete` | 处理 Iso In Incomplete 事件 |
| `USBD_LL_IsoOUTIncomplete` | 处理 Iso Out Incomplete 事件 |
| `USBD_LL_DevConnected` | 处理设备连接事件 |
| `USBD_LL_DevDisconnected` | 处理设备断开事件 |

> 📌 `USBD_LL_*` 这一族是协议栈到 HAL 的切面。本项目中它们全部实现在
> [`USB_DEVICE/Target/usbd_conf.c`](USB_DEVICE/Target/usbd_conf.c)，
> 每个都是「调 `HAL_PCD_*` + 把 HAL 返回值映射成 `USBD_*` 返回值」的薄封装。

### 5.2 `usbd_ctlreq (.c, .h)`

包含确保处理 USB 2.0 规范**第 9 章**所有请求的变量和函数。

| 函数 | 说明 |
|---|---|
| `USBD_StdDevReq` | 处理所有发给 USB 设备的请求 |
| `USBD_StdItfReq` | 处理所有发给接口的请求 |
| `USBD_StdEPReq` | 处理所有发给端点的请求 |
| `USBD_GetDescriptor` | 处理不同类型的 Get Descriptor 请求 |
| `USBD_SetAddress` | 处理 Set Address 请求 |
| `USBD_GetConfig` | 处理 Get Configuration 请求 |
| `USBD_SetConfig` | 处理 Set Configuration 请求 |
| `USBD_GetStatus` | 处理 Get Status 请求 |
| `USBD_SetFeature` | 处理 Set Feature 请求 |
| `USBD_ClrFeature` | 处理 Clear Feature 请求 |
| `USBD_ParseSetupRequest` | 把接收到的数据缓冲区组织成 setup 请求结构 |
| `USBD_CtlError` | 处理 USB 低层错误 |
| `USBD_GetString` | 把 ASCII 字符串转成 Unicode |
| `USBD_GetLen` | 返回字符串长度 |

### 5.3 `usbd_ioreq (.c, .h)`

包含在端点 0 上发送和接收数据的实现。

| 函数 | 说明 |
|---|---|
| `USBD_CtlSendData` | 开始在端点 0 上发送数据 |
| `USBD_CtlContinueSendData` | 在端点 0 上发送剩余数据 |
| `USBD_CtlPrepareRx` | 准备端点 0 接收数据 |
| `USBD_CtlContinueRx` | 继续在端点 0 上接收数据 |
| `USBD_CtlSendStatus` | 在端点 0 上发送零长度包 |
| `USBD_CtlReceiveStatus` | 在端点 0 上接收零长度包 |
| `USBD_GetRxCount` | 返回接收到的数据长度 |

### 5.4 `usbd_conf_template (.c, .h)`

开发新应用时，实现 USB 设备配置和中断回调的模板。

### 5.5 `usbd_desc_template (.c, .h)`

开发新应用时，实现 USB 设备描述符的模板。

### 5.6 `usbd_def.h`

包含所有 USB 设备通用常量和结构定义。

> 📌 在本项目中，`usbd_conf.c` / `usbd_desc.c` 就是 5.4 / 5.5 这两个模板的实例化；
> 而 `usbd_core.c` / `usbd_ctlreq.c` / `usbd_ioreq.c` 是官方原封不动的三个文件。

---

## 6. USB 设备库主要数据结构

STM32 USB 设备库提供通用结构来管理各种 USB 设备数据。这些结构可根据类、外设硬件类型
和应用需求进行填充。

### 6.1 Setup 请求处理结构

从主机收到请求后，会被解码并放入以下结构以便处理：

```c
typedef struct usb_setup_req
{
  uint8_t   bmRequest;  /* 该字段包含：数据传输方向信息（下一通信阶段是否需要数据交换）、
                           请求类型、以及精确的目的地（可以是设备、接口或端点）。 */
  uint8_t   bRequest;   /* 该字段指定请求。主要是主机对任何已连接设备都要求的标准请求，
                           以及取决于设备功能的可选请求。 */
  uint16_t  wValue;     /* 该字段包含设备为正确处理所需知晓的请求参数。
                           对某些请求没有必需参数，因此该字段为零。 */
  uint16_t  wIndex;     /* 该字段包含设备为正确处理所需知晓的请求参数。
                           对某些请求没有必需参数，因此该字段为零。 */
  uint16_t  wLength;    /* 该字段指定在 bmRequest 字段指定方向上、下一传输阶段要传输的
                           数据长度。如果不需要数据传输，该字段为零。 */
} USBD_SetupReqTypedef;
```

> 📌 本项目 CompositeBuilder 里大量出现 `HIBYTE(req->wIndex)` 取单元 ID、
> `LOBYTE(req->wIndex)` 取接口号，正是基于此结构的字段定义。

### 6.2 USB 设备描述符句柄结构

为便于处理各种设备描述符，实现了一个专用结构，包含所有通用必需的描述符。该结构可根据
类的要求填充。

```c
typedef struct
{
  uint8_t *(*GetDeviceDescriptor)(USBD_SpeedTypeDef speed, uint16_t *length);
      /* 响应 "Get Descriptor" 请求、描述符类型为设备描述符 */
  uint8_t *(*GetLangIDStrDescriptor)(USBD_SpeedTypeDef speed, uint16_t *length);
      /* 描述符类型为字符串、且索引指向 langId 字符串描述符 */
  uint8_t *(*GetManufacturerStrDescriptor)(USBD_SpeedTypeDef speed, uint16_t *length);
      /* 制造商字符串描述符 */
  uint8_t *(*GetProductStrDescriptor)(USBD_SpeedTypeDef speed, uint16_t *length);
      /* 产品字符串描述符 */
  uint8_t *(*GetSerialStrDescriptor)(USBD_SpeedTypeDef speed, uint16_t *length);
      /* 序列号字符串描述符 */
  uint8_t *(*GetConfigurationStrDescriptor)(USBD_SpeedTypeDef speed, uint16_t *length);
      /* 配置字符串描述符 */
  uint8_t *(*GetInterfaceStrDescriptor)(USBD_SpeedTypeDef speed, uint16_t *length);
      /* 接口字符串描述符 */
  uint8_t *(*GetUserStrDescriptor)(USBD_SpeedTypeDef speed, uint8_t idx, uint16_t *length);
      /* 用户字符串描述符（若支持） */
  uint8_t *(*GetBOSDescriptor)(USBD_SpeedTypeDef speed, uint16_t *length);
      /* BOS 描述符（若支持） */
} USBD_DescriptorsTypeDef;
```

> 📌 本项目 `USB_DEVICE/App/usbd_desc.c` 里的 `FS_Desc` 就是这个结构的实例，
> 串号由 `Get_SerialNum()` 从芯片唯一 ID 动态生成。

### 6.3 USB 设备句柄结构

此结构包含各种 USB 设备数据和信息。可根据类的要求填充。

```c
typedef struct _USBD_HandleTypeDef
{
  uint8_t                 id;              /* 低层核心索引（FS 或 HS），仅为设备软件需求而定义 */
  uint32_t                dev_config;      /* wValue 字段的低字节指定所需配置 */
  uint32_t                dev_default_config; /* 默认配置始终为 0；设备尚未配置且处于 addressed
                                                 状态时由 get_config 调用；设备处于 configured
                                                 状态时由 get status 赋值 */
  uint32_t                dev_config_status;  /* 包含两个信息：设备是否自供电、是否支持远程唤醒 */
  USBD_SpeedTypeDef       dev_speed;       /* 在 usbd_ll_set_speed（core.c）中赋值，包含设备速度 */
  USBD_EndpointTypeDef    ep_in[15];       /* IN 端点总数，取决于设备硬件，最多可达 16 */
  USBD_EndpointTypeDef    ep_out[15];      /* OUT 端点总数，取决于设备硬件，最多可达 16 */
  uint32_t                ep0_state;       /* 端点 0 状态，取决于传输阶段，取值：
                                               USBD_EP0_SETUP            —— 调用 USBD_LL_SetupStage 时
                                               USBD_EP0_IDLE             —— status in/out 阶段完成或复位后
                                               USBD_EP0_DATA_IN / OUT    —— 发送或接收数据时
                                               USBD_EP0_STATUS_IN / OUT  —— 发送或接收 status 时 */
  uint32_t                ep0_data_len;    /* USBD_LL_SetupStage() 中指定的控制传输第二阶段
                                               传输的数据长度 */
  uint8_t                 dev_state;       /* 设备状态，取值：
                                               USBD_STATE_DEFAULT     —— USBD_Init() 初始化、
                                                   USBD_DeInit() 反初始化、USBD_LL_Reset() 复位、
                                                   USBD_LL_DevDisconnected() 断开时
                                               USBD_STATE_SUSPENDED  —— USBD_LL_Suspend() 挂起时
                                               USBD_STATE_ADDRESSED  —— set_address 且收到的地址有效时
                                               USBD_STATE_CONFIGURED —— set config 且设备处于 addressed 时 */
  uint8_t                 dev_old_state;   /* 切换到当前状态之前的设备前一个状态 */
  uint8_t                 dev_address;     /* 收到 Set Address 命令时必须在 USBD_SetAddress()
                                               中修改的地址 */
  uint8_t                 dev_connection_status; /* 设备连接状态 */
  uint8_t                 dev_test_mode;   /* 测试模式；置 1 时软件将运行测试模式功能 */
  uint32_t                dev_remote_wakeup;    /* 远程唤醒支持状态；USBD_SetFeature() 时置 1，
                                               USBD_LL_Reset() 复位后或 USBD_ClrFeature() 时置 0 */
  USBD_SetupReqTypedef    request;         /* 请求结构。USBD_ParseSetupRequest 在 ll_setup_stage 中
                                               用接收到的数据填充请求的各部分。根据 bmRequest，
                                               调用以下之一：
                                                 USBD_StdDevReq  —— 处理标准设备请求
                                                 USBD_StdItfReq  —— 处理标准接口请求
                                                 USBD_StdEPReq   —— 处理标准端点请求 */
  USBD_DescriptorsTypeDef *pDesc;          /* 指向包含所有 USB 描述符的结构，取决于类 */
  USBD_ClassTypeDef       *pClass;         /* 指向包含所有 USB 类参数的结构；
                                               USBD_RegisterClass() 用于填充 usbd_def.h 中定义的
                                               结构，各变量的值在类文件中定义 */
  void                    *pClassData;     /* USBD_class_Handle_Typedef 尺寸的分配 */
  void                    *pUserData;      /* 指向某些类所需、其他类不用的接口结构的指针 */
  void                    *pData;          /* 指向低层栈结构（PCD_HandleTypeDef）的指针，
                                               在 usbd_ll_init() 中初始化 */
  void                    *pBosDesc;       /* 指向 BOS 描述符（若类需要） */
  void                    *pConfDesc;      /* 指向配置描述符 */
} USBD_HandleTypeDef;
```

> 📌 **对照本项目**：`hUsbDeviceFS` 就是这个结构。
>
> - `pData` 在 [`usbd_conf.c`](USB_DEVICE/Target/usbd_conf.c) 的 `USBD_LL_Init()` 里
>   被赋为 `&hpcd_USB_OTG_FS`
> - `pUserData[0]` / `pUserData[1]` 在 `MX_USB_DEVICE_Init()` 里被**手动**赋为
>   audio / cdc 的 fops 表
> - `pConfDesc` 由 CompositeBuilder 每注册一个类就刷新一次

### 6.4 USB 设备类结构

为便于处理设备的各种类，实现了一个专用结构，包含所有通用必需的函数。

```c
typedef struct _Device_cb
{
  uint8_t (*Init)               (struct _USBD_HandleTypeDef *pdev, uint8_t cfgidx);
      /* 允许初始化类 */
  uint8_t (*DeInit)             (struct _USBD_HandleTypeDef *pdev, uint8_t cfgidx);
      /* 允许反初始化类 */
  uint8_t (*Setup)              (struct _USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
      /* 处理类特定请求 */
  uint8_t (*EP0_TxSent)         (struct _USBD_HandleTypeDef *pdev);
      /* 处理端点 0 Tx Ready 事件，可为 NULL */
  uint8_t (*EP0_RxReady)        (struct _USBD_HandleTypeDef *pdev);
      /* 处理端点 0 Rx Ready 数据事件，可为 NULL */
  uint8_t (*DataIn)             (struct _USBD_HandleTypeDef *pdev, uint8_t epnum);
      /* 处理通过非控制 IN 端点发送数据，可为 NULL */
  uint8_t (*DataOut)            (struct _USBD_HandleTypeDef *pdev, uint8_t epnum);
      /* 处理通过非控制 OUT 端点接收数据，可为 NULL */
  uint8_t (*SOF)                (struct _USBD_HandleTypeDef *pdev);
      /* 处理 SOF 事件，可为 NULL */
  uint8_t (*IsoINIncomplete)    (struct _USBD_HandleTypeDef *pdev, uint8_t epnum);
      /* 处理 ISO IN Incomplete 事件，可为 NULL */
  uint8_t (*IsoOUTIncomplete)   (struct _USBD_HandleTypeDef *pdev, uint8_t epnum);
      /* 处理 ISO OUT Incomplete 事件，可为 NULL */
  uint8_t *(*GetHSConfigDescriptor)(uint16_t *length);
      /* 提供 HS 速度配置描述符，不支持 HS 时可为 NULL */
  uint8_t *(*GetFSConfigDescriptor)(uint16_t *length);
      /* 提供 FS 速度配置描述符 */
  uint8_t *(*GetOtherSpeedConfigDescriptor)(uint16_t *length);
      /* 提供 other speed 配置描述符 */
  uint8_t *(*GetDeviceQualifierDescriptor)(uint16_t *length);
      /* 提供 Device Qualifier 描述符 */
  uint8_t *(*GetUsrStrDescriptor)(struct _USBD_HandleTypeDef *pdev, uint8_t index,
                                  uint16_t *length);
      /* 提供用户字符串描述符 */
} USBD_ClassTypeDef;
```

> 📌 **这就是本项目 CompositeBuilder 的核心机制**：`USBD_CMPSIT` 这个「容器类」把前
> 9 个回调全填 `NULL`、只填 4 个描述符回调，正是利用了这个「可为 NULL」的设计；
> 而 `USBD_AUDIO` / `USBD_CDC` 各自填上前 9 个。

### 6.5 低层 USB 外设控制驱动（PCD）结构

```c
{
  PCD_TypeDef             *Instance;      /* 寄存器基地址 */
  PCD_InitTypeDef         Init;           /* PCD 初始化所需参数的结构 */
  __IO uint8_t            USB_Address;    /* USB 分配的地址 */
  PCD_EPTypeDef           IN_ep[n];       /* IN 端点结构数组，n 为该产品支持的端点数，最多 16 */
  PCD_EPTypeDef           OUT_ep[n];      /* OUT 端点结构数组，同上 */
  HAL_LockTypeDef         Lock;           /* HAL 锁状态（用于同步） */
  __IO PCD_StateTypeDef   State;          /* PCD 通信状态：
                                             HAL_PCD_STATE_RESET   —— 反初始化 PCD 时
                                             HAL_PCD_STATE_READY   —— 初始化 PCD 后
                                             HAL_PCD_STATE_ERROR   —— 设备或核心初始化出错时
                                             HAL_PCD_STATE_BUSY    —— 处理某些瞬态事件时
                                             HAL_PCD_STATE_TIMEOUT —— 发生超时时 */
  __IO  uint32_t          ErrorCode;      /* PCD 错误码 */
  uint32_t                Setup[12];      /* Setup 包缓冲区 */
  PCD_LPM_StateTypeDef    LPM_State;      /* LPM 状态 */
  uint32_t                BESL;
  uint32_t                lpm_active;     /* LPM 激活状态（Link Power Management 启用/禁用） */
  uint32_t                battery_charging_active; /* 电池充电启用/禁用 */
  void                    *pData;         /* 指向上层栈句柄的指针 */
} PCD_HandleTypeDef;
```

---

## 7. USB 设备硬件抽象层与低层

### 7.1 STM32 USB 设备硬件抽象层（HAL）

与对所有 STM32 微控制器通用的 USB 设备库不同，HAL 层因 STM32 器件而异。但不同层可以
很容易地链接在一起并成功工作。对每个系列，有：

- **`stm32XXxx_hal_pcd (.c, .h)`**：XX 指 STM32 器件系列，可以是 f4、f7、h7、wb、g4
  等。这些文件包含各种回调的注册与注销，以及一组允许管理外设控制器驱动初始化、
  反初始化和数据传输的变量和函数。
- **`stm32XXxx_hal_pcd_ex (.c, .h)`**：同上命名规则。这些文件包含允许更新端点数据
  缓冲区、以及某些特性（LPM、BCD）激活与去激活的变量和函数。

> 📌 本项目用的是 `stm32l4xx_hal_pcd.c` 与 `stm32l4xx_hal_pcd_ex.c`；
> `HAL_PCDEx_SetRxFiFo()` / `HAL_PCDEx_SetTxFiFo()` 就在 `_ex` 文件里。

### 7.2 STM32 USB 设备低层（Low layer）

这一层高度依赖于 STM32 USB 硬件外设类型，因为它确保了高层（HAL、USB 设备库和应用）
与 USB 硬件外设通信、并据此控制其状态所需的各类寄存器访问。对每个系列，有：

- **`stm32XXxx_ll_usb (.c, .h)`**：包含确保访问 USB 外设硬件寄存器的各种变量和函数。

---

## 8. USB 设备应用实现

对于每个 STM32 USB 设备应用，有一组文件必须相应更新，才能与 STM32 USB 设备库、硬件
抽象层和低层正确对接：

| 文件 | 职责 |
|---|---|
| `main (.c, .h)` | 包含应用所需的主函数和变量 |
| `usb_device (.c, .h)` | **仅存在于高级应用架构**。包含所有 USB 设备初始化函数和变量。对于基础应用架构，初始化在 `main(.c, .h)` 中完成 |
| `stm32XXxx_it (.c, .h)` | 包含 USB 及其他系统中断的 IRQ 处理函数 |
| `usb_desc (.c, .h)` | 包含描述符定义及相关函数。取决于所选的类 |
| `usbd_conf (.c, .h)` | 包含 GPIO 和低层句柄初始化，以及 USB 回调定义。取决于硬件 |

在应用文件（可以是 `main.c` 或 `usb_device.c`）中，USB 设备的初始化主要通过三个主要
函数完成：

- **`USBD_Init()`**：初始化设备栈并加载类驱动。
- **`USBD_RegisterClass()`**：把类驱动链接到设备核心。
- **`USBD_Start()`**：允许用户启动 USB 设备核心。

对于某些需要 USB 与其他外设对接的类，还需要调用
**`USBD_CLASS_RegisterInterface(...)`**（CLASS 可以是 AUDIO、DFU、CDC、MSC）来添加
接口回调。

> ⚠️ **本项目偏离了上述标准做法，原因如下**：
>
> 1. 原文这三步对应本项目的阶段 1、2、6；但本项目多了
>    `USBD_RegisterClassComposite()` × 2（复合设备专用），且顺序在
>    `RegisterClass` 之后、`Start` 之前。
> 2. 本项目**故意没有**使用 `USBD_AUDIO_RegisterInterface()` /
>    `USBD_CDC_RegisterInterface()`，改为在
>    [`usb_device.c`](USB_DEVICE/App/usb_device.c) 中手动
>    `pUserData[0] = &USBD_AUDIO_fops_FS; pUserData[1] = &USBD_CDC_fops_FS;`。
>
>    原因是那两个 API 内部用 `pdev->pUserData[pdev->classId] = fops;` 存表，
>    而 `USBD_RegisterClassComposite()` 每次都会 `classId++`。等第二个类注册完，
>    `classId` 已经指向越界位置，两个回调表都会落到错误槽位，
>    `SET_CONFIGURATION` 阶段直接踩空指针。
>
>    详细分析见 [usb_composite_vcp.md](usb_composite_vcp.md) 与代码注释。

---

## 9. USB 设备组件组织

与其他 STM32Cube firmware 中间件一样，USB 设备应用的实现需要不同驱动层之间的交互：

- **应用层**：包含与所需 USB 设备功能相关的文件，因应用而异。其中一部分只依赖类，
  另一部分包含依赖于 STM32 USB 硬件外设的配置文件。
- **STM32 USB 设备库**：包含对所有 STM32 USB 类和应用通用的 API 的 "Core"，以及
  包含所有 STM32 USB 设备支持类相关 API 的 "Classes"。
- **硬件抽象层和低层驱动**：确保与 USB 硬件外设的交互。这些驱动与类无关，只依赖
  USB 硬件外设类型。

映射到本项目的目录：

```
应用层          USB_DEVICE/App/        usb_device.c  usbd_cdc_if.c  usbd_audio_if.c  usbd_desc.c
                USB_DEVICE/Target/     usbd_conf.c（HAL 绑定 + FIFO 分配）

USB 设备库      Middlewares/ST/STM32_USB_Device_Library/Core/      usbd_core.c  usbd_ctlreq.c  usbd_ioreq.c
                Middlewares/ST/STM32_USB_Device_Library/Class/CDC/ 官方 CDC 类
                Middlewares/Custom/USB_Audio/                     本项目自维护的 Audio 类

HAL             Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pcd.c
                Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pcd_ex.c

低层            Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_usb.c
```

---

## 10. 与本项目的对照

| 官方文档说法 | 本项目实际情况 |
|---|---|
| 初始化三步：`USBD_Init` → `USBD_RegisterClass` → `USBD_Start` | 多出 `USBD_RegisterClassComposite()` × 2，顺序在 `RegisterClass` 之后、`Start` 之前 |
| 类接口回调用 `USBD_CLASS_RegisterInterface()` | **弃用**，改为手动 `pUserData[0/1] = &fops`，否则回调落错槽位 |
| AUDIO 类「实现用于访问 USB speaker 设备」 | 官方类只有播放拓扑，因此自建 [`Middlewares/Custom/USB_Audio/`](Middlewares/Custom/USB_Audio/Src/usbd_audio.c)，用 `USBD_AUDIO_CAPTURE_DESC=1` 切到麦克风采集拓扑 |
| 端点零用于控制传输 | EP0 走控制传输；Audio 等时 IN `0x81`、CDC 三端点 `0x01/0x82/0x83` 均为协商后配置 |
| 「主机无法与缺少必需描述符的设备继续通信」 | 复合设备的配置描述符由 CompositeBuilder 运行时拼装；缺 IAD 或拓扑方向错误都会导致 Windows 报代码 10 |

---

## 11. 参考来源

- [Introduction to USB with STM32 — ST Wiki（原始页面，`USB_overview` 重定向至此）](https://wiki.st.com/stm32mcu/wiki/Introduction_to_USB_with_STM32)
- [同页面中文镜像（wiki.stmicroelectronics.cn）](https://wiki.stmicroelectronics.cn/stm32mcu/wiki/Introduction_to_USB_with_STM32)
- [USB IF 已定义类代码列表](https://www.usb.org/defined-class-codes)
- [AN4879 — Introduction to USB hardware and PCB guidelines using STM32 MCUs](https://www.st.com/content/st_com/en/technical-documents/an4879-introduction-to-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.html)
  （补充参考：给出 STM32L4 系列 USB 支持列表，其中注明 STM32L4x1 不含 USB）

---

## 12. 未翻译的章节

原页面还有以下章节，因与本项目（纯 Device 侧）无关而未译。如需其中任何一节，
可以再补。

- **Video related to STM32 USB**（培训视频链接，仅有链接无正文）
- **USB Host library overview** 整章 —— 含 USB Host Core（`usbh_core` /
  `usbh_ctlreq` / `usbh_ioreq` / `usbh_pipes` / `usbh_conf_template` / `usbh_def`）
  及 Host 侧 HID、MSC、CDC、AUDIO、MTP 各类的函数表
- **USB Device Classes** 各具体类的函数表格（HID、CustomHID、MSC、CDC、CDC ECM、
  CDC RNDIS、AUDIO、Billboard、DFU、Template）

> 其中 **AUDIO 设备类**与 **CDC 设备类**两节与本项目直接相关，如需要可告知补译。

---

*本文档为 ST 官方 Wiki 页面的中文翻译，供本项目内部参考。原文版权归 STMicroelectronics 所有。*
