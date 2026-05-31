/**
 * @file    lv_port_indev.h
 * @brief   LVGL Input Device Port - Keypad driver header
 */

#ifndef __LV_PORT_INDEV_H__
#define __LV_PORT_INDEV_H__

#include "lvgl.h"

void          lv_port_indev_init(void);
lv_indev_t  *lv_port_indev_get_keypad(void);

#endif /* __LV_PORT_INDEV_H__ */
