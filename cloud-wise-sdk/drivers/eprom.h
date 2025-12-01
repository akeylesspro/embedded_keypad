#pragma once

#include <stdint.h>

#define READ_COMMAND 0x03
#define WRITE_COMMAND 0x02
#define WRITE_ENABLE_COMMAND 0x06
#define WRITE_DISABLE_COMMAND 0x04

#define DEVICE_ADDRESS 0xFC
#define MANUFACTURE_ADDRESS 0xFA

void     eprom_init(void);
uint16_t eprom_read_manufacture_id(void);
uint32_t eprom_read_device_id(void);
uint8_t *eprom_read(uint8_t *buffer, uint8_t address, uint8_t length);
void     eprom_write(uint8_t *buffer, uint8_t address, uint8_t length);