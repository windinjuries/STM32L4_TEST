#include "host_system_info.h"

#include "main.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define HOST_RX_RING_SIZE 256U
#define HOST_LINE_SIZE    192U

static volatile uint8_t host_rx_ring[HOST_RX_RING_SIZE];
static volatile uint16_t host_rx_head;
static volatile uint16_t host_rx_tail;
static volatile bool host_rx_overflow;

static char host_line[HOST_LINE_SIZE];
static uint16_t host_line_length;
static bool host_line_overflow;

static host_system_info_t host_snapshot;
static bool host_snapshot_valid;
static uint32_t host_snapshot_tick;

static char *trim_text(char *text)
{
    char *end;

    while (isspace((unsigned char)*text))
    {
        text++;
    }

    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
    {
        end--;
    }
    *end = '\0';

    return text;
}

static bool parse_percent(const char *text, float *value)
{
    char *end;
    float parsed;

    parsed = strtof(text, &end);
    if (end == text)
    {
        return false;
    }

    while (isspace((unsigned char)*end))
    {
        end++;
    }
    if (*end == '%')
    {
        end++;
    }
    if (*end != '\0')
    {
        return false;
    }
    if (parsed < 0.0f || parsed > 100.0f)
    {
        return false;
    }

    *value = parsed;
    return true;
}

static bool parse_host_line(char *line)
{
    char *cursor;
    char *separator;
    char *token;
    float cpu = 0.0f;
    float memory = 0.0f;
    char time_value[sizeof(host_snapshot.time)] = {0};
    bool saw_system = false;
    bool saw_cpu = false;
    bool saw_memory = false;
    bool saw_time = false;

    separator = strchr(line, ',');
    if (separator == NULL)
    {
        return false;
    }
    *separator = '\0';
    if (strcmp(trim_text(line), "SYS") != 0)
    {
        return false;
    }
    saw_system = true;

    cursor = separator + 1;
    while (*cursor != '\0')
    {
        separator = strchr(cursor, ',');
        if (separator != NULL)
        {
            *separator = '\0';
        }
        token = trim_text(cursor);
        if (strncmp(token, "cpu=", 4) == 0)
        {
            saw_cpu = parse_percent(token + 4, &cpu);
            if (!saw_cpu)
            {
                return false;
            }
        }
        else if (strncmp(token, "mem=", 4) == 0 ||
                 strncmp(token, "memory=", 7) == 0)
        {
            const char *value = (token[3] == '=') ? token + 4 : token + 7;
            saw_memory = parse_percent(value, &memory);
            if (!saw_memory)
            {
                return false;
            }
        }
        else if (strncmp(token, "time=", 5) == 0)
        {
            const char *value = trim_text(token + 5);
            if (*value == '\0' || strlen(value) >= sizeof(time_value))
            {
                return false;
            }
            strcpy(time_value, value);
            saw_time = true;
        }

        if (separator == NULL)
        {
            break;
        }
        cursor = separator + 1;
    }

    if (!saw_system || !saw_cpu || !saw_memory || !saw_time)
    {
        return false;
    }

    host_snapshot.cpu_percent = cpu;
    host_snapshot.memory_percent = memory;
    strcpy(host_snapshot.time, time_value);
    host_snapshot_tick = HAL_GetTick();
    host_snapshot_valid = true;
    return true;
}

static bool read_rx_byte(uint8_t *byte)
{
    if (host_rx_tail == host_rx_head)
    {
        return false;
    }

    *byte = host_rx_ring[host_rx_tail];
    host_rx_tail = (uint16_t)((host_rx_tail + 1U) % HOST_RX_RING_SIZE);
    return true;
}

void host_system_info_on_usb_rx(const uint8_t *data, uint32_t length)
{
    if (data == NULL)
    {
        return;
    }

    for (uint32_t index = 0; index < length; index++)
    {
        uint16_t next_head = (uint16_t)((host_rx_head + 1U) % HOST_RX_RING_SIZE);
        if (next_head == host_rx_tail)
        {
            host_rx_overflow = true;
            return;
        }

        host_rx_ring[host_rx_head] = data[index];
        host_rx_head = next_head;
    }
}

bool host_system_info_process(void)
{
    bool updated = false;
    uint8_t byte;

    if (host_rx_overflow)
    {
        host_rx_overflow = false;
        host_line_overflow = true;
        host_line_length = 0;
    }

    while (read_rx_byte(&byte))
    {
        if (byte == '\n')
        {
            if (!host_line_overflow && host_line_length > 0)
            {
                host_line[host_line_length] = '\0';
                if (parse_host_line(host_line))
                {
                    updated = true;
                }
            }
            host_line_length = 0;
            host_line_overflow = false;
        }
        else if (byte != '\r')
        {
            if (host_line_length < (HOST_LINE_SIZE - 1U))
            {
                host_line[host_line_length++] = (char)byte;
            }
            else
            {
                host_line_overflow = true;
            }
        }
    }

    return updated;
}

bool host_system_info_get(host_system_info_t *out)
{
    if (out == NULL || !host_snapshot_valid)
    {
        return false;
    }

    *out = host_snapshot;
    return true;
}

uint32_t host_system_info_age_ms(void)
{
    if (!host_snapshot_valid)
    {
        return UINT32_MAX;
    }

    return HAL_GetTick() - host_snapshot_tick;
}
