
#include "stm32l475xx.h"
#include "stm32l4xx_hal_uart.h"
#define RX_BUFFER_LEN_LEN 256
static unsigned char rx_buffer[RX_BUFFER_LEN_LEN];
int 

void uart_dma_start_receive()
{

    HAL_UART_Receive_DMA(&huart1, rx_buffer, RX_BUFFER_LEN_LEN);
    
}