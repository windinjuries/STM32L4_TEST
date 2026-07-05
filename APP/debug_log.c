#include "debug_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "stm32l4xx_hal.h"

/* 引用 CubeMX 初始化的 USART1 句柄 */
extern UART_HandleTypeDef huart1;

/* 当前日志级别，默认 DEBUG */
static log_level_t s_log_level = LOG_LEVEL_DEBUG;

/* 日志级别前缀字符串 */
static const char *const s_level_prefix[] = {
    [LOG_LEVEL_NONE]  = "",
    [LOG_LEVEL_ERROR] = "[E] ",
    [LOG_LEVEL_WARN]  = "[W] ",
    [LOG_LEVEL_INFO]  = "[I] ",
    [LOG_LEVEL_DEBUG] = "[D] ",
};

void log_init(void)
{
    /* USART1 已由 MX_USART1_UART_Init() 初始化，无需额外操作 */
    log_puts("\r\n========================================\r\n");
    log_puts("  Debug Log System Initialized\r\n");
    log_puts("  USART1 115200 8N1\r\n");
    log_puts("========================================\r\n");
}

void log_set_level(log_level_t level)
{
    s_log_level = level;
}

void log_puts(const char *str)
{
    if (str == NULL) return;
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

void log_printf(log_level_t level, const char *fmt, ...)
{
    char buffer[256];
    int pos = 0;

    /* 检查日志级别 */
    if (level > s_log_level || level == LOG_LEVEL_NONE || level > LOG_LEVEL_DEBUG) {
        return;
    }

    /* 添加级别前缀 */
    const char *prefix = s_level_prefix[level];
    if (prefix[0] != '\0') {
        while (*prefix && pos < (int)sizeof(buffer) - 3) {
            buffer[pos++] = *prefix++;
        }
    }

    /* 格式化用户消息 */
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buffer + pos, sizeof(buffer) - pos - 2, fmt, args);
    va_end(args);

    if (ret > 0) {
        pos += (ret < (int)(sizeof(buffer) - pos - 2)) ? ret : (int)(sizeof(buffer) - pos - 2);
    }

    /* 添加换行 */
    if (pos + 2 <= (int)sizeof(buffer)) {
        buffer[pos++] = '\r';
        buffer[pos++] = '\n';
    }

    /* 通过 USART1 发送 */
    HAL_UART_Transmit(&huart1, (uint8_t *)buffer, pos, HAL_MAX_DELAY);
}
