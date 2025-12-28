#include "lv_gui.h"
#include "cmsis_os.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_demo_stress.h"
extern void lv_user_app(void);

void lvgl_gui_task(void const * argument)
{
    lv_init();
    lv_port_disp_init();
    lv_user_app();
    /* handle the tasks of LVGL */
	while (1)
	{
		lv_task_handler();
		osDelay(30);
	}
}
