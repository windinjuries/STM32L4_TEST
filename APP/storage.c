#include "bsp_flash.h"
#include <complex.h>
#include <stdint.h>

// FLASH Bank 2 Page 383
#define FLASH_PAGE_ADDR (0x08000000 + 0x7F800)

typedef struct
{
    uint32_t ota_flag;
    uint16_t reserved;
    uint16_t crc;
} parameter;

typedef struct
{
    uint32_t address;
    uint8_t init_status;
} storage_status;
static storage_status status;  
static parameter param;


int param_storage_read()
{
    // TODO
    BSP_FLASH_ReadBuffer(status.address, (uint8_t*)&param, sizeof(param));
    return 0;
}

int param_storage_write()
{
    // TODO
    BSP_FLASH_WriteBuffer(status.address, (uint8_t*)&param, sizeof(param));
    return 0;

}

int param_storage_init()
{
    // TODO
    BSP_FLASH_Init();
    status.address = FLASH_PAGE_ADDR;
    status.init_status = 0;
    param_storage_read();
    if(param.ota_flag == 0)
    {
        param.ota_flag = 1;
        BSP_FLASH_ErasePage(127);
        param_storage_write();
    }

    return 0;
}

