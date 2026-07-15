#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "wizchip_conf.h"
#include "CH395_hw.h"
#include "CH395CMD.H"
#include "debug_log.h"
#include <stdint.h>

static SemaphoreHandle_t s_ch395_spi_mutex;

void Delay_Us(uint32_t n)
{
    ch395_delay_us(n);
}

void Delay_Ms(uint32_t n)
{
    HAL_Delay(n);
}

void ch395_delay_us(uint32_t us)
{
    static uint8_t s_dwt_inited = 0U;

    if (s_dwt_inited == 0U)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0U;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        s_dwt_inited = 1U;
    }

    const uint32_t start = DWT->CYCCNT;
    const uint32_t cycles = (SystemCoreClock * us + 999999U) / 1000000U;

    while ((DWT->CYCCNT - start) < cycles)
    {
    }
}

 void ch395_cs_select(void)
{
    HAL_GPIO_WritePin(CH395_CS_GPIO_Port, CH395_CS_Pin, GPIO_PIN_RESET);
}

 void ch395_cs_deselect(void)
{
    HAL_GPIO_WritePin(CH395_CS_GPIO_Port, CH395_CS_Pin, GPIO_PIN_SET);
}

void ch395_cris_enter(void)
{
    xSemaphoreTake(s_ch395_spi_mutex, portMAX_DELAY);
}

void ch395_cris_exit(void)
{
    xSemaphoreGive(s_ch395_spi_mutex);
}

uint8_t ch395_spi_swapbyte(uint8_t wb)
{
    uint8_t tx = wb;
    uint8_t rx = 0x00;
    HAL_SPI_TransmitReceive(&CH395_SPI_HANDLE, &tx, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

void ch395_hw_reset(void)
{
    HAL_GPIO_WritePin(CH395_RST_GPIO_Port, CH395_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(CH395_RST_GPIO_Port, CH395_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
}

int8_t ch395_communication_test(void)
{
    // const uint8_t testdata = 0x5AU;
    // uint8_t result = 0U;

    return 0;
}

int8_t ch395_init(void)
{
    uint8_t ver = 0;
    if (ch395_communication_test() == 0)
    {
        LOG_INFO("CH395 communication test passed.");
    }
    else
    {
        LOG_ERROR("CH395 communication test failed.");
    }
    ver = CH395CMDGetVer();
    printf("CH395VER : %2x\r\n", ver);
    int8_t ret = CH395CMDInitCH395();
    return ret;
}


void ch395_hw_int_enable(void)
{
    HAL_NVIC_EnableIRQ(CH395_INT_EXTI_IRQn);
}

void ch395_hw_int_disable(void)
{
    HAL_NVIC_DisableIRQ(CH395_INT_EXTI_IRQn);
}

void ch395_hw_init(void)
{
    if (s_ch395_spi_mutex == NULL)
    {
        s_ch395_spi_mutex = xSemaphoreCreateMutex();
    }

    ch395_hw_reset();
}
