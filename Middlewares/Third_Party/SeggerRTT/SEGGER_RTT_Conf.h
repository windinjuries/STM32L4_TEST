/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*        SEGGER RTT * Real Time Transfer for embedded targets        *
*                                                                    *
**********************************************************************
---------------------------END-OF-HEADER------------------------------
Purpose : RTT configuration for STM32L475VET6 (Cortex-M4F).
----------------------------------------------------------------------
*/

#ifndef SEGGER_RTT_CONF_H
#define SEGGER_RTT_CONF_H

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/* Up buffer 0 (Terminal output T->H): 1024 字节，适合日志输出 */
#define BUFFER_SIZE_UP      (1024)

/* Down buffer 0 (Terminal input H->T): 16 字节，支持少量按键输入 */
#define BUFFER_SIZE_DOWN    (16)

/* 最大 RTT 通道数：3 个 up (Terminal, 预留日志, 预留) + 3 个 down */
#define SEGGER_RTT_MAX_NUM_UP_BUFFERS      (3)
#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS    (3)

/* 默认模式：NO_BLOCK_SKIP — 缓冲区满时不阻塞，跳过数据
 * 如需可靠日志，可改为 SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL */
#define SEGGER_RTT_MODE_DEFAULT    SEGGER_RTT_MODE_NO_BLOCK_SKIP

/* printf 缓冲大小：128 字节，减少 RTT Write 调用次数 */
#define SEGGER_RTT_PRINTF_BUFFER_SIZE    (128)

/* Cortex-M4 不需要 cache line 对齐 */
#define SEGGER_RTT_CPU_CACHE_LINE_SIZE   (0)

#endif /* SEGGER_RTT_CONF_H */

/*************************** End of file ****************************/
