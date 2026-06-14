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

set(USER_SOURCES_APP
    BSP/bsp_flash.c
    APP/esp8266_wifi.c
    APP/dc_control.c
    App/storage.c
    App/app_main.c
    # Test
    # Test/test_bsp_flash.c
)

# 合并所有用户源文件
set(USER_SOURCES ${USER_SOURCES_FREEMASTER} ${USER_SOURCES_LVGL} ${LVGL_CORE_SOURCES} ${USER_SOURCES_APP})

# 用户 include 路径
set(USER_INCLUDES
    APP
    BSP
    Test
    Test/unity
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
)

# 用户编译定义
set(USER_COMPILE_DEFINITIONS
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
    # RUN_FLASH_TESTS
)

# 用户链接库
set(USER_LINK_LIBRARIES
    # lvgl 已作为源文件直接加入编译，无需再作为库链接
)