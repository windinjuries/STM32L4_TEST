# user.cmake - 定义用户变量

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
)

set(USER_SOURCES_SEGGER_RTT
    Middlewares/Third_Party/SeggerRTT/SEGGER_RTT.c
    Middlewares/Third_Party/SeggerRTT/SEGGER_RTT_printf.c
)

set(USER_SOURCES_APP
    BSP/bsp_flash.c
    APP/esp8266_wifi.c
    APP/dc_control.c
    App/storage.c
    App/app_main.c
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

set(USER_COMPILE_DEFINITIONS
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
)

set(USER_LINK_LIBRARIES
)
