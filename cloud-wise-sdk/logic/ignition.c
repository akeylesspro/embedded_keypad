// #include "ble_services_manager.h"

#include "FreeRTOS.h"
#include "event_groups.h"

#include "main.h"
#include "ignition.h"
#include "ble/ble_leach.h"

#include "logic/peripherals.h"
#include "nrfx_log.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint32_t ignition_value = 0x1234;

EventGroupHandle_t event_ignition_characteristic_changed;


void ignition_command_deprecated(uint8_t *value_buffer, uint8_t value_length)
{
    uint32_t value = 0;

    if (value_length > 0) {
        if (value_length > 4) {
            value_length = 4;
        }

        switch (value_length) {
        case 1:
            value = value_buffer[0];
            break;

        case 2:
            value += value_buffer[0] << 8;
            value += value_buffer[1];
            break;

        case 3:
            value += value_buffer[0] << 16;
            value += value_buffer[1] << 8;
            value += value_buffer[2];
            break;

        case 4:
            value += value_buffer[0] << 24;
            value += value_buffer[1] << 16;
            value += value_buffer[2] << 8;
            value += value_buffer[3];
            break;
        }
    }

    NRF_LOG_INFO("ignition characteristic value: %08xh: ", value);

    if (value & (1<<IGNITION_BIT_SWITCH_EN) )
    {
      if (value & (1<<IGNITION_BIT_SWITCH) )
      {
        send_event_from_isr( event_ignition_characteristic_changed, EVENT_IGNITION_ON );
        NRF_LOG_INFO("send event switch on");
      }
      else
      {
        send_event_from_isr( event_ignition_characteristic_changed, EVENT_IGNITION_OFF );
        NRF_LOG_INFO("send event switch off");
      }
    }

    if (value & (1<<IGNITION_BIT_RELAY_STATE_EN) )
    {
      if (value & (1<<IGNITION_BIT_RELAY_STATE) )
      {
        send_event_from_isr( event_ignition_characteristic_changed, EVENT_IGNITION_NORMALY_OPEN );
        NRF_LOG_INFO("send event relay normaly open");
      }
      else
      {
        send_event_from_isr( event_ignition_characteristic_changed, EVENT_IGNITION_NORMALY_CLOSE );
        NRF_LOG_INFO("send event relay normaly close");
      }
    }

    if (value & (1<<IGNITION_BIT_CMD_DEFAULT_PARAMS) )
    {
        send_event_from_isr( event_ignition_characteristic_changed, EVENT_IGNITION_SET_DEFAULT );
        NRF_LOG_INFO("send event setting default parameters");
    }

    if (value & (1<<IGNITION_BIT_CMD_RESET) )
    {
      vTaskDelay(1000);
      NVIC_SystemReset();
    }


    /*
    // change relay here
    // ..
    ignition_value = value;

    if (ignition_value & 0x00000001) {
        peripherals_ignition_on();
    } else {
        peripherals_ignition_off();
    }
    */    
}

uint32_t get_ignition_value(void)
{
    // semaphore here
    // ..

    return ignition_value;
}
