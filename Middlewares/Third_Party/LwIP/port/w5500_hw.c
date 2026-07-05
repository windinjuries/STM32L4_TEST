#include "w5500_hw.h"
#include "wizchip_conf.h"
#include "w5500.h"
#include "debug_log.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

SPI_HandleTypeDef hspi1;

static SemaphoreHandle_t s_w5500_spi_mutex;

static void w5500_cs_select(void)
{
    HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_RESET);
}

static void w5500_cs_deselect(void)
{
    HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_SET);
}

static void w5500_cris_enter(void)
{
    xSemaphoreTake(s_w5500_spi_mutex, portMAX_DELAY);
}

static void w5500_cris_exit(void)
{
    xSemaphoreGive(s_w5500_spi_mutex);
}

static uint8_t w5500_spi_readbyte(void)
{
    uint8_t tx = 0x00;
    uint8_t rx = 0x00;
    HAL_SPI_TransmitReceive(&W5500_SPI_HANDLE, &tx, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

static void w5500_spi_writebyte(uint8_t wb)
{
    HAL_SPI_Transmit(&W5500_SPI_HANDLE, &wb, 1, HAL_MAX_DELAY);
}

static void w5500_spi_readburst(uint8_t *pBuf, uint16_t len)
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

        HAL_SPI_TransmitReceive(&W5500_SPI_HANDLE,
                                dummy_tx,
                                &pBuf[offset],
                                chunk,
                                HAL_MAX_DELAY);
        offset = (uint16_t)(offset + chunk);
    }
}

static void w5500_spi_writeburst(uint8_t *pBuf, uint16_t len)
{
    if ((pBuf == NULL) || (len == 0U))
    {
        return;
    }
    HAL_SPI_Transmit(&W5500_SPI_HANDLE, pBuf, len, HAL_MAX_DELAY);
}

void w5500_hw_reset(void)
{
    HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
}

void w5500_hw_register_callbacks(void)
{
    reg_wizchip_cris_cbfunc(w5500_cris_enter, w5500_cris_exit);
    reg_wizchip_cs_cbfunc(w5500_cs_select, w5500_cs_deselect);
    reg_wizchip_spi_cbfunc(w5500_spi_readbyte, w5500_spi_writebyte);
    reg_wizchip_spiburst_cbfunc(w5500_spi_readburst, w5500_spi_writeburst);
}

uint8_t w5500_hw_register_read_test(void)
{
    uint8_t tx[4] = {0x00U, 0x39U, 0x00U, 0x00U};
    uint8_t rx[4] = {0x00U, 0x00U, 0x00U, 0x00U};
    uint8_t version;

    HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&W5500_SPI_HANDLE, tx, rx, 4U, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_SET);

    version = rx[3];
    LOG_INFO("W5500 direct read: tx=0x%02X 0x%02X 0x%02X 0x%02X rx=0x%02X 0x%02X 0x%02X 0x%02X",
             tx[0], tx[1], tx[2], tx[3], rx[0], rx[1], rx[2], rx[3]);
    LOG_INFO("W5500 register test: VERSIONR=0x%02X", version);
    if (version != 0x04U)
    {
        LOG_ERROR("W5500 register test failed: VERSIONR=0x%02X", version);
        return 0U;
    }

    return 1U;
}

void w5500_hw_int_enable(void)
{
    HAL_NVIC_EnableIRQ(W5500_INT_EXTI_IRQn);
}

void w5500_hw_int_disable(void)
{
    HAL_NVIC_DisableIRQ(W5500_INT_EXTI_IRQn);
}

void w5500_hw_init(void)
{
    if (s_w5500_spi_mutex == NULL)
    {
        s_w5500_spi_mutex = xSemaphoreCreateMutex();
    }

    w5500_hw_reset();
    w5500_hw_register_callbacks();
    if (w5500_hw_register_read_test() != 0U)
    {
        LOG_INFO("W5500 register read test passed.");
    }
    else
    {
        LOG_ERROR("W5500 register read test failed.");
    }
}
