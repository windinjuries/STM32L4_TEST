#ifndef HOST_SYSTEM_INFO_H
#define HOST_SYSTEM_INFO_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float cpu_percent;
    float memory_percent;
    char time[32];
} host_system_info_t;

void host_system_info_on_usb_rx(const uint8_t *data, uint32_t length);
bool host_system_info_process(void);
bool host_system_info_get(host_system_info_t *out);
uint32_t host_system_info_age_ms(void);

#endif /* HOST_SYSTEM_INFO_H */
