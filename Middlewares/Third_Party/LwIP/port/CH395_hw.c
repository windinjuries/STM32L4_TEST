#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "CH395_hw.h"
#include "debug_log.h"
#include <stdint.h>

SPI_HandleTypeDef hspi1;

static SemaphoreHandle_t s_ch395q_spi_mutex;

static void ch395q_cs_select(void)
{
    HAL_GPIO_WritePin(ch395q_CS_GPIO_Port, ch395q_CS_Pin, GPIO_PIN_RESET);
}

static void ch395q_cs_deselect(void)
{
    HAL_GPIO_WritePin(ch395q_CS_GPIO_Port, ch395q_CS_Pin, GPIO_PIN_SET);
}

static void ch395q_cris_enter(void)
{
    xSemaphoreTake(s_ch395q_spi_mutex, portMAX_DELAY);
}

static void ch395q_cris_exit(void)
{
    xSemaphoreGive(s_ch395q_spi_mutex);
}

static uint8_t ch395q_spi_readbyte(void)
{
    uint8_t tx = 0x00;
    uint8_t rx = 0x00;
    HAL_SPI_TransmitReceive(&ch395q_SPI_HANDLE, &tx, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

static void ch395q_spi_writebyte(uint8_t wb)
{
    HAL_SPI_Transmit(&ch395q_SPI_HANDLE, &wb, 1, HAL_MAX_DELAY);
}

static void ch395q_spi_readburst(uint8_t *pBuf, uint16_t len)
{
    static uint8_t dummy_tx[64];
    uint16_t offset = 0U;
    uint16_t chunk;

    if ((pBuf == NULL) || (len == 0U))
    {
        return;
    }

    while (offset < len)
    {
        chunk = (uint16_t)(len - offset);
        if (chunk > sizeof(dummy_tx))
        {
            chunk = sizeof(dummy_tx);
        }

        HAL_SPI_TransmitReceive(&ch395q_SPI_HANDLE,
                                dummy_tx,
                                &pBuf[offset],
                                chunk,
                                HAL_MAX_DELAY);
        offset = (uint16_t)(offset + chunk);
    }
}

static void ch395q_spi_writeburst(uint8_t *pBuf, uint16_t len)
{
    if ((pBuf == NULL) || (len == 0U))
    {
        return;
    }
    HAL_SPI_Transmit(&ch395q_SPI_HANDLE, pBuf, len, HAL_MAX_DELAY);
}

void ch395q_hw_reset(void)
{
    HAL_GPIO_WritePin(ch395q_RST_GPIO_Port, ch395q_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(ch395q_RST_GPIO_Port, ch395q_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
}

void ch395q_hw_register_callbacks(void)
{
    reg_wizchip_cris_cbfunc(ch395q_cris_enter, ch395q_cris_exit);
    reg_wizchip_cs_cbfunc(ch395q_cs_select, ch395q_cs_deselect);
    reg_wizchip_spi_cbfunc(ch395q_spi_readbyte, ch395q_spi_writebyte);
    reg_wizchip_spiburst_cbfunc(ch395q_spi_readburst, ch395q_spi_writeburst);
}
int8_t ch395q_read_version_test(void)
{
    uint8_t version = 0U;
    ch395q_cris_enter();
    ch395q_spi_writebyte(0x01);
    version = ch395q_spi_readbyte();
    ch395q_cris_exit();

    LOG_INFO("ch395q register test: VERSIONR=0x%02X", version);
    if (version != 0x04U)
    {
        LOG_ERROR("ch395q register test failed: VERSIONR=0x%02X", version);
        return -1;
    }

    return 0;

}

void ch395q_hw_int_enable(void)
{
    HAL_NVIC_EnableIRQ(ch395q_INT_EXTI_IRQn);
}

void ch395q_hw_int_disable(void)
{
    HAL_NVIC_DisableIRQ(ch395q_INT_EXTI_IRQn);
}

void ch395q_hw_init(void)
{
    if (s_ch395q_spi_mutex == NULL)
    {
        s_ch395q_spi_mutex = xSemaphoreCreateMutex();
    }

    ch395q_hw_reset();
    ch395q_hw_register_callbacks();
    if (ch395q_hw_register_read_test() != 0U)
    {
        LOG_INFO("ch395q register read test passed.");
    }
    else
    {
        LOG_ERROR("ch395q register read test failed.");
    }
}
