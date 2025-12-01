#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Serial Number String (Device Information Service)
#define SERIAL_NUMBER_WIDTH 9

extern char device_serial_number[SERIAL_NUMBER_WIDTH];

void peripherals_init(void);

uint8_t peripherals_read_battery_level(void);

uint8_t  peripherals_read_temperature(void);
uint16_t peripherals_read_vdd(void);

void peripherals_toggle_leds(void);

void peripherals_ignition_on(void);
void peripherals_ignition_off(void);

void peripheral_turn_on_all_led(bool including_astrix_led);
void peripheral_turn_off_all_led(bool including_astrix_led);
void peripheral_set_led(uint32_t pin_number, bool on);
bool peripheral_is_switch_ignition_on(void);
void peripheral_toggle_led(uint32_t pin_number);

