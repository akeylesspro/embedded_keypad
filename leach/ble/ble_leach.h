#pragma once

#include "ble.h"
#include "ble_srv_common.h"


#include "nrf_sdh_ble.h"
#include "sdk_config.h"

#include <stdbool.h>
#include <stdint.h>

#define COMMAND_CHAR_MAX_LEN 64
#define ACC_CHAR_MAX_LEN 16
#define EVENTS_CHAR_MAX_LEN 64
#define MEASURE_CHAR_MAX_LEN 16
#define STATUS_CHAR_MAX_LEN 32
#define IGNITION_CHAR_MAX_LEN 4

#define IGNITION_BIT_SWITCH           0
#define IGNITION_BIT_SWITCH_EN        1
#define IGNITION_BIT_RELAY_STATE      2
#define IGNITION_BIT_RELAY_STATE_EN   3

#define IGNITION_BIT_CMD_CLEAR_ALL         8
#define IGNITION_BIT_CMD_DEFAULT_PARAMS    9
#define IGNITION_BIT_CMD_RESET             10


