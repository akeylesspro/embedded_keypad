#include "ble_leach.h"

#include "nrf_log_ctrl.h"
#include "nrf_ringbuf.h"
#include "nrfx_atomic.h"

#define NRF_LOG_MODULE_NAME ble_leach
#define NRF_LOG_LEVEL BLE_LEACH_BLE_LOG_LEVEL
#include "nrfx_log.h"
NRF_LOG_MODULE_REGISTER();

#define BLE_UUID_COMMAND_CHARACTERISTIC 0xB6F3
#define BLE_UUID_IGNITION_CHARACTERISTIC 0x5401

// Cloud-Wise Leach 128-bit base UUID B94D0000-F943-11E7-8C3F-9A214CF093AE
#define ILEACH_BASE_UUID                                                                                                                   \
    {                                                                                                                                      \
        0x83, 0xa8, 0x74, 0x7a, 0x52, 0xb5, 0x4d, 0x8c, 0x6b, 0x6c, 0x3f, 0xb6, 0x76, 0x36, 0xc7, 0x21,                                    \
    }
