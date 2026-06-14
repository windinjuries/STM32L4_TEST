/**
  ******************************************************************************
  * @file    bsp_flash.h
  * @brief   STM32L4 internal Flash BSP interface (Bank 2 operation)
  ******************************************************************************
  */

#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Bank 2 parameters ---------------------------------------------------------*/
#define BSP_FLASH_BANK2_ADDR         0x08040000UL       /* Bank 2 start address */
#define BSP_FLASH_BANK2_SIZE         (256UL * 1024UL)   /* Bank 2 size 256KB */
#define BSP_FLASH_PAGE_SIZE          0x800UL            /* Page size 2KB */
#define BSP_FLASH_BANK2_PAGE_COUNT   128                /* Bank 2 page count */

/* Test area — last 4 pages of Bank 2 (final 8KB) ----------------------------*/
#define BSP_FLASH_TEST_ADDR          0x0807F000UL       /* Test area start address */
#define BSP_FLASH_TEST_PAGE          124                /* Test start page number within Bank 2 */

/* Return status -------------------------------------------------------------*/
typedef enum {
    BSP_FLASH_OK = 0,
    BSP_FLASH_ERROR,
    BSP_FLASH_INVALID_PARAM
} BSP_Flash_Status_t;

/* API function declarations -------------------------------------------------*/

/**
  * @brief  Initialize Flash (unlock and clear error flags)
  * @retval BSP_Flash_Status_t
  */
BSP_Flash_Status_t BSP_FLASH_Init(void);

/**
  * @brief  Erase a specified page in Bank 2
  * @param  page: Page number within Bank 2 (0 ~ 127)
  * @retval BSP_Flash_Status_t
  */
BSP_Flash_Status_t BSP_FLASH_ErasePage(uint32_t page);

/**
  * @brief  Write a 64-bit Double-Word (address must be double-word aligned)
  * @param  address: Double-word aligned address within Bank 2
  * @param  data: 64-bit data
  * @retval BSP_Flash_Status_t
  */
BSP_Flash_Status_t BSP_FLASH_Write(uint32_t address, uint64_t data);

/**
  * @brief  Write an arbitrary byte buffer (auto-handles alignment)
  * @param  address: Start address within Bank 2
  * @param  data: Source data buffer pointer
  * @param  length: Data length (bytes)
  * @retval BSP_Flash_Status_t
  */
BSP_Flash_Status_t BSP_FLASH_WriteBuffer(uint32_t address,
                                         const uint8_t *data,
                                         uint32_t length);

/**
  * @brief  Read a 64-bit Double-Word
  * @param  address: Read address
  * @retval 64-bit data
  */
uint64_t BSP_FLASH_Read64(uint32_t address);

/**
  * @brief  Read an arbitrary byte buffer
  * @param  address: Start address
  * @param  data: Destination buffer pointer
  * @param  length: Read length (bytes)
  */
void BSP_FLASH_ReadBuffer(uint32_t address, uint8_t *data, uint32_t length);

/* Test entry -----------------------------------------------------------------*/
void bsp_flash_run_tests(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_FLASH_H */
