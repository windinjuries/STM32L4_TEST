/**
  ******************************************************************************
  * @file    bsp_flash.c
  * @brief   STM32L4 internal Flash BSP interface implementation (Bank 2 operation)
  ******************************************************************************
  */

#include "bsp_flash.h"
#include <string.h>

/* Private macros ------------------------------------------------------------*/

/* Bank 2 address range */
#define BSP_FLASH_BANK2_END  (BSP_FLASH_BANK2_ADDR + BSP_FLASH_BANK2_SIZE - 1U)

/* Double-word alignment mask */
#define DOUBLEWORD_ALIGN_MASK  (~0x07UL)

/* Parameter validation macros -----------------------------------------------*/

#define IS_BANK2_PAGE(page)      ((page) < BSP_FLASH_BANK2_PAGE_COUNT)

#define IS_BANK2_ADDRESS(addr)   (((addr) >= BSP_FLASH_BANK2_ADDR) && \
                                  ((addr) <= BSP_FLASH_BANK2_END))

#define IS_BANK2_RANGE(addr, len) \
    (((addr) >= BSP_FLASH_BANK2_ADDR) && \
     ((uint32_t)((addr) + (len) - 1U) <= BSP_FLASH_BANK2_END) && \
     ((len) > 0U))

#define IS_DOUBLEWORD_ALIGNED(addr)  (((addr) & 0x07U) == 0U)

/* ---------------------------------------------------------------------------*/
/* Public functions                                                            */
/* ---------------------------------------------------------------------------*/

/**
  * @brief  Initialize Flash
  */
BSP_Flash_Status_t BSP_FLASH_Init(void)
{
    /* Unlock Flash control register */
    if (HAL_FLASH_Unlock() != HAL_OK) 
    {
        return BSP_FLASH_ERROR;
    }

    /* Clear all pending error flags */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    return BSP_FLASH_OK;
}

/**
  * @brief  Erase a specified page in Bank 2
  */
BSP_Flash_Status_t BSP_FLASH_ErasePage(uint32_t page)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    HAL_StatusTypeDef hal_status;

    /* Parameter validation */
    if (!IS_BANK2_PAGE(page)) {
        return BSP_FLASH_INVALID_PARAM;
    }

    /* Configure erase parameters */
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks     = FLASH_BANK_2;
    erase_init.Page      = page;
    erase_init.NbPages   = 1;

    hal_status = HAL_FLASHEx_Erase(&erase_init, &page_error);

    if (hal_status != HAL_OK) {
        return BSP_FLASH_ERROR;
    }

    return BSP_FLASH_OK;
}

/**
  * @brief  Write a 64-bit Double-Word
  */
BSP_Flash_Status_t BSP_FLASH_Write(uint32_t address, uint64_t data)
{
    HAL_StatusTypeDef hal_status;

    /* Parameter validation: address range + double-word alignment */
    if (!IS_BANK2_ADDRESS(address) || !IS_DOUBLEWORD_ALIGNED(address)) 
    {
        return BSP_FLASH_INVALID_PARAM;
    }

    hal_status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                   address, data);

    if (hal_status != HAL_OK) {
        return BSP_FLASH_ERROR;
    }

    return BSP_FLASH_OK;
}

/**
  * @brief  Write an arbitrary byte buffer (auto-handles double-word alignment)
  */
BSP_Flash_Status_t BSP_FLASH_WriteBuffer(uint32_t address,
                                         const uint8_t *data,
                                         uint32_t length)
{
    uint32_t offset;
    uint32_t aligned_addr;
    uint32_t tail_len;
    uint64_t dw_data;

    /* Parameter validation */
    if ((data == NULL) || (!IS_BANK2_RANGE(address, length))) {
        return BSP_FLASH_INVALID_PARAM;
    }

    offset = 0;

    /* ---- Handle leading unaligned bytes (read-modify-write) ---- */
    if (!IS_DOUBLEWORD_ALIGNED(address)) {
        uint32_t pre_bytes = 8U - (address & 0x07U);
        if (pre_bytes > length) {
            pre_bytes = length;
        }

        /* Base address aligned to double-word boundary */
        aligned_addr = address & DOUBLEWORD_ALIGN_MASK;

        /* Read existing double-word */
        dw_data = BSP_FLASH_Read64(aligned_addr);

        /* Overwrite corresponding bytes with input data */
        {
            uint32_t byte_offset = address - aligned_addr;
            uint8_t *p = (uint8_t *)&dw_data;
            for (uint32_t i = 0; i < pre_bytes; i++) {
                p[byte_offset + i] = data[offset + i];
            }
        }

        /* Write back */
        if (BSP_FLASH_Write(aligned_addr, dw_data) != BSP_FLASH_OK) {
            return BSP_FLASH_ERROR;
        }

        offset += pre_bytes;
    }

    /* ---- Write complete middle double-words ---- */
    while (offset + 8U <= length) {
        /* Destination address is guaranteed aligned (leading bytes already handled) */
        uint32_t dw_addr = address + offset;
        memcpy(&dw_data, &data[offset], 8);

        if (BSP_FLASH_Write(dw_addr, dw_data) != BSP_FLASH_OK) {
            return BSP_FLASH_ERROR;
        }

        offset += 8U;
    }

    /* ---- Handle trailing unaligned bytes (read-modify-write) ---- */
    tail_len = length - offset;
    if (tail_len > 0) {
        uint32_t dw_addr = address + offset;

        /* dw_addr is guaranteed double-word aligned at this point */
        aligned_addr = dw_addr;

        /* Read existing double-word */
        dw_data = BSP_FLASH_Read64(aligned_addr);

        /* Overwrite first tail_len bytes with input data */
        {
            uint8_t *p = (uint8_t *)&dw_data;
            for (uint32_t i = 0; i < tail_len; i++) {
                p[i] = data[offset + i];
            }
        }

        /* Write back */
        if (BSP_FLASH_Write(aligned_addr, dw_data) != BSP_FLASH_OK) {
            return BSP_FLASH_ERROR;
        }
    }

    return BSP_FLASH_OK;
}

/**
  * @brief  Read a 64-bit Double-Word
  */
uint64_t BSP_FLASH_Read64(uint32_t address)
{
    return *(volatile uint64_t *)address;
}

/**
  * @brief  Read an arbitrary byte buffer
  */
void BSP_FLASH_ReadBuffer(uint32_t address, uint8_t *data, uint32_t length)
{
    memcpy(data, (const void *)address, length);
}
