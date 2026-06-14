/**
  ******************************************************************************
  * @file    test_bsp_flash.c
  * @brief   STM32L4 Flash BSP unit tests (Unity framework)
  ******************************************************************************
  */

#include "bsp_flash.h"
#include "unity_config.h"
#include "unity.h"
#include <string.h>

/* ==========================================================================*/
/* Test helper variables                                                      */
/* ==========================================================================*/

/* Test pattern data: incrementing bytes */
static const uint8_t test_pattern[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
    0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
    0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
    0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* Test page count (uses last 4 pages: 124, 125, 126, 127) */
#define TEST_PAGE_COUNT  4

/* ==========================================================================*/
/* setUp / tearDown                                                          */
/* ==========================================================================*/

void setUp(void)
{
    /* Erase the test start page to prepare for write tests */
    (void)BSP_FLASH_ErasePage(BSP_FLASH_TEST_PAGE);
}

void tearDown(void)
{
    /* No operation needed */
}

/* ==========================================================================*/
/* Helper functions                                                           */
/* ==========================================================================*/

/**
  * @brief  Verify all bytes in the specified address range are 0xFF
  */
static int verify_erased(uint32_t addr, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)addr;
    for (uint32_t i = 0; i < len; i++) {
        if (p[i] != 0xFF) {
            return 0;
        }
    }
    return 1;
}

/**
  * @brief  Verify two memory regions have matching data
  */
static int verify_data(uint32_t addr, const uint8_t *expected, uint32_t len)
{
    uint8_t buf[256];
    BSP_FLASH_ReadBuffer(addr, buf, len);
    return (memcmp(buf, expected, len) == 0);
}

/* ==========================================================================*/
/* Test cases                                                                 */
/* ==========================================================================*/

/**
  * @brief  Test: erase a page and verify all bytes become 0xFF
  */
void test_flash_erase_page(void)
{
    uint32_t test_addr = BSP_FLASH_TEST_ADDR;

    TEST_ASSERT_EQUAL(BSP_FLASH_OK, BSP_FLASH_ErasePage(BSP_FLASH_TEST_PAGE));

    /* Verify entire 2KB page is 0xFF */
    TEST_ASSERT_TRUE(verify_erased(test_addr, BSP_FLASH_PAGE_SIZE));
}

/**
  * @brief  Test: write a 64-bit value and read it back
  */
void test_flash_write_doubleword(void)
{
    uint64_t test_val = 0x0123456789ABCDEFULL;
    uint32_t test_addr = BSP_FLASH_TEST_ADDR;

    TEST_ASSERT_EQUAL(BSP_FLASH_OK,
                      BSP_FLASH_Write(test_addr, test_val));

    uint64_t read_back = BSP_FLASH_Read64(test_addr);
    TEST_ASSERT_EQUAL_UINT64(test_val, read_back);
}

/**
  * @brief  Test: write a byte buffer and read back byte-by-byte
  */
void test_flash_write_buffer(void)
{
    uint32_t test_addr = BSP_FLASH_TEST_ADDR;
    uint32_t len = sizeof(test_pattern);

    TEST_ASSERT_EQUAL(BSP_FLASH_OK,
                      BSP_FLASH_WriteBuffer(test_addr, test_pattern, len));

    uint8_t buf[256];
    BSP_FLASH_ReadBuffer(test_addr, buf, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(test_pattern, buf, len);
}

/**
  * @brief  Test: write to a non-double-word-aligned address (unaligned write)
  */
void test_flash_write_unaligned(void)
{
    /* Start writing at address +3, i.e. 4-byte aligned but not 8-byte aligned */
    uint32_t test_addr = BSP_FLASH_TEST_ADDR + 3U;
    const uint8_t data[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33 };
    uint32_t len = sizeof(data);

    TEST_ASSERT_EQUAL(BSP_FLASH_OK,
                      BSP_FLASH_WriteBuffer(test_addr, data, len));

    /* Verify data */
    uint8_t buf[9];
    BSP_FLASH_ReadBuffer(test_addr, buf, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, buf, len);

    /* Verify first 3 bytes (address 0, 1, 2) are untouched (should be 0xFF) */
    TEST_ASSERT_TRUE(verify_erased(BSP_FLASH_TEST_ADDR, 3));
}

/**
  * @brief  Test: complete erase->write->read-back flow
  */
void test_flash_erase_then_write(void)
{
    uint32_t test_addr = BSP_FLASH_TEST_ADDR;
    uint32_t len = 128; /* Only test first 128 bytes */

    /* Step 1: Erase */
    TEST_ASSERT_EQUAL(BSP_FLASH_OK,
                      BSP_FLASH_ErasePage(BSP_FLASH_TEST_PAGE));

    /* Step 2: Verify erase succeeded */
    TEST_ASSERT_TRUE(verify_erased(test_addr, len));

    /* Step 3: Write data */
    TEST_ASSERT_EQUAL(BSP_FLASH_OK,
                      BSP_FLASH_WriteBuffer(test_addr, test_pattern, len));

    /* Step 4: Verify written data */
    TEST_ASSERT_TRUE(verify_data(test_addr, test_pattern, len));
}

/**
  * @brief  Test: write at a page boundary
  */
void test_flash_write_boundary(void)
{
    /* Start writing from 16 bytes before the end of the test page */
    uint32_t test_addr = BSP_FLASH_TEST_ADDR + BSP_FLASH_PAGE_SIZE - 16U;
    const uint8_t data[] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
    };
    uint32_t len = sizeof(data);

    TEST_ASSERT_EQUAL(BSP_FLASH_OK,
                      BSP_FLASH_WriteBuffer(test_addr, data, len));

    uint8_t buf[16];
    BSP_FLASH_ReadBuffer(test_addr, buf, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, buf, len);
}

/**
  * @brief  Test: invalid parameters should return BSP_FLASH_INVALID_PARAM
  */
void test_flash_invalid_params(void)
{
    /* Invalid page number */
    TEST_ASSERT_EQUAL(BSP_FLASH_INVALID_PARAM,
                      BSP_FLASH_ErasePage(128)); /* Out of range */
    TEST_ASSERT_EQUAL(BSP_FLASH_INVALID_PARAM,
                      BSP_FLASH_ErasePage(0xFFFFFFFF));

    /* Invalid address: Bank 1 address */
    TEST_ASSERT_EQUAL(BSP_FLASH_INVALID_PARAM,
                      BSP_FLASH_Write(0x08000000, 0x12345678));

    /* Unaligned address */
    TEST_ASSERT_EQUAL(BSP_FLASH_INVALID_PARAM,
                      BSP_FLASH_Write(BSP_FLASH_TEST_ADDR + 1, 0x12345678));

    /* NULL pointer */
    TEST_ASSERT_EQUAL(BSP_FLASH_INVALID_PARAM,
                      BSP_FLASH_WriteBuffer(BSP_FLASH_TEST_ADDR, NULL, 100));

    /* Length = 0 and address within Bank 2 — range check will reject length=0 */
    TEST_ASSERT_EQUAL(BSP_FLASH_INVALID_PARAM,
                      BSP_FLASH_WriteBuffer(BSP_FLASH_TEST_ADDR, test_pattern, 0));

    /* Out-of-bounds write (beyond Bank 2) */
    TEST_ASSERT_EQUAL(BSP_FLASH_INVALID_PARAM,
                      BSP_FLASH_Write(0x08080000, 0x12345678));
}

/**
  * @brief  Test: ReadBuffer and Read64 value consistency
  */
void test_flash_read_buffer(void)
{
    uint32_t test_addr = BSP_FLASH_TEST_ADDR;
    uint64_t test_val = 0xFEDCBA9876543210ULL;

    /* Write first, then read */
    TEST_ASSERT_EQUAL(BSP_FLASH_OK,
                      BSP_FLASH_Write(test_addr, test_val));

    /* Read64 */
    uint64_t val64 = BSP_FLASH_Read64(test_addr);
    TEST_ASSERT_EQUAL_UINT64(test_val, val64);

    /* ReadBuffer reads 8 bytes, manually assemble into uint64_t */
    uint8_t buf[8];
    BSP_FLASH_ReadBuffer(test_addr, buf, 8);

    uint64_t val_from_buf = 0;
    for (int i = 0; i < 8; i++) {
        val_from_buf |= ((uint64_t)buf[i]) << (i * 8);
    }
    TEST_ASSERT_EQUAL_UINT64(test_val, val_from_buf);
}

/* ==========================================================================*/
/* Test entry                                                                 */
/* ==========================================================================*/

void bsp_flash_run_tests(void)
{
    BSP_Flash_Status_t status;

    /* Initialize Flash */
    status = BSP_FLASH_Init();
    if (status != BSP_FLASH_OK) {
        /* Initialization failed: output error message and exit */
        UNITY_OUTPUT_CHAR('F');
        UNITY_OUTPUT_CHAR('L');
        UNITY_OUTPUT_CHAR('A');
        UNITY_OUTPUT_CHAR('S');
        UNITY_OUTPUT_CHAR('H');
        UNITY_OUTPUT_CHAR(' ');
        UNITY_OUTPUT_CHAR('I');
        UNITY_OUTPUT_CHAR('N');
        UNITY_OUTPUT_CHAR('I');
        UNITY_OUTPUT_CHAR('T');
        UNITY_OUTPUT_CHAR(' ');
        UNITY_OUTPUT_CHAR('F');
        UNITY_OUTPUT_CHAR('A');
        UNITY_OUTPUT_CHAR('I');
        UNITY_OUTPUT_CHAR('L');
        UNITY_OUTPUT_CHAR('E');
        UNITY_OUTPUT_CHAR('D');
        UNITY_OUTPUT_CHAR('\r');
        UNITY_OUTPUT_CHAR('\n');
        return;
    }

    UNITY_BEGIN();

    RUN_TEST(test_flash_erase_page);
    RUN_TEST(test_flash_write_doubleword);
    RUN_TEST(test_flash_write_buffer);
    RUN_TEST(test_flash_write_unaligned);
    RUN_TEST(test_flash_erase_then_write);
    RUN_TEST(test_flash_write_boundary);
    RUN_TEST(test_flash_invalid_params);
    RUN_TEST(test_flash_read_buffer);

    UNITY_END();
}
