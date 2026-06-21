/**
 ******************************************************************************
 * @file    modbus.c
 * @brief   Modbus RTU slave middleware implementation
 * @note    Uses UART3 + DMA reception + IDLE interrupt to detect frame boundaries.
 *          Frames are processed in a dedicated FreeRTOS task synchronized by a semaphore.
 ******************************************************************************
 */

#include "modbus.h"
#include "config.h"
#include "main.h"
#include "cmsis_os.h"
#include <string.h>

extern UART_HandleTypeDef  huart3;
extern DMA_HandleTypeDef   hdma_usart3_rx;

#define MODBUS_FRAME_BUF_COUNT  2

typedef enum
{
    MODBUS_FUNC_READ_HOLDING_REGS    = 0x03,
    MODBUS_FUNC_READ_INPUT_REGS      = 0x04,
    MODBUS_FUNC_WRITE_SINGLE_REG     = 0x06,
    MODBUS_FUNC_WRITE_MULTIPLE_REGS  = 0x10,
} modbus_func_code_t;


/** Modbus task semaphore, released by ISR and waited by the task */
static osSemaphoreId  rx_sem_id;
osSemaphoreDef(rx_sem);

/** DMA receive buffer, written only by DMA */
static uint8_t  rx_dma_buf[MODBUS_RX_BUF_SIZE];

/** Double frame buffer: ISR copies complete frames here, task parses from it */
static uint8_t  rx_frame_buf[MODBUS_FRAME_BUF_COUNT][MODBUS_RX_BUF_SIZE];
static volatile uint16_t rx_frame_len[MODBUS_FRAME_BUF_COUNT];
static volatile uint8_t  rx_frame_write_idx;
static volatile uint8_t  rx_frame_read_idx;
static volatile uint8_t  rx_frame_count;
static volatile uint32_t rx_frame_overflow_count;

/** Frame currently being parsed */
static const uint8_t *rx_buf;

/** Length of the frame currently being parsed */
static uint16_t rx_current_frame_len;

/** Transmit buffer */
static uint8_t  tx_buf[MODBUS_RX_BUF_SIZE];

/** Register files */
static int16_t  holding_regs[MODBUS_HOLDING_REG_COUNT];
static int16_t  input_regs[MODBUS_INPUT_REG_COUNT];

/*------------------------------------------------------------------------------
 * CRC16 - Modbus RTU polynomial (0xA001)
 *----------------------------------------------------------------------------*/

/**
 * @brief  Calculate Modbus RTU CRC16
 * @param  buf   Data buffer pointer
 * @param  len   Data length in bytes
 * @return CRC16 value
 */
static uint16_t modbus_crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;

    while (len--)
    {
        crc ^= *buf++;
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/*------------------------------------------------------------------------------
 * DMA receive management
 *----------------------------------------------------------------------------*/

/**
 * @brief  Start or restart DMA reception
 * @note   Enable IDLE interrupt after DMA reception is armed.
 */
static void modbus_start_rx(void)
{
    HAL_UART_Receive_DMA(&huart3, rx_dma_buf, MODBUS_RX_BUF_SIZE);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
}

/*------------------------------------------------------------------------------
 * Exception response
 *----------------------------------------------------------------------------*/

/**
 * @brief  Send a Modbus exception response
 * @param  func     Original function code
 * @param  exc_code Exception code
 */
static void modbus_send_exception(uint8_t func, uint8_t exc_code)
{
    tx_buf[0] = MODBUS_SLAVE_ADDRESS;
    tx_buf[1] = func | 0x80;            /* Exception flag */
    tx_buf[2] = exc_code;

    uint16_t crc = modbus_crc16(tx_buf, 3);
    tx_buf[3] = (uint8_t)(crc & 0xFF);
    tx_buf[4] = (uint8_t)(crc >> 8);

    HAL_UART_Transmit(&huart3, tx_buf, 5, 100);
}

/*------------------------------------------------------------------------------
 * Function code handlers
 *----------------------------------------------------------------------------*/

/**
 * @brief  Handle 0x03 - Read Holding Registers
 * @note   Request frame: [addr][0x03][start_hi][start_lo][qty_hi][qty_lo][crc_lo][crc_hi]
 */
static void handle_read_holding_regs(void)
{
    if (rx_current_frame_len != 8)
    {
        modbus_send_exception(MODBUS_FUNC_READ_HOLDING_REGS, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t start_addr = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
    uint16_t quantity   = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];

    /* Quantity range: 1 to 125 */
    if (quantity < 1 || quantity > 125)
    {
        modbus_send_exception(MODBUS_FUNC_READ_HOLDING_REGS, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    /* Address range check */
    if ((uint32_t)start_addr + quantity > MODBUS_HOLDING_REG_COUNT)
    {
        modbus_send_exception(MODBUS_FUNC_READ_HOLDING_REGS, MODBUS_EXC_ILLEGAL_DATA_ADDRESS);
        return;
    }

    /* Build response */
    uint8_t byte_count = (uint8_t)(quantity * 2);
    tx_buf[0] = MODBUS_SLAVE_ADDRESS;
    tx_buf[1] = MODBUS_FUNC_READ_HOLDING_REGS;
    tx_buf[2] = byte_count;

    for (uint16_t i = 0; i < quantity; i++)
    {
        int16_t val = holding_regs[start_addr + i];
        tx_buf[3 + i * 2]     = (uint8_t)((uint16_t)val >> 8);
        tx_buf[3 + i * 2 + 1] = (uint8_t)((uint16_t)val & 0xFF);
    }

    uint16_t resp_len = 3 + byte_count;
    uint16_t crc = modbus_crc16(tx_buf, resp_len);
    tx_buf[resp_len]     = (uint8_t)(crc & 0xFF);
    tx_buf[resp_len + 1] = (uint8_t)(crc >> 8);

    HAL_UART_Transmit(&huart3, tx_buf, resp_len + 2, 100);
}

/**
 * @brief  Handle 0x04 - Read Input Registers
 * @note   Request frame has the same format as 0x03, with a different register file.
 */
static void handle_read_input_regs(void)
{
    if (rx_current_frame_len != 8)
    {
        modbus_send_exception(MODBUS_FUNC_READ_INPUT_REGS, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t start_addr = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
    uint16_t quantity   = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];

    if (quantity < 1 || quantity > 125)
    {
        modbus_send_exception(MODBUS_FUNC_READ_INPUT_REGS, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    if ((uint32_t)start_addr + quantity > MODBUS_INPUT_REG_COUNT)
    {
        modbus_send_exception(MODBUS_FUNC_READ_INPUT_REGS, MODBUS_EXC_ILLEGAL_DATA_ADDRESS);
        return;
    }

    uint8_t byte_count = (uint8_t)(quantity * 2);
    tx_buf[0] = MODBUS_SLAVE_ADDRESS;
    tx_buf[1] = MODBUS_FUNC_READ_INPUT_REGS;
    tx_buf[2] = byte_count;

    for (uint16_t i = 0; i < quantity; i++)
    {
        int16_t val = input_regs[start_addr + i];
        tx_buf[3 + i * 2]     = (uint8_t)((uint16_t)val >> 8);
        tx_buf[3 + i * 2 + 1] = (uint8_t)((uint16_t)val & 0xFF);
    }

    uint16_t resp_len = 3 + byte_count;
    uint16_t crc = modbus_crc16(tx_buf, resp_len);
    tx_buf[resp_len]     = (uint8_t)(crc & 0xFF);
    tx_buf[resp_len + 1] = (uint8_t)(crc >> 8);

    HAL_UART_Transmit(&huart3, tx_buf, resp_len + 2, 100);
}

/**
 * @brief  Handle 0x06 - Write Single Register
 * @note   Request frame: [addr][0x06][reg_hi][reg_lo][val_hi][val_lo][crc_lo][crc_hi]
 *         Response echoes the request frame.
 */
static void handle_write_single_reg(void)
{
    if (rx_current_frame_len != 8)
    {
        modbus_send_exception(MODBUS_FUNC_WRITE_SINGLE_REG, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t reg_addr = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
    uint16_t reg_val  = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];

    if (reg_addr >= MODBUS_HOLDING_REG_COUNT)
    {
        modbus_send_exception(MODBUS_FUNC_WRITE_SINGLE_REG, MODBUS_EXC_ILLEGAL_DATA_ADDRESS);
        return;
    }

    holding_regs[reg_addr] = (int16_t)reg_val;

    /* Echo the request */
    tx_buf[0] = MODBUS_SLAVE_ADDRESS;
    tx_buf[1] = MODBUS_FUNC_WRITE_SINGLE_REG;
    tx_buf[2] = rx_buf[2];      /* Register address high byte */
    tx_buf[3] = rx_buf[3];      /* Register address low byte */
    tx_buf[4] = rx_buf[4];      /* Register value high byte */
    tx_buf[5] = rx_buf[5];      /* Register value low byte */

    uint16_t crc = modbus_crc16(tx_buf, 6);
    tx_buf[6] = (uint8_t)(crc & 0xFF);
    tx_buf[7] = (uint8_t)(crc >> 8);

    HAL_UART_Transmit(&huart3, tx_buf, 8, 100);
}

/**
 * @brief  Handle 0x10 - Write Multiple Registers
 * @note   Request frame: [addr][0x10][start_hi][start_lo][qty_hi][qty_lo][byte_cnt][data...][crc]
 *         Response frame: [addr][0x10][start_hi][start_lo][qty_hi][qty_lo][crc]
 */
static void handle_write_multiple_regs(void)
{
    /* Minimum length: addr + func + start(2) + qty(2) + byte_cnt + crc(2) = 9 */
    if (rx_current_frame_len < 9)
    {
        modbus_send_exception(MODBUS_FUNC_WRITE_MULTIPLE_REGS, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t start_addr = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
    uint16_t quantity   = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];
    uint8_t  byte_count = rx_buf[6];

    if (quantity < 1 || quantity > 123)
    {
        modbus_send_exception(MODBUS_FUNC_WRITE_MULTIPLE_REGS, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    if (byte_count != quantity * 2)
    {
        modbus_send_exception(MODBUS_FUNC_WRITE_MULTIPLE_REGS, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    /* Total length = 7-byte fixed header + byte_count data + 2-byte CRC */
    if (rx_current_frame_len < (uint16_t)(7 + byte_count + 2))
    {
        modbus_send_exception(MODBUS_FUNC_WRITE_MULTIPLE_REGS, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    if ((uint32_t)start_addr + quantity > MODBUS_HOLDING_REG_COUNT)
    {
        modbus_send_exception(MODBUS_FUNC_WRITE_MULTIPLE_REGS, MODBUS_EXC_ILLEGAL_DATA_ADDRESS);
        return;
    }

    /* Write registers */
    for (uint16_t i = 0; i < quantity; i++)
    {
        holding_regs[start_addr + i] =
            (int16_t)(((uint16_t)rx_buf[7 + i * 2] << 8) | rx_buf[7 + i * 2 + 1]);
    }

    /* Response echoes start address and quantity */
    tx_buf[0] = MODBUS_SLAVE_ADDRESS;
    tx_buf[1] = MODBUS_FUNC_WRITE_MULTIPLE_REGS;
    tx_buf[2] = rx_buf[2];
    tx_buf[3] = rx_buf[3];
    tx_buf[4] = rx_buf[4];
    tx_buf[5] = rx_buf[5];

    uint16_t crc = modbus_crc16(tx_buf, 6);
    tx_buf[6] = (uint8_t)(crc & 0xFF);
    tx_buf[7] = (uint8_t)(crc >> 8);

    HAL_UART_Transmit(&huart3, tx_buf, 8, 100);
}

/*------------------------------------------------------------------------------
 * Frame parsing and dispatch
 *----------------------------------------------------------------------------*/

/**
 * @brief  Parse and process a received Modbus frame
 * @note   Checks slave address and CRC, then dispatches by function code.
 */
static void modbus_process_frame(void)
{
    uint16_t len = rx_current_frame_len;

    /* Minimum frame: addr(1) + func(1) + crc(2) = 4 bytes */
    if (len < 4)
    {
        return;
    }

    /* Slave address filter */
    if (rx_buf[0] != MODBUS_SLAVE_ADDRESS)
    {
        return;
    }

    /* CRC check */
    uint16_t rx_crc = ((uint16_t)rx_buf[len - 1] << 8) | rx_buf[len - 2];
    if (modbus_crc16(rx_buf, len - 2) != rx_crc)
    {
        return;
    }

    /* Dispatch by function code */
    modbus_func_code_t func = (modbus_func_code_t)rx_buf[1];

    switch (func)
    {
        case MODBUS_FUNC_READ_HOLDING_REGS:
            handle_read_holding_regs();
            break;

        case MODBUS_FUNC_READ_INPUT_REGS:
            handle_read_input_regs();
            break;

        case MODBUS_FUNC_WRITE_SINGLE_REG:
            handle_write_single_reg();
            break;

        case MODBUS_FUNC_WRITE_MULTIPLE_REGS:
            handle_write_multiple_regs();
            break;

        default:
            modbus_send_exception((uint8_t)func, MODBUS_EXC_ILLEGAL_FUNCTION);
            break;
    }
}

void modbus_init(void)
{
    rx_sem_id = osSemaphoreCreate(osSemaphore(rx_sem), 1);

    memset(holding_regs, 0, sizeof(holding_regs));
    memset(input_regs,  0, sizeof(input_regs));

    rx_buf = NULL;
    rx_current_frame_len = 0;
    rx_frame_write_idx = 0;
    rx_frame_read_idx = 0;
    rx_frame_count = 0;
    rx_frame_overflow_count = 0;

    modbus_start_rx();
}

void modbus_rx_notify(uint16_t size)
{
    if (size > 0 && size <= MODBUS_RX_BUF_SIZE)
    {
        if (rx_frame_count < MODBUS_FRAME_BUF_COUNT)
        {
            uint8_t idx = rx_frame_write_idx;

            memcpy(rx_frame_buf[idx], rx_dma_buf, size);
            rx_frame_len[idx] = size;
            rx_frame_write_idx = (uint8_t)((idx + 1U) % MODBUS_FRAME_BUF_COUNT);
            rx_frame_count++;

            osSemaphoreRelease(rx_sem_id);
        }
        else
        {
            rx_frame_overflow_count++;
        }
    }
    modbus_start_rx();
}

void modbus_task(void const *argument)
{
    (void)argument;

    for (;;)
    {
        uint8_t idx = 0;
        uint8_t has_frame = 0;
        uint32_t primask;

        /* Wait for a complete Modbus frame */
        osSemaphoreWait(rx_sem_id, osWaitForever);

        primask = __get_PRIMASK();
        __disable_irq();
        if (rx_frame_count > 0)
        {
            idx = rx_frame_read_idx;
            rx_buf = rx_frame_buf[idx];
            rx_current_frame_len = rx_frame_len[idx];
            has_frame = 1;
        }
        if (primask == 0U)
        {
            __enable_irq();
        }

        if (has_frame)
        {
            modbus_process_frame();

            primask = __get_PRIMASK();
            __disable_irq();
            rx_frame_read_idx = (uint8_t)((idx + 1U) % MODBUS_FRAME_BUF_COUNT);
            rx_frame_count--;
            if (primask == 0U)
            {
                __enable_irq();
            }
        }
    }
}

/*------------------------------------------------------------------------------
 * Register read/write API
 *----------------------------------------------------------------------------*/

int16_t modbus_read_holding_register(uint16_t addr)
{
    if (addr < MODBUS_HOLDING_REG_COUNT)
    {
        return holding_regs[addr];
    }
    return 0;
}

void modbus_write_holding_register(uint16_t addr, int16_t value)
{
    if (addr < MODBUS_HOLDING_REG_COUNT)
    {
        holding_regs[addr] = value;
    }
}

int16_t modbus_read_input_register(uint16_t addr)
{
    if (addr < MODBUS_INPUT_REG_COUNT)
    {
        return input_regs[addr];
    }
    return 0;
}

void modbus_write_input_register(uint16_t addr, int16_t value)
{
    if (addr < MODBUS_INPUT_REG_COUNT)
    {
        input_regs[addr] = value;
    }
}
