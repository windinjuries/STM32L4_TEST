#include <stdint.h>
#include "main.h"
#include "cmsis_os.h"
#include "drv_led.h"

extern SPI_HandleTypeDef hspi3;

#define LCD_

static int spi_transmit(uint8_t *data, uint16_t size)
{   
    if (HAL_SPI_Transmit(&hspi3, data, size, HAL_MAX_DELAY) != HAL_OK)
    {
        return -1;
    }
    return size;
}

static int spi_receive(uint8_t *data, uint16_t size)
{   
    if (HAL_SPI_Receive(&hspi3, data, size, HAL_MAX_DELAY) != HAL_OK)
    {
        return -1;
    }
    return size;
}

static int lcd_write_cmd(const uint8_t cmd)
{
    uint32_t len;

    HAL_GPIO_ResetPin(GPIOB, GPIO_PIN_4);
    len = spi_transmit((uint8_t *)&cmd, 1);
    if (len != 1)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static int lcd_write_data(const uint8_t data)
{
    uint32_t len;
    HAL_GPIO_ResetPin(GPIOB, GPIO_PIN_4);
    len = spi_transmit((uint8_t *)&data, 1);
    if (len != 1)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static int lcd_write_half_word(const uint16_t da)
{
    uint32_t len;
    char data[2] = {0};

    data[0] = da >> 8;
    data[1] = da;

    HAL_GPIO_ResetPin(GPIOB, GPIO_PIN_4);
    len = spi_transmit((uint8_t *)&data, 2);
    if (len != 2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static void lcd_gpio_init(void)
{

    HAL_GPIO_ResetPin(GPIOB, GPIO_PIN_6);
    osDelay(100); /* wait at least 100ms for reset */
    HAL_GPIO_SetPin(GPIOB, GPIO_PIN_6);
}

static int hw_lcd_init(void)
{
    /* lcd reset */
    lcd_gpio_init();

    /* Memory Data Access Control */
    lcd_write_cmd(0x36);
    lcd_write_data(0x00);
    /* RGB 5-6-5-bit  */
    lcd_write_cmd(0x3A);
    lcd_write_data(0x65);
    /* Porch Setting */
    lcd_write_cmd(0xB2);
    lcd_write_data(0x0C);
    lcd_write_data(0x0C);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);
    /*  Gate Control */
    lcd_write_cmd(0xB7);
    lcd_write_data(0x35);
    /* VCOM Setting */
    lcd_write_cmd(0xBB);
    lcd_write_data(0x19);
    /* LCM Control */
    lcd_write_cmd(0xC0);
    lcd_write_data(0x2C);
    /* VDV and VRH Command Enable */
    lcd_write_cmd(0xC2);
    lcd_write_data(0x01);
    /* VRH Set */
    lcd_write_cmd(0xC3);
    lcd_write_data(0x12);
    /* VDV Set */
    lcd_write_cmd(0xC4);
    lcd_write_data(0x20);
    /* Frame Rate Control in Normal Mode */
    lcd_write_cmd(0xC6);
    lcd_write_data(0x0F);
    /* Power Control 1 */
    lcd_write_cmd(0xD0);
    lcd_write_data(0xA4);
    lcd_write_data(0xA1);
    /* Positive Voltage Gamma Control */
    lcd_write_cmd(0xE0);
    lcd_write_data(0xD0);
    lcd_write_data(0x04);
    lcd_write_data(0x0D);
    lcd_write_data(0x11);
    lcd_write_data(0x13);
    lcd_write_data(0x2B);
    lcd_write_data(0x3F);
    lcd_write_data(0x54);
    lcd_write_data(0x4C);
    lcd_write_data(0x18);
    lcd_write_data(0x0D);
    lcd_write_data(0x0B);
    lcd_write_data(0x1F);
    lcd_write_data(0x23);
    /* Negative Voltage Gamma Control */
    lcd_write_cmd(0xE1);
    lcd_write_data(0xD0);
    lcd_write_data(0x04);
    lcd_write_data(0x0C);
    lcd_write_data(0x11);
    lcd_write_data(0x13);
    lcd_write_data(0x2C);
    lcd_write_data(0x3F);
    lcd_write_data(0x44);
    lcd_write_data(0x51);
    lcd_write_data(0x2F);
    lcd_write_data(0x1F);
    lcd_write_data(0x1F);
    lcd_write_data(0x20);
    lcd_write_data(0x23);
    /* Display Inversion On */
    lcd_write_cmd(0x21);
    /* Sleep Out */
    lcd_write_cmd(0x11);
    /* wait for power stability */
    rt_thread_mdelay(100);

    /* display on */
    lcd_display_on();
    lcd_write_cmd(0x29);

    return 0;
}

void lcd_display_brightness(uint8_t percent)
{
    struct rt_device_pwm *pwm_dev;

    if(percent > 100)
    {
        percent = 100;
    }

    pwm_dev = (struct rt_device_pwm*)rt_device_find("pwm4");
    if(pwm_dev != RT_NULL)
    {
        rt_pwm_set(pwm_dev, 2, 1000000, percent*10000); /* PB7, PWM4 CH2 with 1000Hz */
        rt_pwm_enable(pwm_dev, 2);
    }
}

void lcd_display_on(void)
{
    lcd_display_brightness(100);
}

void lcd_display_off(void)
{
    lcd_display_brightness(0);
}

/* lcd enter the minimum power consumption mode and backlight off. */
void lcd_enter_sleep(void)
{
    lcd_display_off();
    rt_thread_mdelay(5);
    lcd_write_cmd(0x10);
}
/* lcd turn off sleep mode and backlight on. */
void lcd_exit_sleep(void)
{
    lcd_display_on();
    rt_thread_mdelay(5);
    lcd_write_cmd(0x11);
    rt_thread_mdelay(120);
}

/**
 * Set drawing area
 *
 * @param   x1      start of x position
 * @param   y1      start of y position
 * @param   x2      end of x position
 * @param   y2      end of y position
 *
 * @return  void
 */
void lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_write_cmd(0x2a);
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1);
    lcd_write_data(x2 >> 8);
    lcd_write_data(x2);

    lcd_write_cmd(0x2b);
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1);
    lcd_write_data(y2 >> 8);
    lcd_write_data(y2);

    lcd_write_cmd(0x2C);
}


/**
 * full color array on the lcd.
 *
 * @param   x_start     start of x position
 * @param   y_start     start of y position
 * @param   x_end       end of x position
 * @param   y_end       end of y position
 * @param   color       Fill color array's pointer
 *
 * @return  void
 */
void lcd_fill_array(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, void *pcolor)
{
    uint32_t size = 0;

    size = (x_end - x_start + 1) * (y_end - y_start + 1) * 2/*16bit*/;
    lcd_address_set(x_start, y_start, x_end, y_end);
    rt_pin_write(LCD_DC_PIN, PIN_HIGH);
    rt_spi_send(spi_dev_lcd, pcolor, size);
}
