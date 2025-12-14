#ifndef __DRV_LCD_H__
#define __DRV_LCD_H__

#define LCD_W 240
#define LCD_H 240

void lcd_enter_sleep(void);
void lcd_exit_sleep(void);
void lcd_display_on(void);
void lcd_display_off(void);
void lcd_display_brightness(uint8_t percent);

void lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_fill_array(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, void *pcolor);

#endif