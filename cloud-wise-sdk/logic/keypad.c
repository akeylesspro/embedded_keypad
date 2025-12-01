

#include "monitor.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "hal/hal_boards.h"
#include "logic/peripherals.h"
#include "ble/ble_services_manager.h"

#include "nrfx_log.h"
#include "nrf_log_ctrl.h"

#include "keypad.h"
#include "systemType.h"


#define SCAN_RATE_MSEC 40
#define KEYPAD_LONG_PRESS_THRESHOLD 2500

#define KEY_PRESS_BEEP_DURATION_MS 50

QueueHandle_t keypad_message_queue;

extern KeyState key_state;
extern SecurityState security_state;

static void keypad_sendkey(uint16_t press_time, uint8_t key, bool key_release);
static void HandleStatusLedBlinking(void);

void keypad_init_task(void)
{
  keypad_message_queue = xQueueCreate(1, sizeof(KeypadMessage) );
}

void keypad_thread(void *pvParameters)
{
  uint32_t pin_value1;
  uint32_t pin_value2;
  uint32_t key_press_time = 0;
  uint32_t duration;  
  uint8_t last_key = 0;
  bool key_pressed;
  bool long_key_found;

  while (1)
  {
    monitor_task_set(TASK_MONITOR_BIT_KEYPAD);

    vTaskDelay(SCAN_RATE_MSEC);
    
    HandleStatusLedBlinking();

    ////////////////////
    // Keypad Scaning //
    ////////////////////

    // scan step 1
    nrf_gpio_pin_set(HAL_BUTTON_KEY1);
    nrf_gpio_pin_set(HAL_BUTTON_KEY2);

    pin_value1 = nrf_gpio_pin_read(HAL_BUTTON_KEY3);
    pin_value2 = nrf_gpio_pin_read(HAL_BUTTON_KEY4);

    key_pressed = true;

    if (pin_value1 == 0)
    {
      // NRF_LOG_INFO("Key *");
      last_key = 6;

      if (key_press_time == 0)
      {
        key_press_time = xTaskGetTickCount();
        keypad_sendkey(0, last_key, false);
      }
    }
    else if (pin_value2 == 0)
    {
      // NRF_LOG_INFO("Key 5");
      last_key = 5;      

      if (key_press_time == 0)
      {
        
        // simulate unexpected reset:
        // NVIC_SystemReset();

        key_press_time = xTaskGetTickCount();
        keypad_sendkey(0, last_key, false);
      }
    }
    else
    {
      // scan step 2
      nrf_gpio_pin_clear(HAL_BUTTON_KEY1);
      nrf_gpio_pin_set(HAL_BUTTON_KEY2);

      pin_value1 = nrf_gpio_pin_read(HAL_BUTTON_KEY3);
      pin_value2 = nrf_gpio_pin_read(HAL_BUTTON_KEY4);

      if (pin_value1 == 0)
      {
        // NRF_LOG_INFO("Key 1");
        last_key = 1;

        if (key_press_time == 0)
        {
          key_press_time = xTaskGetTickCount();
          keypad_sendkey(0, last_key, false);
        }
      }
      else if (pin_value2 == 0)
      {
        // NRF_LOG_INFO("Key 2");
        last_key = 2;

        if (key_press_time == 0)
        {
          key_press_time = xTaskGetTickCount();
          keypad_sendkey(0, last_key, false);
        }
      }
      else
      {
        // scan step 3
        nrf_gpio_pin_set(HAL_BUTTON_KEY1);
        nrf_gpio_pin_clear(HAL_BUTTON_KEY2);

        pin_value1 = nrf_gpio_pin_read(HAL_BUTTON_KEY3);
        pin_value2 = nrf_gpio_pin_read(HAL_BUTTON_KEY4);

        if (pin_value1 == 0)
        {
          // NRF_LOG_INFO("Key 3");
          last_key = 3;

          if (key_press_time == 0)
          {
            key_press_time = xTaskGetTickCount();
            keypad_sendkey(0, last_key, false);
          }
        }
        else if (pin_value2 == 0)
        {
          // NRF_LOG_INFO("Key 4");
          last_key = 4;

          if (key_press_time == 0)
          {
            key_press_time = xTaskGetTickCount();
            keypad_sendkey(0, last_key, false);
          }
        }
        else
        {
          // NRF_LOG_INFO("No Key");

          key_pressed = false;

          if (last_key != 0 && !long_key_found)
          {
            duration = timeDiff(xTaskGetTickCount(), key_press_time);
            keypad_sendkey(duration, last_key, true);

            NRF_LOG_INFO("Key %d %dms", last_key, duration);

            last_key = 0;
            key_press_time = 0;
          }
        }
      }
    }

    if (last_key != 0 && key_pressed && !long_key_found)
    {
      // NRF_LOG_INFO("Key Pressed");
      duration = timeDiff(xTaskGetTickCount(), key_press_time);

      if (duration >= KEYPAD_LONG_PRESS_THRESHOLD)
      {        
        keypad_sendkey(duration, last_key, true);

        NRF_LOG_INFO("Long Key %d %dms", last_key, duration);        
        long_key_found = true;
      }
    }

    if (!key_pressed)
    {
      key_press_time = 0;
      last_key = 0;
      long_key_found = false;
    }

    NRF_LOG_FLUSH();
  }
}

static void keypad_sendkey(uint16_t press_time, uint8_t key, bool key_release)
{
  KeypadMessage msg;
  bool ret;

  msg.time_ms = press_time;
  msg.is_long_key = press_time >= KEYPAD_LONG_PRESS_THRESHOLD ? 1 : 0;
  msg.key_release = key_release ? 1 : 0;
  msg.key = key;
  NRF_LOG_INFO("msg.key_release %d ", msg.key_release);     
  ret = xQueueSend( keypad_message_queue, &msg, 1);
}

bool keypad_receive_key(KeypadMessage* msg)
{
  bool ret;

  ret = xQueueReceive(keypad_message_queue, msg, 1);

  return ret;
}
//bool ignition_flag_K_pri =0;
static void HandleStatusLedBlinking(void)
{
  static uint32_t armed_state_blink_time = 0;
  static uint32_t garage_state_blink_time = 0;

  bool ignition_flag = peripheral_is_switch_ignition_on();
 // if(ignition_flag && ignition_flag_K_pri ==0) NRF_LOG_INFO("ignition_on - led blinking");     
 // ignition_flag_K_pri=ignition_flag;

  switch (security_state)
  {
    case SECURITY_STATE_DISARMED:
      peripheral_set_led(HAL_LED_STATUS, false);
      if (ignition_flag)
        peripheral_set_led(HAL_LED_STATUS, false);
      else
        peripheral_set_led(HAL_LED_STATUS, true);
      armed_state_blink_time = 0;
      garage_state_blink_time = 0;
      break;

      /*
    case SECURITY_STATE_NEW_PASSWORD:
      peripheral_set_led(HAL_LED_STATUS, true);

      armed_state_blink_time = 0;
      garage_state_blink_time = 0;
      break;
      */

   case SECURITY_STATE_ARMED:

    armed_state_blink_time += KEY_PRESS_BEEP_DURATION_MS;

    if (key_state == KEYSTATE_LOCKED)
    {
      if ( armed_state_blink_time <= 200 ){
#ifndef hiddenKeyboard                                    //Ran add this filter to stop led blinking and to keep it in off 09/08/25
        peripheral_set_led(HAL_LED_STATUS, true);
#endif
         
        }
      else if ( armed_state_blink_time < 400 ){

        peripheral_set_led(HAL_LED_STATUS, false);
        

        }
      else
        armed_state_blink_time = 0;
    }
    else
    {
      if ( armed_state_blink_time <= 1900 ){

        peripheral_set_led(HAL_LED_STATUS, false);
        
        }
      else if ( armed_state_blink_time < 2000 )
      {
#ifndef hiddenKeyboard                                    //Ran add this filter to stop led blinking and to keep it in off 09/08/25
        peripheral_set_led(HAL_LED_STATUS, true);
#endif
      
      }
      else{
        armed_state_blink_time = 0;
        
        }
    }

    garage_state_blink_time = 0;
    break;


    /*
   case SECURITY_STATE_GARAGE:

    garage_state_blink_time += KEY_PRESS_BEEP_DURATION_MS;

    if ( garage_state_blink_time < 1000 )
      peripheral_set_led(HAL_LED_STATUS, true);
    else if ( garage_state_blink_time < 2000 )
    {
      peripheral_set_led(HAL_LED_STATUS, false);
    }
    else
      garage_state_blink_time = 0;

    armed_state_blink_time = 0;
    break;
    */



  }
}
