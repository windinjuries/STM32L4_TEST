#ifndef __APP_MAIN_H
#define __APP_MAIN_H

void app_init();

void app_task_init();

void app_timer_it_callback();
void modbus_task(void const *argument);
#endif