/**
 ******************************************************************************
 * @file    modbus.h
 * @brief   Modbus RTU Slave 中间层，使用 UART3 + DMA + IDLE 中断
 ******************************************************************************
 */

#ifndef __MODBUS_H
#define __MODBUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
 * 可配置参数
 *----------------------------------------------------------------------------*/

/** @brief Modbus 从站地址 */
#define MODBUS_SLAVE_ADDRESS        1

/** @brief 保持寄存器数量 */
#define MODBUS_HOLDING_REG_COUNT    200

/** @brief 输入寄存器数量 */
#define MODBUS_INPUT_REG_COUNT      100

/** @brief UART 接收缓冲区大小（必须 ≥ 256，Modbus ADU 上限） */
#define MODBUS_RX_BUF_SIZE          256

/*------------------------------------------------------------------------------
 * Modbus 异常码
 *----------------------------------------------------------------------------*/

#define MODBUS_EXC_ILLEGAL_FUNCTION       0x01
#define MODBUS_EXC_ILLEGAL_DATA_ADDRESS   0x02
#define MODBUS_EXC_ILLEGAL_DATA_VALUE     0x03
#define MODBUS_EXC_SLAVE_DEVICE_FAILURE   0x04

/*------------------------------------------------------------------------------
 * 公共 API
 *----------------------------------------------------------------------------*/

/**
 * @brief  初始化 Modbus 模块（在 RTOS 调度器启动前调用）
 * @note   创建信号量，清零寄存器，启动 DMA 接收
 */
void modbus_init(void);

/**
 * @brief  Modbus RTU 任务函数（FreeRTOS 任务入口）
 * @note   阻塞等待信号量 → 处理帧 → 发送回复 → 循环
 * @param  argument  未使用
 */
void modbus_task(void const *argument);

/**
 * @brief  ISR 通知回调：当 USART3 收到完整 Modbus 帧时调用
 * @note   在 USART3_IRQHandler 中调用，释放信号量唤醒 Modbus 任务
 * @param  size  接收到的字节数
 */
void modbus_rx_notify(uint16_t size);

/*------------------------------------------------------------------------------
 * 寄存器读写 API（应用层调用）
 *----------------------------------------------------------------------------*/

int16_t modbus_read_holding_register(uint16_t addr);
void    modbus_write_holding_register(uint16_t addr, int16_t value);

int16_t modbus_read_input_register(uint16_t addr);
void    modbus_write_input_register(uint16_t addr, int16_t value);

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_H */
