#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ble_gap.h"
#include "ble_task.h"
#include "ble_dis_c.h"

void ble_services_init_0(void);
void ble_services_init_peripheral(void);
void ble_services_init_central(void);

void ble_services_advertising_start(void);
void decode_peripheral_device_by_dis(uint8_t* value, uint8_t length, ble_dis_c_char_type_t char_type);


bool ble_services_is_connected(void);

// scan functions //
void scan_start(bool force);
void scan_stop(bool force);
void scan_init_by_white_list(void);
void scan_init_by_uuid(bool with_mac_address_filter);

typedef struct
{
  uint32_t ignition_state;
  uint32_t connection_time;
  ble_gap_addr_t peer_address;  
  int16_t conn_handler;
  PeripheralType peripheral_type;
  bool is_connected;  
  bool is_ignition_state_changed;  
  
} 
ble_peripheral_info_t;
 

