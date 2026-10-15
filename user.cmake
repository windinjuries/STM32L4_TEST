# user.cmake - 定义用户变量
#
# ============================================================================
# USB 栈说明（重要）
# ----------------------------------------------------------------------------
# USB 已从 TEST.ioc 中移除，CubeMX 不再生成任何 USB 代码。以下三处由本仓库
# 自行维护，全部通过本文件挂载到构建系统：
#
#   1. Middlewares/ST/STM32_USB_Device_Library/   官方 ST USB 设备库（零修改）
#   2. Middlewares/Custom/USB_Audio/              自定义 ES8388 麦克风 Audio 类
#   3. USB_DEVICE/                                USB 应用层与底层驱动
#
# 注意：官方库自带 Class/AUDIO（USB Speaker/播放方向，无麦克风能力），本工程
#       不使用它，A4 处的 FILTER 将其排除，Audio 类由自定义实现提供。
# ============================================================================

# ---------------------------------------------------------------------------
# 官方 USB 设备库根目录与源码（递归收集，便于日后追加类；未激活的类由
# CompositeBuilder 的 #if USBD_CMPSIT_ACTIVATE_* 守卫自行排除编译）
# ---------------------------------------------------------------------------
set(USB_DEVICE_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library)

# Only the modules that are actually compiled are collected here:
#   Core              - USBD core
#   CompositeBuilder  - descriptor/endpoint assembler (uses USBD_CMPSIT_ACTIVATE_*)
#   CDC               - virtual COM port (activated below)
#
# The vendor library ships ten more classes (HID, MSC, DFU, VIDEO, CCID, MTP,
# Printer, BillBoard, CDC_ECM, CDC_RNDIS). They are all guarded by
# USBD_CMPSIT_ACTIVATE_* and compile to nothing, but collecting their sources
# here also requires adding each one's Inc directory, otherwise they fail with
# "usbd_xxx.h: No such file or directory". They are therefore left out; to add
# one later, append its Src and Inc paths here and set its ACTIVATE macro in
# USB_COMPOSITE_DEFINITIONS.
file(GLOB_RECURSE USB_DEVICE_LIBRARY_SOURCES
    ${USB_DEVICE_LIBRARY_DIR}/Core/Src/*.c
    ${USB_DEVICE_LIBRARY_DIR}/Class/CompositeBuilder/Src/*.c
    ${USB_DEVICE_LIBRARY_DIR}/Class/CDC/Src/*.c
)

# ST ships every class with example/porting scaffolding named *_template.c
# (usbd_conf_template.c, usbd_desc_template.c, usbd_cdc_if_template.c, ...).
# They are NOT part of the library build: each one references project specific
# functions (e.g. hUsbDeviceFS, USBD_CDC_Template_fops) and would either fail
# to link or collide with the real implementation. Drop them all.
list(FILTER USB_DEVICE_LIBRARY_SOURCES EXCLUDE REGEX "_template\\.c$")

# 官方库包含路径（Class/AUDIO 不含在内：使用自定义实现）
set(USB_DEVICE_LIBRARY_INCLUDES
    ${USB_DEVICE_LIBRARY_DIR}/Core/Inc
    ${USB_DEVICE_LIBRARY_DIR}/Class/CompositeBuilder/Inc
    ${USB_DEVICE_LIBRARY_DIR}/Class/CDC/Inc
)

# ---------------------------------------------------------------------------
# USB 应用层与底层驱动（原由 CubeMX 生成，现由本仓库维护）
# ---------------------------------------------------------------------------
set(USB_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE)
set(USB_APP_SOURCES
    ${USB_APP_DIR}/Target/usbd_conf.c
    ${USB_APP_DIR}/App/usb_device.c
    ${USB_APP_DIR}/App/usbd_desc.c
    ${USB_APP_DIR}/App/usbd_audio_if.c
    ${USB_APP_DIR}/App/usbd_cdc_if.c
)
# Guard against ever pulling in an ST *_template.c sample through this list.
list(FILTER USB_APP_SOURCES EXCLUDE REGEX "_template\\.c$")

file(GLOB_RECURSE LVGL_CORE_SOURCES
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/core/*.c
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/draw/*.c
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/draw/sw/*.c
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/hal/*.c
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/misc/*.c
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/widgets/*.c
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/extra/*.c
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/font/*.c
)

set(LWIP_DIR Middlewares/Third_Party/lwip-STABLE-2_2_1_RELEASE)
set(IOLIB_DIR Middlewares/Third_Party/ioLibrary_Driver-3.2.0)

set(USER_SOURCES_LWIP_CORE
    ${LWIP_DIR}/src/core/init.c
    ${LWIP_DIR}/src/core/def.c
    ${LWIP_DIR}/src/core/dns.c
    ${LWIP_DIR}/src/core/inet_chksum.c
    ${LWIP_DIR}/src/core/ip.c
    ${LWIP_DIR}/src/core/mem.c
    ${LWIP_DIR}/src/core/memp.c
    ${LWIP_DIR}/src/core/netif.c
    ${LWIP_DIR}/src/core/pbuf.c
    ${LWIP_DIR}/src/core/raw.c
    ${LWIP_DIR}/src/core/stats.c
    ${LWIP_DIR}/src/core/sys.c
    ${LWIP_DIR}/src/core/altcp.c
    ${LWIP_DIR}/src/core/altcp_alloc.c
    ${LWIP_DIR}/src/core/altcp_tcp.c
    ${LWIP_DIR}/src/core/tcp.c
    ${LWIP_DIR}/src/core/tcp_in.c
    ${LWIP_DIR}/src/core/tcp_out.c
    ${LWIP_DIR}/src/core/timeouts.c
    ${LWIP_DIR}/src/core/udp.c
    ${LWIP_DIR}/src/core/ipv4/acd.c
    ${LWIP_DIR}/src/core/ipv4/autoip.c
    ${LWIP_DIR}/src/core/ipv4/dhcp.c
    ${LWIP_DIR}/src/core/ipv4/etharp.c
    ${LWIP_DIR}/src/core/ipv4/icmp.c
    ${LWIP_DIR}/src/core/ipv4/ip4_frag.c
    ${LWIP_DIR}/src/core/ipv4/ip4.c
    ${LWIP_DIR}/src/core/ipv4/ip4_addr.c
    ${LWIP_DIR}/src/api/api_lib.c
    ${LWIP_DIR}/src/api/api_msg.c
    ${LWIP_DIR}/src/api/err.c
    ${LWIP_DIR}/src/api/if_api.c
    ${LWIP_DIR}/src/api/netbuf.c
    ${LWIP_DIR}/src/api/netdb.c
    ${LWIP_DIR}/src/api/netifapi.c
    ${LWIP_DIR}/src/api/sockets.c
    ${LWIP_DIR}/src/api/tcpip.c
    ${LWIP_DIR}/src/netif/ethernet.c
)

set(USER_SOURCES_LWIP_PORT
    Middlewares/Third_Party/LwIP/port/ethernetif.c
    Middlewares/Third_Party/LwIP/port/w5500_hw.c
    Middlewares/Third_Party/LwIP/port/ch395_hw.c
    ${LWIP_DIR}/contrib/ports/freertos/sys_arch.c
)

set(USER_SOURCES_W5500
    ${IOLIB_DIR}/Ethernet/wizchip_conf.c
    ${IOLIB_DIR}/Ethernet/W5500/w5500.c
    ${IOLIB_DIR}/Ethernet/socket.c
)

set(USER_SOURCES_CH395
    Middlewares/Third_Party/CH395/CH395SPI_HW.c
    Middlewares/Third_Party/CH395/CH395CMD.c
)

set(USER_SOURCES_FREEMASTER
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_utils.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_ures.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_tsa.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_sha.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_serial.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_scope.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_rec.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_protocol.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_pipes.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_pdbdm.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_can.c
    Middlewares/Third_Party/FreeMaster/src/common/freemaster_appcmd.c
    Middlewares/Third_Party/FreeMaster/src/port/freemaster_stm32l4_uart.c
)

set(USER_SOURCES_LVGL
    Middlewares/Third_Party/LVGL/port/lv_port_disp.c
    Middlewares/Third_Party/LVGL/port/lv_port_indev.c
    Middlewares/Third_Party/LVGL/port/drv_lcd.c
    Middlewares/Third_Party/LVGL/app/lv_gui.c
    Middlewares/Third_Party/LVGL/app/lv_demo_stress.c
    Middlewares/Third_Party/LVGL/app/lv_app.c
    Middlewares/Third_Party/LVGL/app/assets/img_home_icons.c
)

set(USER_SOURCES_SEGGER_RTT
    Middlewares/Third_Party/SeggerRTT/SEGGER_RTT.c
    Middlewares/Third_Party/SeggerRTT/SEGGER_RTT_printf.c
)

set(USER_SOURCES_APP
    BSP/bsp_flash.c
    APP/esp8266_wifi.c
    APP/dc_control.c
    APP/es8388.c
    App/storage.c
    App/app_main.c
    APP/host_system_info.c
    APP/modbus.c
    APP/net_init.c
    APP/tcp_client_demo.c
    APP/debug_log.c
)

set(USER_SOURCES ${USER_SOURCES_FREEMASTER} ${USER_SOURCES_LVGL} ${LVGL_CORE_SOURCES} ${USER_SOURCES_APP}
    ${USER_SOURCES_LWIP_CORE} ${USER_SOURCES_LWIP_PORT} ${USER_SOURCES_W5500} ${USER_SOURCES_CH395}
    ${USER_SOURCES_SEGGER_RTT})

set(USER_INCLUDES
    APP
    BSP
    Test
    Test/unity
    Middlewares/Third_Party/LwIP/port
    Middlewares/Third_Party/LwIP/port/arch
    ${LWIP_DIR}/src/include
    ${LWIP_DIR}/contrib
    ${LWIP_DIR}/contrib/ports/freertos/include
    ${IOLIB_DIR}/Ethernet
    ${IOLIB_DIR}/Ethernet/W5500
    Middlewares/Third_Party/FreeMaster/src/common
    Middlewares/Third_Party/FreeMaster/src/port
    Middlewares/Third_Party/FreeMaster/src/platforms/gen32le
    Middlewares/Third_Party/LVGL/port
    Middlewares/Third_Party/LVGL/app
    Middlewares/Third_Party/LVGL/app/assets
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/core
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/draw
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/draw/sw
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/extra
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/font
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/hal
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/misc
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src/widgets
    Middlewares/Third_Party/CH395
    Middlewares/Third_Party/SeggerRTT
)

# ---------------------------------------------------------------------------
# USB 复合设备编译定义
# 必须同时作用于两个目标：
#   - 主工程 target（usb_device.c / usbd_cdc_if.c / usbd_audio_if.c）
#   - USB_Device_Library OBJECT 库（官方 Core + CompositeBuilder + CDC +
#     自定义 Audio），它不会继承主 target 的 USER_COMPILE_DEFINITIONS
# ---------------------------------------------------------------------------
set(USB_COMPOSITE_DEFINITIONS
    # USE_USBD_COMPOSITE is mandatory: without it USBD_MAX_SUPPORTED_CLASS is 1
    # and the Audio and CDC class data would share pClassDataCmsit[0].
    USE_USBD_COMPOSITE
    # Classes assembled by the official CompositeBuilder. Only these two are
    # compiled into the descriptor builder; every other class stays out.
    USBD_CMPSIT_ACTIVATE_AUDIO=1
    USBD_CMPSIT_ACTIVATE_CDC=1
    # The CompositeBuilder emits a USB Speaker (playback) Audio descriptor by
    # default. This product is a microphone, so select the capture topology:
    # Microphone input terminal 0x0201 + USB Streaming output terminal 0x0302,
    # 2 channels. Without it the host configures a playback function around an
    # isochronous IN endpoint and the device fails to start (Code 10).
    USBD_AUDIO_CAPTURE_DESC=1
    # Interface Association Descriptors.
    # The device descriptor reports bDeviceClass = 0x00 (class defined at the
    # interface level) and the configuration exposes more than one interface,
    # so each function (Audio = IF0/IF1, CDC = IF2/IF3) must be grouped by an
    # IAD or the host cannot bind a driver to the function and the device fails
    # to start. The CompositeBuilder only emits IADs when this is defined.
    USBD_COMPOSITE_USE_IAD=1
    # The stock CDC class defaults (IN 0x81 / CMD 0x82) collide with the Audio
    # isochronous IN endpoint (0x81). Remap them; these values must stay in
    # sync with CdcEpAdd[] in USB_DEVICE/App/usb_device.c and the FIFO
    # allocation in USB_DEVICE/Target/usbd_conf.c.
    CDC_IN_EP=0x82
    CDC_OUT_EP=0x01
    CDC_CMD_EP=0x83
)

set(USER_COMPILE_DEFINITIONS
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
    ${USB_COMPOSITE_DEFINITIONS}
)

set(USER_LINK_LIBRARIES
)

set(USB_AUDIO_CUSTOM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Custom/USB_Audio)
set(USB_AUDIO_CUSTOM_INC ${USB_AUDIO_CUSTOM_DIR}/Inc)
set(USB_AUDIO_CUSTOM_SRC ${USB_AUDIO_CUSTOM_DIR}/Src/usbd_audio.c)

# ---------------------------------------------------------------------------
# 官方 USB 设备库：以本仓库自维护的目录树替换 CubeMX 生成的文件列表
# ---------------------------------------------------------------------------
if(TARGET USB_Device_Library)
    # 排除官方 AUDIO 类：Audio 由自定义实现提供（避免重复符号与头文件歧义）
    list(FILTER USB_DEVICE_LIBRARY_SOURCES EXCLUDE REGEX "Class/AUDIO/Src/usbd_audio\\.c$")
    # 追加本项目自定义 Audio 类
    list(APPEND USB_DEVICE_LIBRARY_SOURCES ${USB_AUDIO_CUSTOM_SRC})

    set_property(TARGET USB_Device_Library PROPERTY SOURCES ${USB_DEVICE_LIBRARY_SOURCES})
    target_compile_definitions(USB_Device_Library PRIVATE ${USB_COMPOSITE_DEFINITIONS})
    target_include_directories(USB_Device_Library PUBLIC ${USB_DEVICE_LIBRARY_INCLUDES})
    target_include_directories(USB_Device_Library BEFORE PRIVATE ${USB_AUDIO_CUSTOM_INC})
endif()

# ---------------------------------------------------------------------------
# 主工程：USB 应用层文件改由本仓库提供（CubeMX 不再生成 usb_device.c 等）
# ---------------------------------------------------------------------------
get_target_property(_application_sources ${CMAKE_PROJECT_NAME} SOURCES)
if(_application_sources)
    list(FILTER _application_sources EXCLUDE REGEX "USB_DEVICE/(App|Target)/usbd_(conf|desc|audio_if)\\.c$")
    list(FILTER _application_sources EXCLUDE REGEX "USB_DEVICE/App/usb_device\\.c$")
    list(APPEND _application_sources ${USB_APP_SOURCES})
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY SOURCES ${_application_sources})
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${USB_DEVICE_LIBRARY_INCLUDES})
target_include_directories(stm32cubemx BEFORE INTERFACE ${USB_AUDIO_CUSTOM_INC})

# ---------------------------------------------------------------------------
# 从 CubeMX 生成的包含路径中剔除 Class/AUDIO/Inc。
# MX_Include_Dirs 仍在项目内列出该目录，而其中的 usbd_audio.h 是官方 Speaker
# (播放方向) 头文件，不含 USBD_AUDIO_GetEpPcktSze 等复合所需声明。若让它遮蔽
# 自定义头文件，CompositeBuilder 会因找不到该符号而链接失败。
# 本仓库不使用官方 AUDIO 类，因此直接移除该路径（CubeMX 下次生成 .ioc 时
# 也会自行去掉；此处保证在过渡期内同样正确）。
# ---------------------------------------------------------------------------
if(TARGET stm32cubemx)
    get_target_property(_cubemx_incs stm32cubemx INTERFACE_INCLUDE_DIRECTORIES)
    if(_cubemx_incs)
        set(_cubemx_incs_filtered "")
        foreach(_inc IN LISTS _cubemx_incs)
            if(_inc MATCHES "STM32_USB_Device_Library/Class/AUDIO/Inc")
                message(STATUS "user.cmake: dropping official AUDIO include dir: ${_inc}")
            else()
                list(APPEND _cubemx_incs_filtered ${_inc})
            endif()
        endforeach()
        set_property(TARGET stm32cubemx PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${_cubemx_incs_filtered})
    endif()
endif()
set(USER_INCLUDES Middlewares/Custom/USB_Audio/Inc USB_DEVICE/App USB_DEVICE/Target ${USER_INCLUDES})
