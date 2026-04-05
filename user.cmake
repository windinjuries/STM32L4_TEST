# user.cmake - 定义用户变量

# FreeMaster 源文件列表
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

# LVGL 源文件列表
set(USER_SOURCES_LVGL
    Middlewares/Third_Party/LVGL/port/lv_port_disp.c
    Middlewares/Third_Party/LVGL/port/drv_lcd.c
    Middlewares/Third_Party/LVGL/app/lv_gui.c
    Middlewares/Third_Party/LVGL/app/lv_demo_stress.c
    Middlewares/Third_Party/LVGL/app/lv_app.c
    Core/Src/esp8266_wifi.c
    Core/Src/dc_control.c
)

set(USER_SOURCES_APP
    Core/Src/esp8266_wifi.c
    Core/Src/dc_control.c

)

# 合并所有用户源文件
set(USER_SOURCES ${USER_SOURCES_FREEMASTER} ${USER_SOURCES_APP})

# 用户 include 路径
set(USER_INCLUDES
    Middlewares/Third_Party/FreeMaster/src/common
    Middlewares/Third_Party/FreeMaster/src/port
    Middlewares/Third_Party/FreeMaster/src/platforms/gen32le
    Middlewares/Third_Party/LVGL/port
    Middlewares/Third_Party/LVGL/lvgl-release-v8.3/src
)

# 用户编译定义
set(USER_COMPILE_DEFINITIONS
    LV_CONF_INCLUDE_SIMPLE
)

# 用户链接库
set(USER_LINK_LIBRARIES
    lvgl
)