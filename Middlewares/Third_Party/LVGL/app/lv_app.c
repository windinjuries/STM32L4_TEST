/**
 * @file    lv_app.c
 * @brief   LVGL GUI Application - Multi-page menu interface with key navigation
 * @details Home page with menu, Time page (RTC), Weather page (wttr.in),
 *          and System Info page.
 *          Supports 4-button keypad: UP / DOWN / LEFT / RIGHT / ENTER (long press).
 */

#include "lvgl.h"
#include "lv_port_indev.h"
#include "string.h"
#include "stdio.h"
#include "main.h"

/* ====================================================================
 *  External symbols
 * ==================================================================== */
extern RTC_HandleTypeDef hrtc;
extern int8_t esp8266_get_weather(char *city, char *weather_buf);

/* ====================================================================
 *  Screen objects
 * ==================================================================== */
static lv_obj_t *scr_home;
static lv_obj_t *scr_time;
static lv_obj_t *scr_weather;
static lv_obj_t *scr_system;

/* ====================================================================
 *  Group objects (for keypad navigation)
 * ==================================================================== */
static lv_group_t *group_home;
static lv_group_t *group_time;
static lv_group_t *group_weather;
static lv_group_t *group_system;

/* ====================================================================
 *  Time page widgets
 * ==================================================================== */
static lv_obj_t *label_time_date;
static lv_obj_t *label_time_clock;
static lv_obj_t *label_time_weekday;
static char time_date_buf[32];
static char time_clock_buf[32];
static char time_weekday_buf[16];

/* ====================================================================
 *  Weather page widgets
 * ==================================================================== */
static lv_obj_t *label_weather_temp;
static lv_obj_t *label_weather_status;
static lv_obj_t *spinner_weather;
static char weather_buf[64];

/* ====================================================================
 *  Color palette
 * ==================================================================== */
#define COLOR_BLUE        lv_color_hex(0x1565C0)
#define COLOR_BLUE_DARK   lv_color_hex(0x0D47A1)
#define COLOR_ORANGE      lv_color_hex(0xEF6C00)
#define COLOR_GREEN       lv_color_hex(0x2E7D32)
#define COLOR_PURPLE      lv_color_hex(0x6A1B9A)
#define COLOR_WHITE       lv_color_hex(0xFFFFFF)
#define COLOR_BG          lv_color_hex(0xF0F2F5)
#define COLOR_CARD        lv_color_hex(0xFFFFFF)
#define COLOR_TEXT        lv_color_hex(0x212121)
#define COLOR_GRAY        lv_color_hex(0x757575)

/* ====================================================================
 *  Focus style — makes focused buttons clearly visible
 * ==================================================================== */
static lv_style_t style_focus;

static void style_init(void)
{
    lv_style_init(&style_focus);
    /* Bright golden border when focused */
    lv_style_set_border_color(&style_focus, lv_color_hex(0xFFD600));
    lv_style_set_border_width(&style_focus, 3);
    lv_style_set_border_opa(&style_focus, LV_OPA_COVER);
    /* Slightly darker background */
    lv_style_set_bg_color(&style_focus, lv_color_hex(0xE8EAF6));
    lv_style_set_bg_opa(&style_focus, LV_OPA_COVER);
}

/* ====================================================================
 *  Forward declarations
 * ==================================================================== */
static void create_home_page(void);
static void create_time_page(void);
static void create_weather_page(void);
static void create_system_page(void);
static void back_home_cb(lv_event_t *e);

/* ====================================================================
 *  Helper: Switch group for keypad navigation
 * ==================================================================== */
static void switch_group(lv_group_t *grp)
{
    if (grp == NULL) return;

    lv_indev_t *indev = lv_port_indev_get_keypad();
    if (indev)
    {
        lv_indev_set_group(indev, grp);
    }
    /* Set as default group so unassigned indevs also pick it up */
    lv_group_set_default(grp);

    /* Focus the first focusable object if nothing focused yet */
    if (lv_group_get_focused(grp) == NULL)
    {
        lv_group_focus_next(grp);
    }
}

/* ====================================================================
 *  Helper: Create a title bar with back button
 *  @param parent  Parent object (screen)
 *  @param title   Title text
 *  @param grp     Group to add the back button to (can be NULL)
 * ==================================================================== */
static lv_obj_t *create_title_bar(lv_obj_t *parent, const char *title,
                                   lv_group_t *grp)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 240, 38);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, COLOR_BLUE, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);

    /* Back button */
    lv_obj_t *btn_back = lv_btn_create(bar);
    lv_obj_set_size(btn_back, 44, 30);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_bg_color(btn_back, COLOR_BLUE_DARK, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_back, 4, LV_PART_MAIN);
    lv_obj_add_style(btn_back, &style_focus, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btn_back, back_home_cb, LV_EVENT_CLICKED, NULL);

    /* Add back button to group for key navigation */
    if (grp != NULL)
    {
        lv_group_add_obj(grp, btn_back);
    }

    lv_obj_t *label_arrow = lv_label_create(btn_back);
    lv_label_set_text(label_arrow, LV_SYMBOL_LEFT);
    lv_obj_center(label_arrow);
    lv_obj_set_style_text_color(label_arrow, COLOR_WHITE, LV_PART_MAIN);

    /* Title */
    lv_obj_t *label_title = lv_label_create(bar);
    lv_label_set_text(label_title, title);
    lv_obj_center(label_title);
    lv_obj_set_style_text_color(label_title, COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_20, LV_PART_MAIN);

    return bar;
}

/* ====================================================================
 *  Helper: Create a menu-style button with icon color
 *  @return The button object (for adding to group)
 * ==================================================================== */
static lv_obj_t *create_menu_btn(lv_obj_t *parent, const char *icon,
                                  const char *label_text, lv_color_t icon_color,
                                  lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 220, 44);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_add_style(btn, &style_focus, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    /* Icon circle */
    lv_obj_t *icon_circle = lv_obj_create(btn);
    lv_obj_set_size(icon_circle, 32, 32);
    lv_obj_align(icon_circle, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_radius(icon_circle, 16, LV_PART_MAIN);
    lv_obj_set_style_bg_color(icon_circle, icon_color, LV_PART_MAIN);
    lv_obj_set_style_border_width(icon_circle, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(icon_circle, 0, LV_PART_MAIN);

    lv_obj_t *icon_label = lv_label_create(icon_circle);
    lv_label_set_text(icon_label, icon);
    lv_obj_center(icon_label);
    lv_obj_set_style_text_color(icon_label, COLOR_WHITE, LV_PART_MAIN);

    /* Button text */
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 50, 0);
    lv_obj_set_style_text_color(label, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);

    /* Right arrow */
    lv_obj_t *arrow = lv_label_create(btn);
    lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(arrow, COLOR_GRAY, LV_PART_MAIN);

    return btn;
}

/* ====================================================================
 *  Back button callback
 * ==================================================================== */
static void back_home_cb(lv_event_t *e)
{
    (void)e;
    switch_group(group_home);
    lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

/* ====================================================================
 *  Menu button callbacks
 * ==================================================================== */
static void menu_time_cb(lv_event_t *e)
{
    (void)e;
    switch_group(group_time);
    lv_scr_load_anim(scr_time, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

static void menu_weather_cb(lv_event_t *e)
{
    (void)e;
    switch_group(group_weather);
    lv_scr_load_anim(scr_weather, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

static void menu_system_cb(lv_event_t *e)
{
    (void)e;
    switch_group(group_system);
    lv_scr_load_anim(scr_system, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

/* ====================================================================
 *  RTC time refresh callback (called every second)
 * ==================================================================== */
static void rtc_refresh_cb(lv_timer_t *timer)
{
    (void)timer;
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    /* Update date: YYYY-MM-DD */
    snprintf(time_date_buf, sizeof(time_date_buf), "%04d-%02d-%02d",
             2000 + sDate.Year, sDate.Month, sDate.Date);
    lv_label_set_text(label_time_date, time_date_buf);

    /* Update time: HH:MM:SS */
    snprintf(time_clock_buf, sizeof(time_clock_buf), "%02d:%02d:%02d",
             sTime.Hours, sTime.Minutes, sTime.Seconds);
    lv_label_set_text(label_time_clock, time_clock_buf);

    /* Update weekday */
    static const char *weekdays[] = {
        "Monday", "Tuesday", "Wednesday", "Thursday",
        "Friday", "Saturday", "Sunday"
    };
    uint8_t wday = sDate.WeekDay;
    if (wday >= 1 && wday <= 7)
    {
        snprintf(time_weekday_buf, sizeof(time_weekday_buf), "%s",
                 weekdays[wday - 1]);
    }
    else
    {
        snprintf(time_weekday_buf, sizeof(time_weekday_buf), "Unknown");
    }
    lv_label_set_text(label_time_weekday, time_weekday_buf);
}

/* ====================================================================
 *  Weather refresh callback (called periodically)
 *  NOTE: esp8266_get_weather() blocks ~3 seconds, so this freezes LVGL
 *        briefly.  For production, consider offloading to a separate task.
 * ==================================================================== */
static void weather_refresh_cb(lv_timer_t *timer)
{
    (void)timer;
    int8_t ret;

    /* Show spinner while loading */
    if (spinner_weather)
    {
        lv_obj_clear_flag(spinner_weather, LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text(label_weather_status, "Updating...");

    /* Fetch weather from wttr.in */
    ret = esp8266_get_weather("Beijing", weather_buf);
    if (ret == 0) /* ESP8266_EOK = 0 */
    {
        lv_label_set_text(label_weather_temp, weather_buf);
        lv_label_set_text(label_weather_status, "Updated");
    }
    else
    {
        lv_label_set_text(label_weather_temp, "--°C");
        lv_label_set_text(label_weather_status, "Failed to update");
    }

    /* Hide spinner */
    if (spinner_weather)
    {
        lv_obj_add_flag(spinner_weather, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ====================================================================
 *  Manual weather refresh button callback
 * ==================================================================== */
static void weather_manual_refresh_cb(lv_event_t *e)
{
    (void)e;
    weather_refresh_cb(NULL);
}

/* ====================================================================
 *  HOME PAGE
 * ==================================================================== */
static void create_home_page(void)
{
    lv_obj_t *btn;

    scr_home = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_home, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr_home, 0, LV_PART_MAIN);

    /* ---- Group for keypad navigation ---- */
    group_home = lv_group_create();

    /* ---- Title bar ---- */
    lv_obj_t *title_bar = lv_obj_create(scr_home);
    lv_obj_set_size(title_bar, 240, 50);
    lv_obj_align(title_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(title_bar, COLOR_BLUE, LV_PART_MAIN);
    lv_obj_set_style_pad_all(title_bar, 0, LV_PART_MAIN);

    /* App title icon + text */
    lv_obj_t *title_icon = lv_label_create(title_bar);
    lv_label_set_text(title_icon, LV_SYMBOL_HOME);
    lv_obj_align(title_icon, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_style_text_color(title_icon, COLOR_WHITE, LV_PART_MAIN);

    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "STM32L4 Menu");
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 40, 0);
    lv_obj_set_style_text_color(title_label, COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, LV_PART_MAIN);

    /* ---- Menu button container (flex layout) ---- */
    lv_obj_t *menu_cont = lv_obj_create(scr_home);
    lv_obj_set_size(menu_cont, 240, 188);
    lv_obj_align(menu_cont, LV_ALIGN_TOP_MID, 0, 52);
    lv_obj_set_style_bg_color(menu_cont, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(menu_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menu_cont, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(menu_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu_cont, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(menu_cont, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(menu_cont, 4, LV_PART_MAIN);

    /* Menu items — add each to the home group */
    btn = create_menu_btn(menu_cont, LV_SYMBOL_SETTINGS, "  Time",
                          COLOR_BLUE, menu_time_cb);
    lv_group_add_obj(group_home, btn);

    btn = create_menu_btn(menu_cont, LV_SYMBOL_WIFI, "  Weather",
                          COLOR_ORANGE, menu_weather_cb);
    lv_group_add_obj(group_home, btn);

    btn = create_menu_btn(menu_cont, LV_SYMBOL_LIST, "  System Info",
                          COLOR_GREEN, menu_system_cb);
    lv_group_add_obj(group_home, btn);
}

/* ====================================================================
 *  NTP time sync callback
 * ==================================================================== */
static lv_obj_t *sync_status_label = NULL;

static void sync_time_cb(lv_event_t *e)
{
    (void)e;
    extern int8_t esp8266_set_rtc_time_ntp(void);

    if (sync_status_label)
    {
        lv_label_set_text(sync_status_label, "Syncing...");
        lv_obj_set_style_text_color(sync_status_label, COLOR_ORANGE, LV_PART_MAIN);
    }

    /* esp8266_set_rtc_time_ntp blocks ~3s; runs in LVGL task context */
    int8_t ret = esp8266_set_rtc_time_ntp();

    if (sync_status_label)
    {
        if (ret == 0)
        {
            lv_label_set_text(sync_status_label, "Time Synced");
            lv_obj_set_style_text_color(sync_status_label, COLOR_GREEN, LV_PART_MAIN);
        }
        else
        {
            lv_label_set_text(sync_status_label, "Sync Failed");
            lv_obj_set_style_text_color(sync_status_label, COLOR_ORANGE, LV_PART_MAIN);
        }
    }
}

/* ====================================================================
 *  TIME PAGE
 * ==================================================================== */
static void create_time_page(void)
{
    scr_time = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_time, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr_time, 0, LV_PART_MAIN);

    /* ---- Group for keypad navigation ---- */
    group_time = lv_group_create();

    /* Title bar (back button added to group internally) */
    create_title_bar(scr_time, "Time", group_time);

    /* ---- Content area ---- */
    lv_obj_t *cont = lv_obj_create(scr_time);
    lv_obj_set_size(cont, 240, 148);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_set_style_bg_color(cont, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);

    /* Date card */
    lv_obj_t *card_date = lv_obj_create(cont);
    lv_obj_set_size(card_date, 220, 46);
    lv_obj_align(card_date, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_bg_color(card_date, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_radius(card_date, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card_date, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(card_date, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_date, 0, LV_PART_MAIN);

    label_time_date = lv_label_create(card_date);
    lv_label_set_text(label_time_date, "----/--/--");
    lv_obj_center(label_time_date);
    lv_obj_set_style_text_font(label_time_date, &lv_font_montserrat_24, LV_PART_MAIN);

    /* Time card */
    lv_obj_t *card_time = lv_obj_create(cont);
    lv_obj_set_size(card_time, 220, 56);
    lv_obj_align(card_time, LV_ALIGN_TOP_MID, 0, 58);
    lv_obj_set_style_bg_color(card_time, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_radius(card_time, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card_time, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(card_time, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_time, 0, LV_PART_MAIN);

    label_time_clock = lv_label_create(card_time);
    lv_label_set_text(label_time_clock, "--:--:--");
    lv_obj_center(label_time_clock);
    lv_obj_set_style_text_font(label_time_clock, &lv_font_montserrat_24, LV_PART_MAIN);

    /* Weekday card */
    lv_obj_t *card_wday = lv_obj_create(cont);
    lv_obj_set_size(card_wday, 220, 36);
    lv_obj_align(card_wday, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_style_bg_color(card_wday, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_radius(card_wday, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card_wday, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(card_wday, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_wday, 0, LV_PART_MAIN);

    label_time_weekday = lv_label_create(card_wday);
    lv_label_set_text(label_time_weekday, "--------");
    lv_obj_center(label_time_weekday);
    lv_obj_set_style_text_font(label_time_weekday, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_time_weekday, COLOR_GREEN, LV_PART_MAIN);

    /* ---- Sync status label (between cards and buttons) ---- */
    sync_status_label = lv_label_create(scr_time);
    lv_label_set_text(sync_status_label, "");
    lv_obj_align(sync_status_label, LV_ALIGN_TOP_MID, 0, 188);
    lv_obj_set_style_text_font(sync_status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(sync_status_label, COLOR_GRAY, LV_PART_MAIN);

    /* ---- Bottom button row (2 focusable buttons) ---- */
    lv_obj_t *btn_row = lv_obj_create(scr_time);
    lv_obj_set_size(btn_row, 230, 40);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(btn_row, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Sync NTP button */
    lv_obj_t *btn_sync = lv_btn_create(btn_row);
    lv_obj_set_size(btn_sync, 108, 36);
    lv_obj_set_style_bg_color(btn_sync, COLOR_BLUE, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_sync, 8, LV_PART_MAIN);
    lv_obj_add_style(btn_sync, &style_focus, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btn_sync, sync_time_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group_time, btn_sync);

    lv_obj_t *sync_label = lv_label_create(btn_sync);
    lv_label_set_text(sync_label, LV_SYMBOL_REFRESH " Sync NTP");
    lv_obj_center(sync_label);
    lv_obj_set_style_text_color(sync_label, COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_text_font(sync_label, &lv_font_montserrat_14, LV_PART_MAIN);

    /* Back button */
    lv_obj_t *btn_back2 = lv_btn_create(btn_row);
    lv_obj_set_size(btn_back2, 108, 36);
    lv_obj_set_style_bg_color(btn_back2, COLOR_GRAY, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_back2, 8, LV_PART_MAIN);
    lv_obj_add_style(btn_back2, &style_focus, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btn_back2, back_home_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group_time, btn_back2);

    lv_obj_t *back_label = lv_label_create(btn_back2);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_14, LV_PART_MAIN);
}

/* ====================================================================
 *  WEATHER PAGE
 * ==================================================================== */
static void create_weather_page(void)
{
    scr_weather = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_weather, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr_weather, 0, LV_PART_MAIN);

    /* ---- Group for keypad navigation ---- */
    group_weather = lv_group_create();

    /* Title bar */
    create_title_bar(scr_weather, "Weather", group_weather);

    /* ---- Content area ---- */
    lv_obj_t *cont = lv_obj_create(scr_weather);
    lv_obj_set_size(cont, 240, 200);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_set_style_bg_color(cont, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);

    /* City info card */
    lv_obj_t *card_city = lv_obj_create(cont);
    lv_obj_set_size(card_city, 220, 45);
    lv_obj_align(card_city, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(card_city, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_radius(card_city, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card_city, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(card_city, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_city, 0, LV_PART_MAIN);

    lv_obj_t *city_icon = lv_label_create(card_city);
    lv_label_set_text(city_icon, LV_SYMBOL_GPS);
    lv_obj_align(city_icon, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_style_text_color(city_icon, COLOR_ORANGE, LV_PART_MAIN);

    lv_obj_t *city_label = lv_label_create(card_city);
    lv_label_set_text(city_label, "City: Beijing");
    lv_obj_align(city_label, LV_ALIGN_LEFT_MID, 40, 0);
    lv_obj_set_style_text_font(city_label, &lv_font_montserrat_20, LV_PART_MAIN);

    /* Temperature card */
    lv_obj_t *card_temp = lv_obj_create(cont);
    lv_obj_set_size(card_temp, 220, 80);
    lv_obj_align(card_temp, LV_ALIGN_TOP_MID, 0, 65);
    lv_obj_set_style_bg_color(card_temp, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_radius(card_temp, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card_temp, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(card_temp, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_temp, 0, LV_PART_MAIN);

    /* Spinner for loading indication */
    spinner_weather = lv_spinner_create(card_temp, 1000, 60);
    lv_obj_set_size(spinner_weather, 40, 40);
    lv_obj_align(spinner_weather, LV_ALIGN_TOP_MID, 0, 5);

    label_weather_temp = lv_label_create(card_temp);
    lv_label_set_text(label_weather_temp, "--°C");
    lv_obj_align(label_weather_temp, LV_ALIGN_CENTER, 0, 5);
    lv_obj_set_style_text_font(label_weather_temp, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_weather_temp, COLOR_ORANGE, LV_PART_MAIN);

    /* Status label */
    label_weather_status = lv_label_create(card_temp);
    lv_label_set_text(label_weather_status, "Not updated");
    lv_obj_align(label_weather_status, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_text_color(label_weather_status, COLOR_GRAY, LV_PART_MAIN);

    /* Refresh button — add to group */
    lv_obj_t *btn_refresh = lv_btn_create(cont);
    lv_obj_set_size(btn_refresh, 180, 40);
    lv_obj_align(btn_refresh, LV_ALIGN_TOP_MID, 0, 155);
    lv_obj_set_style_bg_color(btn_refresh, COLOR_ORANGE, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_refresh, 8, LV_PART_MAIN);
    lv_obj_add_style(btn_refresh, &style_focus, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btn_refresh, weather_manual_refresh_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group_weather, btn_refresh);

    lv_obj_t *refresh_icon = lv_label_create(btn_refresh);
    lv_label_set_text(refresh_icon, LV_SYMBOL_REFRESH " Refresh");
    lv_obj_center(refresh_icon);
    lv_obj_set_style_text_color(refresh_icon, COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_text_font(refresh_icon, &lv_font_montserrat_20, LV_PART_MAIN);
}

/* ====================================================================
 *  SYSTEM INFO PAGE
 * ==================================================================== */
static void create_system_page(void)
{
    scr_system = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_system, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr_system, 0, LV_PART_MAIN);

    /* ---- Group for keypad navigation ---- */
    group_system = lv_group_create();

    /* Title bar */
    create_title_bar(scr_system, "System Info", group_system);

    /* ---- Content area ---- */
    lv_obj_t *cont = lv_obj_create(scr_system);
    lv_obj_set_size(cont, 240, 162);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_set_style_bg_color(cont, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);

    /* Info card */
    lv_obj_t *card_info = lv_obj_create(cont);
    lv_obj_set_size(card_info, 220, 156);
    lv_obj_align(card_info, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(card_info, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_radius(card_info, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(card_info, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(card_info, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_info, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_info, 10, LV_PART_MAIN);

    /*
     * Build system info using labels
     */
    const char *info_items[] = {
        "MCU:    STM32L475VET6",
        "Core:    ARM Cortex-M4F",
        "Clock:   80 MHz",
        "RTOS:    FreeRTOS",
        "LVGL:    v8.3.11",
        "Display: 240 x 240",
        "Color:   RGB565 (16-bit)",
        "WiFi:    ESP8266 (AT)",
        NULL
    };

    lv_obj_t *info_labels[8];
    for (int i = 0; info_items[i] != NULL; i++)
    {
        info_labels[i] = lv_label_create(card_info);
        lv_label_set_text(info_labels[i], info_items[i]);
        lv_obj_align(info_labels[i], LV_ALIGN_TOP_LEFT, 0, i * 17);
        lv_obj_set_style_text_color(info_labels[i], COLOR_TEXT, LV_PART_MAIN);
        lv_obj_set_style_text_font(info_labels[i], &lv_font_montserrat_14, LV_PART_MAIN);
    }

    /* ---- Bottom back button (focusable) ---- */
    lv_obj_t *btn_back2 = lv_btn_create(scr_system);
    lv_obj_set_size(btn_back2, 200, 36);
    lv_obj_align(btn_back2, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(btn_back2, COLOR_GRAY, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_back2, 8, LV_PART_MAIN);
    lv_obj_add_style(btn_back2, &style_focus, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btn_back2, back_home_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group_system, btn_back2);

    lv_obj_t *back_label = lv_label_create(btn_back2);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_14, LV_PART_MAIN);
}

/* ====================================================================
 *  Main entry: lv_user_app()
 *  Called once from lvgl_gui_task after LVGL and display/indev init
 * ==================================================================== */
void lv_user_app(void)
{
    /* ---- Initialize styles ---- */
    style_init();

    /* ---- Create all pages ---- */
    create_home_page();
    create_time_page();
    create_weather_page();
    create_system_page();

    /* ---- Load home page as initial screen ---- */
    lv_scr_load(scr_home);

    /* ---- Assign home group to keypad ---- */
    switch_group(group_home);

    /* ---- Start RTC refresh timer: every 1 second ---- */
    lv_timer_create(rtc_refresh_cb, 1000, NULL);

    /*
     * Start weather refresh timer: every 10 minutes (600000 ms).
     * NOTE: For testing, use a shorter interval (e.g., 30000 = 30s).
     *       Change to 600000 for production.
     */
    lv_timer_create(weather_refresh_cb, 600000, NULL);

    /*
     * Trigger an initial weather fetch immediately.
     * Note: this will block for ~3 seconds during init.
     */
    weather_refresh_cb(NULL);
}
