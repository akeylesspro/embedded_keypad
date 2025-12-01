#pragma once

#include "hal/hal_data_types.h"

#ifdef DEBUG_MODE
  #define USE_SWITCH_IN_GARAGE_MODE
  #define LOCKED_STATE_TIME_SEC (1*60)
  #define DISARM_WITHOUT_DRIVING_SEC (1*60)
#else
  #define USE_SWITCH_IN_GARAGE_MODE
  #define LOCKED_STATE_TIME_SEC (3*60)
  #define DISARM_WITHOUT_DRIVING_SEC (3*60) //Ran Change from 2*60 to 3*60    03/06/2025
#endif

#define CONNECTION_TEST_TIME_IN_SEC (10)

typedef struct {
    uint8_t year;
    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t seconds;
} Calendar;

void monitor_thread(void *arg);

void     InitClock(void);
void     SetTimeFromString(uint8_t *date_str, uint8_t *time_str);
void     GetSystemTime(Calendar *c);
void     SetClockString(uint8_t *buffer);
void     set_leach_state(void);
bool     send_ignition_state_to_peripheral(uint32_t value, uint8_t conn_handler );
bool     send_log_to_peripheral(uint8_t* log_message, uint8_t conn_handler );
void     back_to_armed_state(bool with_buzzer);
void     send_log(uint8_t* log_message);
bool     check_all_peripheral_connected(void);
uint32_t GetTimeStampFromDate(void);


void failure_buzzer(void);
void success_buzzer(void);
void lock_buzzer(void);


typedef struct {
    uint32_t last_monitor_time;
    uint16_t task_bits;
} MonitorState;

#define MONITOR_TEST_TIME 7 //30

#define KEYPADLIGHT_AFTER_KEY_PRESS_SEC 6
#define DISARM_GARAGE_TIMEOUT_SEC (1*30)
#define DISARM_NEW_PASSWORD_TIMEOUT_SEC (1*30)

#define WRONG_PASSWORD_TRIES 3

#define TASK_MONITOR_NUM 3
#define TASK_MONITOR_BIT_BLE 0
#define TASK_MONITOR_BIT_KEYPAD 1
#define TASK_MONITOR_BIT_MONITOR 2

#define NRF_QUEUE 1


#define IGNITION_STATE_SWITCH_BIT         0     // 1 - disarmed, 0 - armed
#define IGNITION_STATE_SWITCH_ENABLE_BIT  1     // 1 - enable bit 0, 0 - disable bit 0
#define IGNITION_STATE_RELAY_MODE_BIT     2     // 1 - normaly openned, 0 - normaly closed
#define IGNITION_STATE_RELAY_ENABLE_BIT   3     // 1 - enable bit 2, 0 - disable bit 2

#define RESET_WATCHDOG                                        1
#define RESET_CLEAR_BONDING                                   2
#define RESET_BONDING_COMPLETED                               3
#define RESET_CONNECTION_COUNT_FAIL_AFTER_PASSWORD            4
#define RESET_CONNECTION_COUNT_FAIL_IN_ARM_MODE               5
#define RESET_CONNECTION_LEACH_DISCONNECTED                   6 //Ran Add it on 21/08/25
#define RESET_CONNECTION_MODEM_DISCONNECTED                   7 //Ran Add it on 21/08/25

#define MAX_CONNECTIONS 4

typedef enum 
{
  KEYSTATE_IDLE,
  KEYSTATE_ASTRIX_RECEIVED,
  KEYSTATE_MAIN_MENU,
  KEYSTATE_GARAGE_MENU,
  KEYSTATE_LOCKED,
  KEYSTATE_LAST
} KeyState;

typedef enum 
{
  SECURITY_STATE_NONE,
  SECURITY_STATE_ARMED,
  SECURITY_STATE_DISARMED,
  SECURITY_STATE_GARAGE,
  SECURITY_STATE_NEW_PASSWORD,
  SECURITY_STATE_LAST,
} SecurityState;

void monitor_task_set(uint8_t task_id);
void monitor_task_check(void);
void change_security_state(SecurityState new_state);
void reset_device(uint8_t reset_code);
