#pragma once

#include "ble.h"
#include "ble_leach.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "sdk_config.h"

#include <stdbool.h>
#include <stdint.h>

/*
typedef struct {
    uint8_t message[COMMAND_CHAR_MAX_LEN];
    uint8_t size;
} BleMessage;
*/

typedef enum 
{
  PERIPHERAL_TYPE_UNKNOWN,
  PERIPHERAL_TYPE_LEACH,
  PERIPHERAL_TYPE_CANBUS_LEACH,
  PERIPHERAL_TYPE_MODEM,
  PERIPHERAL_TYPE_ISTICKER,
  PERIPHERAL_TYPE_NEW,
} PeripheralType;


void ble_thread(void *pvParameters);
void init_ble_task(void);
PeripheralType is_peripheral_recognized(ble_gap_addr_t* search_address);
void print_address(char* text, ble_gap_addr_t *address);
void activate_bonding_state(bool bonding_state);
void de_activate_bonding_state(void);
void disconnect_and_delete_all_peripheral(bool including_address_list_in_eprom);

bool connections_state_lock(uint32_t wait_ms);
void connections_state_unlock(void);
void save_bondings(void);
void set_ignition_state( uint32_t ignition_state, uint16_t connection_handle );
uint8_t blink_bonded_device_count(bool show_bonded_count, bool display_key_led);
void setLeachFound(uint8_t found);
