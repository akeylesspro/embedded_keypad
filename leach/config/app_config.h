#pragma once

#include "shared_app_config.h"

// #define DEBUG_MODE

// <<< Use Configuration Wizard in Context Menu >>>\n

// <h> LEACH

// <o> BLE_LEACH_BLE_OBSERVER_PRIOs
// <i> Priority with which BLE events are dispatched to the AM Service.

#ifndef BLE_LEACH_BLE_OBSERVER_PRIO
#define BLE_LEACH_BLE_OBSERVER_PRIO 2
#endif

// <o> BLE_LEACH_BLE_LOG_LEVEL  - Default Log level

// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug

#ifndef BLE_LEACH_BLE_LOG_LEVEL
#define BLE_LEACH_BLE_LOG_LEVEL NRFX_LOG_LEVEL_INFO
#endif

#ifndef NRF_QUEUE_ENABLED
#define NRF_QUEUE_ENABLED 1
#endif


#define NRF_SDH_BLE_CENTRAL_LINK_COUNT 3
#define NRF_SDH_BLE_PERIPHERAL_LINK_COUNT 1
#define NRF_SDH_BLE_TOTAL_LINK_COUNT 3


// <e> NRF_BLE_GQ_ENABLED - nrf_ble_gq - BLE GATT Queue Module
//==========================================================
#ifndef NRF_BLE_GQ_ENABLED
#define NRF_BLE_GQ_ENABLED 1
#endif
// <o> NRF_BLE_GQ_DATAPOOL_ELEMENT_SIZE - Default size of a single element in the pool of memory objects. 
#ifndef NRF_BLE_GQ_DATAPOOL_ELEMENT_SIZE
#define NRF_BLE_GQ_DATAPOOL_ELEMENT_SIZE 20
#endif

// <o> NRF_BLE_GQ_DATAPOOL_ELEMENT_COUNT - Default number of elements in the pool of memory objects. 
#ifndef NRF_BLE_GQ_DATAPOOL_ELEMENT_COUNT
#define NRF_BLE_GQ_DATAPOOL_ELEMENT_COUNT 8
#endif

// <o> NRF_BLE_GQ_GATTC_WRITE_MAX_DATA_LEN - Maximal size of the data inside GATTC write request (in bytes). 
#ifndef NRF_BLE_GQ_GATTC_WRITE_MAX_DATA_LEN
#define NRF_BLE_GQ_GATTC_WRITE_MAX_DATA_LEN 16
#endif

// <o> NRF_BLE_GQ_GATTS_HVX_MAX_DATA_LEN - Maximal size of the data inside GATTC notification or indication request (in bytes). 
#ifndef NRF_BLE_GQ_GATTS_HVX_MAX_DATA_LEN
#define NRF_BLE_GQ_GATTS_HVX_MAX_DATA_LEN 16
#endif

// </e>

// <o> NRF_BLE_GQ_QUEUE_SIZE - Queue size for BLE GATT Queue module. 
#ifndef NRF_BLE_GQ_QUEUE_SIZE
#define NRF_BLE_GQ_QUEUE_SIZE 6
#endif

#ifndef NRF_BLE_SCAN_BUFFER
#define NRF_BLE_SCAN_BUFFER 31
#endif

#ifndef NRF_BLE_SCAN_ENABLED
#define NRF_BLE_SCAN_ENABLED 1
#endif

#ifndef NRF_BLE_SCAN_SCAN_INTERVAL
#define NRF_BLE_SCAN_SCAN_INTERVAL 160
#endif

#ifndef NRF_BLE_SCAN_SCAN_WINDOW
#define NRF_BLE_SCAN_SCAN_WINDOW 80
#endif

#ifndef NRF_BLE_SCAN_SCAN_DURATION
#define NRF_BLE_SCAN_SCAN_DURATION 0
#endif

#ifndef NRF_BLE_SCAN_SUPERVISION_TIMEOUT
#define NRF_BLE_SCAN_SUPERVISION_TIMEOUT 4000
#endif

#ifndef NRF_BLE_SCAN_MIN_CONNECTION_INTERVAL
#define NRF_BLE_SCAN_MIN_CONNECTION_INTERVAL 7.5
#endif

// <o> NRF_BLE_SCAN_MAX_CONNECTION_INTERVAL - Determines maximum connection interval in milliseconds. 
#ifndef NRF_BLE_SCAN_MAX_CONNECTION_INTERVAL
#define NRF_BLE_SCAN_MAX_CONNECTION_INTERVAL 30
#endif

// <o> NRF_BLE_SCAN_SLAVE_LATENCY - Determines the slave latency in counts of connection events. 
#ifndef NRF_BLE_SCAN_SLAVE_LATENCY
#define NRF_BLE_SCAN_SLAVE_LATENCY 0
#endif

#ifndef NRF_BLE_SCAN_FILTER_ENABLE
#define NRF_BLE_SCAN_FILTER_ENABLE 1
#endif

// scan filters
#define NRF_BLE_SCAN_NAME_CNT 0
#define NRF_BLE_SCAN_UUID_CNT 1
#define NRF_BLE_SCAN_ADDRESS_CNT 4

#ifndef NRF_BLE_SCAN_NAME_MAX_LEN
#define NRF_BLE_SCAN_NAME_MAX_LEN 32
#endif




// </h>

// <<< end of configuration section >>>


#define RUPTELA_SERVICE_SIMULATION


#define timeDiff(a, b) ((int32_t)(a) - (int32_t)(b))

// white list 
#ifdef CONSTANT_MAC_ADDRESS
  #define MAX_TARGET_ADDRESSES 2
#else
  #define MAX_TARGET_ADDRESSES 4
#endif