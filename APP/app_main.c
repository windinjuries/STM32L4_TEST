#include "storage.h"
#include "config.h"
#include "cmsis_os.h"

#if (CONFIG_USE_FREEMASTER == 1)
#include "freemaster.h"
#else
#include "debug_log.h"
#endif

#include "dc_control.h"
#include "lv_gui.h"
#include "esp8266_wifi.h"
#include "bsp_flash.h"
#include "modbus.h"
#include "app_main.h"
#include "stm32l4xx_hal.h"

osThreadId tc214bTaskHandle;
uint32_t tc214bTaskBuffer[ 256 ];
osStaticThreadDef_t tc214bTaskControlBlock;

osThreadId lvglTaskHandle;
uint32_t lvglTaskBuffer[ 2048 ];
osStaticThreadDef_t lvglTaskControlBlock;

osThreadId wifiTaskHandle;
uint32_t wifiTaskBuffer[ 512 ];
osStaticThreadDef_t wifiTaskControlBlock;

osThreadId modbusTaskHandle;
uint32_t modbusTaskBuffer[ 256 ];
osStaticThreadDef_t modbusTaskControlBlock;

osThreadId netTaskHandle;
uint32_t netTaskBuffer[ 512 ];
osStaticThreadDef_t netTaskControlBlock;

void app_init()
{
#if (CONFIG_USE_FREEMASTER == 1)
    FMSTR_Init();
#else
    log_init();
#endif
    param_storage_init();
    modbus_init();
#if (CONFIG_USE_FREEMASTER == 0)
    LOG_INFO("System initialized, starting tasks...");
#endif

    extern TIM_HandleTypeDef htim6;
    HAL_TIM_Base_Start_IT(&htim6);

}

void app_task_init()
{
    osThreadStaticDef(tc214bTask, StartTC214BTask, osPriorityNormal, 0, 256, tc214bTaskBuffer, &tc214bTaskControlBlock);
    tc214bTaskHandle = osThreadCreate(osThread(tc214bTask), NULL);
	
    osThreadStaticDef(wifiTask, wifi_task, osPriorityNormal, 0, 512, wifiTaskBuffer, &wifiTaskControlBlock);
    wifiTaskHandle = osThreadCreate(osThread(wifiTask), NULL);

#if (CONFIG_MODULE_LVGL_ENABLE == 1)
    osThreadStaticDef(lvglTask, lvgl_gui_task, osPriorityNormal, 0, 2048, lvglTaskBuffer, &lvglTaskControlBlock);
    lvglTaskHandle = osThreadCreate(osThread(lvglTask), NULL);
#endif

    osThreadStaticDef(modbusTask, modbus_task, osPriorityNormal, 0, 256, modbusTaskBuffer, &modbusTaskControlBlock);
    modbusTaskHandle = osThreadCreate(osThread(modbusTask), NULL);

    osThreadStaticDef(netTask, net_task, osPriorityNormal, 0, 512, netTaskBuffer, &netTaskControlBlock);
    netTaskHandle = osThreadCreate(osThread(netTask), NULL);

}

void app_timer_it_callback()
{
#if (CONFIG_MODULE_LVGL_ENABLE == 1) 
    extern void lv_tick_inc(uint32_t tick_period);
    lv_tick_inc(1);
#endif
}
