#include <stdint.h>
#include "main.h"
#include "cmsis_os.h"
#include "drv_lcd.h"

extern SPI_HandleTypeDef hspi3;
extern TIM_HandleTypeDef htim3;

/* Reset */
#define LCD_RESET_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
#define LCD_RESET_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);

/* Command / Data */
#define LCD_DC_LOW()     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
#define LCD_DC_HIGH()    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);

#define LCD_CS_LOW()     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
#define LCD_CS_HIGH()    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);

static int spi_transmit(uint8_t *data, uint16_t size)
{   
    LCD_CS_LOW();
    if (HAL_SPI_Transmit(&hspi3, data, size, HAL_MAX_DELAY) != HAL_OK)
    {
        return -1;
    }
    LCD_CS_HIGH();
    return size;
}

static int spi_receive(uint8_t *data, uint16_t size)
{   
    LCD_CS_LOW();
    if (HAL_SPI_Receive(&hspi3, data, size, HAL_MAX_DELAY) != HAL_OK)
    {
        return -1;
    }
    LCD_CS_HIGH();
    return size;
}

static int lcd_write_cmd(const uint8_t cmd)
{
    uint32_t len;

    LCD_DC_LOW();
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
    LCD_DC_HIGH();
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

    LCD_DC_HIGH();
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
    LCD_RESET_LOW();
    /* wait at least 100ms for reset */
    osDelay(100); 
    LCD_RESET_HIGH();
}

int lcd_init(void)
{
    /* lcd reset */
    lcd_gpio_init();

    /* timer start */
    HAL_TIM_Base_Start_IT(&htim3);

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
     osDelay(100); 

    lcd_write_cmd(0x29);

    return 0;
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
    LCD_DC_HIGH();
    spi_transmit(pcolor, size);
}
