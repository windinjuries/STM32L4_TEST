#ifndef __DRV_LCD_H__
#define __DRV_LCD_H__

#include <stdint.h>
#define LCD_W 240
#define LCD_H 240

int  lcd_init(void);
void lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_fill_array(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, void *pcolor);

#endif
