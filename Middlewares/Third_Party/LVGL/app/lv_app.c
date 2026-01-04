#include "lvgl.h"
#include "string.h"
#include "stdio.h"
#include "main.h"  // For RTC_HandleTypeDef

extern RTC_HandleTypeDef hrtc;  // RTC handle from main.c

lv_obj_t *label_date;
lv_obj_t *label_time;
char date_string[32];
char time_string[32];

static void rtc_refresh_cb(lv_timer_t * timer)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    
    // Get current time from RTC
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    // Format date string: YYYY-MM-DD
    snprintf(date_string, sizeof(date_string), "%04d-%02d-%02d", 
             2000 + sDate.Year, sDate.Month, sDate.Date);
    lv_label_set_text(label_date, date_string);
    
    // Format time string: HH:MM:SS
    snprintf(time_string, sizeof(time_string), "%02d:%02d:%02d", 
             sTime.Hours, sTime.Minutes, sTime.Seconds);
    lv_label_set_text(label_time, time_string);
}

void lv_user_app(void)
{
    // Create date label
    label_date = lv_label_create(lv_scr_act());
    lv_label_set_text(label_date, "0000-00-00");
    lv_obj_align(label_date, LV_ALIGN_TOP_LEFT, 10, 10);
    
    // Create time label
    label_time = lv_label_create(lv_scr_act());
    lv_label_set_text(label_time, "00:00:00");
    lv_obj_align(label_time, LV_ALIGN_TOP_LEFT, 10, 40);  // Below date label
    
    // Create timer to refresh every second
    lv_timer_create(rtc_refresh_cb, 1000, NULL);
    
    return;
}


