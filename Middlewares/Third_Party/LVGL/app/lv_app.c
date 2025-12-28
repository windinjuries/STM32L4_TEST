#include "lvgl.h"
#include "string.h"
#include "stdio.h"

lv_obj_t *label_string;
lv_obj_t *label_time;
char time_string[128];
unsigned char hours = 0;
unsigned char minutes = 0;
unsigned char seconds = 0;

static void data_refresh_cb(lv_timer_t * timer)
{
    seconds++;
    if(seconds >= 60)
    {
        seconds = 0;
        minutes++;
        if(minutes >= 60)
        {
            minutes = 0;
            hours++;
            if(hours >= 24)
            {
                hours = 0;
            }
        }
    }
    snprintf(time_string, sizeof(time_string), "%02d:%02d:%02d", hours, minutes, seconds);
    lv_label_set_text(label_time, time_string);
}

void lv_user_app(void)
{
    label_string = lv_label_create(lv_scr_act());
		label_time = lv_label_create(lv_scr_act());
    lv_label_set_text(label_string, "Hello World!");
    lv_label_set_text(label_time, "00:00:00");
    lv_obj_align(label_string, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_align(label_time, LV_ALIGN_CENTER, 0, 0);
    lv_timer_create(data_refresh_cb, 1000, NULL);
    return;
}


