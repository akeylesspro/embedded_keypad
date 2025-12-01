#pragma once

#include <stdint.h>

void     ignition_command(uint8_t *value_buffer, uint8_t value_length);
uint32_t get_ignition_value(void);


#define EVENT_IGNITION_ON               0x0001
#define EVENT_IGNITION_OFF              0x0002
#define EVENT_IGNITION_NORMALY_OPEN     0x0004
#define EVENT_IGNITION_NORMALY_CLOSE    0x0008
#define EVENT_IGNITION_SET_DEFAULT      0x0010
#define EVENT_IGNITION_KEEP_ALIVE       0x8000
