#ifndef __DEBUG_LOG_H
#define __DEBUG_LOG_H

#include <stdint.h>

/* 日志级别 */
typedef enum {
    LOG_LEVEL_NONE  = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_INFO  = 3,
    LOG_LEVEL_DEBUG = 4,
} log_level_t;

/* 初始化日志模块（使用 USART1，由 CubeMX 初始化） */
void log_init(void);

/* 设置日志输出级别（低于该级别的日志不输出） */
void log_set_level(log_level_t level);

/* 输出格式化日志，自动带换行和日志级别前缀 */
void log_printf(log_level_t level, const char *fmt, ...);

/* 直接输出字符串（不带前缀、不换行） */
void log_puts(const char *str);

/* 快捷宏 */
#define LOG_ERROR(fmt, ...)  log_printf(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)   log_printf(LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)   log_printf(LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)  log_printf(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#endif /* __DEBUG_LOG_H */
