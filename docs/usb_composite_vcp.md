# USB 复合设备（Audio + CDC 虚拟串口）改造说明与验证步骤

本文档对应 USB 栈重构：从 CubeMX 手中接管 USB，改用官方完整 ST USB 设备库 +
官方 CompositeBuilder 组装「USB 麦克风 + 虚拟串口」复合设备。

---

## 1. 改造总览

| 项目 | 改造前 | 改造后 |
|---|---|---|
| USB 代码来源 | CubeMX 生成 | 本仓库自维护 |
| USB 类 | Audio（自定义麦克风） | Audio（自定义麦克风）+ CDC（虚拟串口） |
| 复合机制 | 无（单功能） | 官方 `CompositeBuilder`，运行时拼装描述符 |
| `.ioc` 中的 USB | `USB_DEVICE` + `USB_OTG_FS` | 已全部移除 |
| PID | `0x5740` | `0x5745`（避免 Windows 复用旧设备缓存） |

### 目录职责

```
Middlewares/ST/STM32_USB_Device_Library/   官方 ST USB 库（仅 CompositeBuilder 的
                                           Audio 描述符按本产品改造，见第 9 节）
Middlewares/Custom/USB_Audio/              自定义 ES8388 麦克风 Audio 类（本仓库维护）
USB_DEVICE/                                USB 应用层与底层驱动（本仓库维护）
cmake/stm32cubemx/CMakeLists.txt           CubeMX 生成，勿手改
user.cmake                                 唯一构建挂载点（本仓库维护）
```

---

## 2. 复合设备接口与端点分配

构建器在运行时按注册顺序自动分配接口号与拼装配置描述符：

| classId | 功能 | 接口 | 端点 | 类型 |
|---|---|---|---|---|
| 0 | Audio（麦克风） | 0 = AudioControl<br>1 = AudioStreaming (alt 0/1) | `0x81` | Isochronous IN |
| 1 | CDC（虚拟串口） | 2 = CDC Control<br>3 = CDC Data | `0x83`<br>`0x01`<br>`0x82` | Interrupt IN<br>Bulk OUT<br>Bulk IN |

端点地址不是默认值：官方 CDC 默认 `IN=0x81`、`CMD=0x82`，与 Audio 的 `0x81`
冲突，因此通过 `user.cmake` 的编译定义重映射：

```cmake
CDC_IN_EP=0x82
CDC_OUT_EP=0x01
CDC_CMD_EP=0x83
```

> ⚠️ 这三个值必须与 `USB_DEVICE/App/usb_device.c` 的 `CdcEpAdd[]` 以及
> `USB_DEVICE/Target/usbd_conf.c` 的 FIFO 分配保持一致，改动其一时需同步其余两处。

---

## 3. USB OTG FS FIFO 分配

`usbd_conf.c` 的 `HAL_PCDEx_SetTxFiFo()/SetRxFiFo()` 参数单位是 **32 位字数**
（该值被直接写入 FIFO 深度寄存器；OTG 内核对每个 TX FIFO 要求最少 16 字）。

| FIFO | 字 | 字节 | 用途 |
|---|---|---|---|
| RX | 128 | 512 | CDC bulk OUT、控制 SETUP |
| TX 0 | 64 | 256 | EP0 / 非周期 IN |
| TX 1 | 64 | 256 | EP1 IN：Audio 等时 IN（193 B/帧） |
| TX 2 | 16 | 64 | EP2 IN：CDC bulk IN |
| TX 3 | 16 | 64 | EP3 IN：CDC interrupt IN（8 B） |
| **合计** | **288** | 1152 | 预算 320 字（1280 B） |

超出 320 字会破坏外设 FIFO 配置，因此合计必须 ≤ 320。

---

## 4. 虚拟串口 API

`usbd_cdc_if.h` 对外提供：

```c
uint8_t  VCP_Write(const uint8_t *Buf, uint16_t Len);  /* 0=成功 1=忙 2=参数错 */
uint32_t VCP_Available(void);                          /* 可读字节数 */
uint32_t VCP_Read(uint8_t *Buf, uint32_t Len);         /* 返回实际读取字节数 */
uint8_t  VCP_IsConnected(void);                        /* 主机是否已打开端口(DTR) */
```

- 接收：USB 中断里把数据写入 512 B 环形缓冲（`VCP_RX_RING_SIZE`，需为 2 的幂），
  应用侧轮询 `VCP_Available()` / `VCP_Read()`。缓冲满时丢弃并累加 `vcp_rx_ovf`。
- 发送：`VCP_Write()` 非阻塞，忙时返回 1，调用方需自行重试。
  单次最多 `CDC_DATA_FS_MAX_PACKET_SIZE`（64）字节。

---

## 5. 编译验证

本仓库提供 `Tool/build.py`，直接调用 STM32Cube 捆绑的工具链
（cmake 4.3.1 + ninja 1.13.2 + arm-none-eabi-gcc 14.3.1），不依赖 pwsh。

```powershell
cd D:\folder\code\STM32L4_TEST
python Tool\build.py
```

首次配置时**应当**看到这一行（说明包含路径过滤生效）：

```
-- user.cmake: dropping official AUDIO include dir: .../Class/AUDIO/Inc
```

### 已知陷阱（本次改造实际踩到并已修复）

| 报错 / 现象 | 真实原因 | 修法 |
|---|---|---|
| 枚举成 **USB Composite Device** 但"**该设备无法启动 (代码 10)**" | **官方 CompositeBuilder 的 Audio 描述符是播放（喇叭）拓扑**：Input Terminal `0x0101`(USB Streaming)、Output Terminal `0x0301`(Speaker)、`bNrChannels=1`。而本产品是录音，描述符与等时 **IN** 端点方向矛盾，主机按播放设备配置后启动失败 | 定义 `USBD_AUDIO_CAPTURE_DESC`，改由构建器输出完整的麦克风拓扑（见第 9 节）。**只改字段值是不够的**（早期版本把 Output Terminal 改成 `0x0302`、`bTerminalLink` 仍指输入终端，仍报代码 10） |
| 设备管理器显示 **STM32 microphone**，音频功能"**该设备无法启动 (代码 10)** / **无法找到对象**"，同时 VCP 正常 | **音频拓扑无法被主机解析**。`bTerminalLink` 指向的是 Microphone **输入**终端（`bTerminalLink=1`）、输出终端类型为 `0x0302`（Headphones，不是 USB Streaming `0x0101`），且 Feature Unit 的 `bLength=9` 与 `bNrChannels=2` 不匹配（2 声道需要 3 个 bmaControls 字节 = `bLength` 10）。主机找不到"USB 流"终端后返回 `STATUS_OBJECT_NAME_NOT_FOUND`，Windows 就显示为"代码 10 + 无法找到对象" | 按第 9 节的拓扑重建 AC 描述符：IT `0x0201` → FU（`bLength=10`，3 个控制字节）→ OT `0x0101`，`bTerminalLink=3`，`wTotalLength` 由构建器按实际长度计算（`0x28`） |
| 上板枚举时 **HardFault**，位置 `usbd_audio.c:413`（`pUserData[classId]->Init()`） | **`RegisterInterface` 注册到错误槽位**：`USBD_RegisterClassComposite()` 内部经 `USBD_CMPSIT_AddClass()` 已经把 `pdev->classId` 自增，于是随后调用的 `USBD_AUDIO_RegisterInterface()` 把 fops 写进了 `pUserData[1]` 而不是 `[0]`，`pUserData[0]` 保持 NULL。类指针 `pClass[]` 与 `pUserData[]` 因此整体错位一格（`pClass[1]=USBD_AUDIO` 但 `pUserData[1]=audio fops`） | 注册前用 `USBD_CMPSIT_GetClassID(pdev, CLASS_TYPE_x, 0)` 取出真实索引并赋给 `pdev->classId`（见 `usb_device.c`） |
| 上板枚举时 **HardFault**，位置同上 | `USBD_static_malloc()` 是 CubeMX 的单类版本：**忽略 `size`、永远返回同一块缓冲**，`USBD_static_free()` 为空。两个类各自 `USBD_malloc()` 时拿到**同一地址**，后初始化的 CDC 用 `memset` 把 Audio 句柄开头清零 | 改为**顺序分配（bump allocator）**，池大小 `sizeof(USBD_AUDIO_HandleTypeDef) + 640`，并在 `USBD_static_free()` 中回退指针。**新增任何 USB 类都必须重新核对此池容量** |
| 8 个类报 `usbd_xxx.h: No such file or directory`（BillBoard/CCID/CDC_ECM/CDC_RNDIS/DFU/CustomHID/HID/MSC） | 源文件通配用了 `Class/*/Src/*.c` 把所有类都收进来，但只添加了 Core/CompositeBuilder/CDC 的包含路径 | 只收集实际编译的模块（见 `user.cmake` 的 `USB_DEVICE_LIBRARY_SOURCES`）。**追加新类时必须同时补 `Src` 与 `Inc` 两处** |
| `undefined reference to 'USBD_CDC_fops_FS'` | 新增的 `usbd_cdc_if.c` 未加入 `USB_APP_SOURCES`（CubeMX 生成的清单里当然没有它） | 已加入；今后新增应用层 `.c` 都要在此登记 |
| 重复定义 `USBD_CDC_Init` / `hUsbDeviceFS` | ST 每个类都带 `*_template.c` 示例文件，被通配收进来编译 | `user.cmake` 中的 `_template\.c$` 排除规则 |

### 其余可能的报错与含义

| 报错 | 含义 | 处理 |
|---|---|---|
| `USBD_AUDIO_GetEpPcktSze` undefined | `usbd_audio.h` 解析到了官方 Speaker 版而非自定义版 | 确认配置时出现 `dropping official AUDIO include dir` 那行；检查 `user.cmake` 包含路径过滤段 |
| `USBD_MAX_SUPPORTED_CLASS` 相关 / 数组越界 | `USE_USBD_COMPOSITE` 未传到 `USB_Device_Library` 目标 | 检查 `target_compile_definitions(USB_Device_Library ...)` |
| 端点相关断言 / HardFault | FIFO 合计超 320 字，或端点地址冲突 | 对照第 2、3 节表格核对 |

---

## 6. 上板验证顺序

建议分阶段，便于定位问题：

| 阶段 | 操作 | 判定标准 | 状态 |
|---|---|---|---|
| V0 | `python Tool/build.py` | 链接通过，生成 `build/Debug/TEST.elf` | ✅ **已通过** |
| V1 | 只插 USB，不开录音 | 设备管理器同时出现「麦克风」与「COM 口」 | 待上板 |
| V2 | 串口助手打开 COM 口，调用 `VCP_Write()` | PC 能收到数据；发数据能被 `VCP_Read()` 取回 | 待上板 |
| V3 | 打开录音 | 录音正常，且 COM 口仍可用（无需重插） | 待上板 |
| V4 | 并发压测 | 录音运行中 VCP 持续收发 10 分钟，`error_count` 不增长 | 待上板 |

### V0 实测结果

```
Memory region         Used Size  Region Size  %age Used
           FLASH:       83344 B       510 KB     15.96%
            RAM2:        1536 B        32 KB      4.69%
             RAM:       52728 B        96 KB     53.64%
BUILD OK
```

产物 `build/Debug/TEST.elf`，符号表已确认包含：

```
USBD_CMPSIT / USBD_CMPSIT_AddClass / USBD_CoreGetEPAdd
USBD_AUDIO  / USBD_AUDIO_fops_FS  / USBD_AUDIO_GetEpPcktSze
USBD_CDC    / USBD_CDC_fops_FS
```

（`USBD_AUDIO_GetEpPcktSze` 由自定义 Audio 类提供，见第 1 节；官方 AUDIO 类已被排除。）

---

## 7. 注意事项：不要在 CubeMX 中重新启用 USB

`TEST.ioc` 已移除 USB。若重新在 CubeMX 中启用 `USB_DEVICE` 或 `USB_OTG_FS`，
CubeMX 会：

1. 重新生成 `usb_device.c` / `usbd_conf.c` / `usbd_desc.c`，覆盖本仓库的自维护版本；
2. 在 `cmake/stm32cubemx/CMakeLists.txt` 中重新加入官方 `Class/AUDIO/Src/usbd_audio.c`
   与 `Class/AUDIO/Inc`，造成 Audio 类重复与头文件歧义；
3. 可能删除 `stm32l4xx_hal_conf.h` 中的 `HAL_PCD_MODULE_ENABLED`
   （该文件内已加保护性注释说明）。

USB 的 GPIO（PA11/PA12）、时钟、NVIC 均由 `usbd_conf.c` 的 `HAL_PCD_MspInit()`
自行配置，不依赖 CubeMX 生成代码。

---

## 8. 已知待改进项（不阻塞本次改造）

1. **`Class/AUDIO/Inc/usbd_audio.h` 与自定义同名头文件并存**：当前依靠包含路径
   优先级（`target_include_directories(... BEFORE ...)` + 剔除 CubeMX 路径）保证
   自定义版优先。如需彻底消除歧义，可移除官方 `Class/AUDIO/` 目录，代价是官方库
   不再与上游逐字节一致。
2. **采样率基准偏差**：`TEST.ioc` 记录 `SAI1.RealAudioFreq = 47.794 KHz`、
   `ErrorAudioFreq = -0.42 %`（PLLSAI2 分频无法整除出 12.288 MHz）。这会导致
   ADC 生产速率与 USB 48 kHz 消费速率长期漂移约 0.43%，是音频断续/爆音的独立
   根因，与本次复合设备改造无关，需单独处理。
3. **`USBD_AUDIO_Sync()` 的 `wr_ptr` 半/满语义**：自定义 Audio 类中 DMA 半传输
   /全传输回调与读指针的水位语义存在颠倒，同样属于既有音频链路问题，建议在
   复合设备跑通后单独修正。
4. **等时 IN 端点的同步类型声明**：描述符里 `bmAttributes = 0x01`，即
   "Isochronous / 无同步"，而实际采样时钟来自 ES8388 ADC（自由运行）并由
   `packet_accum` 做 192/193 字节的速率补偿，语义上属于**异步源**
   （`0x05`）。Windows 对麦克风一般期望 `0x05`；若后续出现"能枚举但打不开
   录音流""采样率偏移"之类问题，先把这一字节改成 `0x05` 再验证。

---

## 9. 音频（UAC1 麦克风）描述符拓扑

这是"该设备无法启动（代码 10）/ 无法找到对象"的唯一已知成因，改动此段前请先读完。

### 9.1 正确的麦克风拓扑

UAC1 里**数据流方向由终端类型表达**，而不是由端点方向表达：

| 角色 | 终端类型 | 含义 |
|---|---|---|
| 麦克风（声音来源） | Input Terminal `wTerminalType = 0x0201` | 声音从"外部"进入功能 |
| USB 主机（声音去向） | Output Terminal `wTerminalType = 0x0101` | 数据流到 USB |

所以麦克风功能是 `IT(0x0201) → FU → OT(0x0101)`，**AudioStreaming 接口的
`bTerminalLink` 必须指向那个 `0x0101` 输出终端**；扬声器功能则相反，是
`IT(0x0101) → FU → OT(0x0301 Speaker)`，`bTerminalLink` 指向输入终端。

主机解析时先按 `bTerminalLink` 找终端，再顺着每个单元的 `bSourceID` 往回走。
只要有一个 ID 解析不到，Windows 就会返回 `STATUS_OBJECT_NAME_NOT_FOUND`，
在设备管理器里显示为"**该设备无法启动。(代码 10)** + **无法找到对象**"——
**同一块板子上的其它功能（VCP）仍然完全正常**，这正是本项目的现象。

### 9.2 本工程实际输出的 AC 段（`USBD_AUDIO_CAPTURE_DESC=1`）

```
09 24 01 00 01 28 00 01 01              AC Header, wTotalLength=0x28(40), 1 AS 接口(IF1)
0C 24 02 01 01 02 00 02 03 00 00 00     Input Terminal id=1, type=0x0201, 2ch, L/R
0A 24 06 02 01 01 01 00 00 00           Feature Unit id=2, src=1, ctlSize=1, 3 个控制字节
09 24 03 03 01 01 00 02 00              Output Terminal id=3, type=0x0101, src=2
07 24 01 03 01 01 00                    AS General, bTerminalLink=3, bDelay=1, PCM
0B 24 02 01 02 02 10 01 80 BB 00        Format Type I, 2ch/16bit/48000
```

要点（每一条都曾经写错过）：

1. `bTerminalLink = 3`（输出终端），不是 `1`；
2. 输出终端类型 `0x0101`（USB Streaming），**不是** `0x0302`（Headphones）；
3. Feature Unit 的 `bLength = 7 + bControlSize × (bNrChannels + 1) = 10`，
   2 声道必须给 3 个 `bmaControls` 字节，否则主机按 `bNrChannels` 读控制字节时
   会越界读进下一条描述符，拓扑随之断裂；
4. AC Header 的 `wTotalLength` 必须等于 AC 段实际长度。构建器现在**按已写出的
   字节数计算**（不再写死 `0x0027`），`baInterfaceNr` 也取自
   `tclasslist[classId].Ifs[1]`（不再写死 `1`），这样即使以后调整类的注册顺序
   也不会指错接口。

### 9.3 上板核对描述符

1. 用 USBTreeView / USB Device Tree Viewer 抓配置描述符，或
2. 用 GDB 从运行中的固件 dump 出来：

```
arm-none-eabi-gdb build/Debug/TEST.elf
(gdb) break MX_USB_DEVICE_Init
(gdb) run
(gdb) finish                                  # 让描述符构建完成
(gdb) dump binary memory cfg.bin pCmpstFSConfDesc pCmpstFSConfDesc+184   # 184 = wTotalLength
```

3. 交给本仓库的解码器判断结构是否合法、拓扑是否可解析：

```powershell
python Tool\decode_cfgdesc.py cfg.bin
```

输出中应出现：

```
audio topology: AS links to OUTPUT terminal 3, type 0x0101 (USB Streaming) -> CAPTURE
no structural problems found
```

### 9.4 若仍然报代码 10

按以下顺序排除（都与描述符无关了）：

1. **Windows 缓存了旧描述符**：设备管理器里卸载该设备（勾选"删除驱动程序"），
   或改 `USBD_PID_FS` 后再插；USBTreeView 里核对 `idProduct` 是否已变。
2. **`bMaxPower` 超过端口可提供电流**（当前 `USBD_SELF_POWERED=1` + `USBD_MAX_POWER`），
   自供电标志与实际供电方式不符会让音频功能启动失败。
3. **等时端点带宽**：48 kHz × 2ch × 2B = 192 B/帧，端点为 193 B/帧，
   TX FIFO 1 已给 256 B；若 FIFO 分配被改动过需重新核对（见第 3 节）。
4. 若要临时验证"是否 Feature Unit 引起"，可删掉 FU 段并把 AC 段长度改成
   `0x1E`（`IT → OT` 直连，`OT.bSourceID = 1`）——这是 UAC1 规范附录 B 的
   最简麦克风拓扑，可用来二分定位。

