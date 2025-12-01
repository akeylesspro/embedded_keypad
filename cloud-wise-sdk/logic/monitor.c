#include "monitor.h"

#include "FreeRTOS.h"
#include "app_timer.h"
#include "ble/ble_services_manager.h"
#include "commands.h"
#include "configuration.h"
#include "drivers/eprom.h"
#include "drivers/buzzer.h"
#include "event_groups.h"
#include "events.h"
#include "flash_data.h"
#include "hal/hal_boards.h"
#include "hal/hal_drivers.h"
#include "logic/peripherals.h"
#include "logic/serial_comm.h"
#include "logic/state_machine.h"
#include "semphr.h"
#include "task.h"
#include "tracking_algorithm.h"
#include "hal/hal_boards.h"
#include "nrfx_log.h"
#include "nrf_log_ctrl.h"
#include "keypad.h"
#include "nrf_queue.h"
#include "Ruptela.h"

#include <stdlib.h>
#include <string.h>

extern DriverBehaviourState driver_behaviour_state;
extern IStickerErrorBits    error_bits;
extern ble_peripheral_info_t connections_array[];
extern uint8_t bonded_count;
extern bool in_bonding_state;
extern bool power_up;
extern uint32_t connected_count;
extern uint32_t time_since_2020;
extern uint8_t leach_bonded_count;
extern uint8_t modem_bonded_count;

extern bool allLedsAreOn;  //Ran add it 09/10/25

extern deviceSerialWrite device_Serial_write; //Ran add it 09/10/25

#ifdef RUPTELA_SERVICE_SIMULATION
extern Ruptela_Command ruptela_command;
#endif

static uint8_t task_keep_alive_array[TASK_MONITOR_NUM];

APP_TIMER_DEF(m_clock_timer);

#define DD(s) ((int)((s)[0] - '0') * 10 + (int)((s)[1] - '0'));

#define QUEUE_SIZE 4

NRF_QUEUE_DEF(KeypadMessage, my_queue, QUEUE_SIZE, NRF_QUEUE_MODE_NO_OVERFLOW);

static Calendar  absolute_time __attribute__((section(".non_init")));
static uint32_t  clock_counter = 0;
xSemaphoreHandle clock_semaphore;
xSemaphoreHandle watchdog_monitor_semaphore;

bool need_default = false;
bool need_refresh_scan_filter = false;
bool configuration_loaded = false;
uint8_t last_reset_code = 0;

static MonitorState monitor_state;

void  keypad_state_machine(void);
uint8_t GetDaysInMonth(uint8_t year, uint8_t month);
void    InitClock(void);
void    AddSecondsToDate(Calendar *c, uint32_t seconds);

static void change_key_state(KeyState new_state);
static bool check_switch_on_in_garage(void);
static bool check_switch_on_entering_password(void);
static uint32_t armed_state_enter_time = 0;

KeyState key_state;
SecurityState security_state;

static uint32_t disarmed_state_timeout_value = DISARM_WITHOUT_DRIVING_SEC;
static bool is_blocking_keypad_found = false;
static bool driveing_without_password_alert = false;
static uint32_t oneScondCounter =0;
static bool leachConnectedFlag[2] = {0,0};
static bool modemConnectedFlag = false;

uint8_t startScanOnOperation =0;

bool afterStudy = false;
bool needToDoReset = false;
bool stopScanDone = 0;
bool enableToConnectApp = 0;
static void clock_tick_handler(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    clock_counter++;
}

uint32_t getTick(void)
{
    uint32_t c;

    taskENTER_CRITICAL();
    c = clock_counter;
    taskEXIT_CRITICAL();

    return c;
}

void PV(uint8_t **buffer, uint8_t val)
{
    uint8_t *p = *buffer;
    uint8_t  d;

    d    = val / 10;
    p[0] = d + '0';

    d    = val % 10;
    p[1] = d + '0';

    (*buffer) += 2;
}

void PC(uint8_t **buffer, uint8_t ch)
{
    uint8_t *p = *buffer;

    p[0] = ch;
    (*buffer) += 1;
}

void SetClockString(uint8_t *buffer)
{
    uint8_t *ptr = buffer;

    xSemaphoreTake(clock_semaphore, portMAX_DELAY);

    PV(&ptr, absolute_time.day);
    PC(&ptr, '-');

    PV(&ptr, absolute_time.month);
    PC(&ptr, '-');

    PV(&ptr, absolute_time.year);
    PC(&ptr, ' ');

    PV(&ptr, absolute_time.hour);
    PC(&ptr, ':');

    PV(&ptr, absolute_time.minute);
    PC(&ptr, ':');

    PV(&ptr, absolute_time.seconds);
    PC(&ptr, ' ');

    xSemaphoreGive(clock_semaphore);
}

static uint8_t read_buffer[16 + 4];
static uint8_t write_buffer[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

static int count = 0;
static int count_x = 0;


void monitor_thread(void *arg)
{
    uint8_t *read_ptr = NULL;
    UNUSED_PARAMETER(arg);
    
    eprom_init();

    // reset buzzer
    if (power_up)
    {
      buzzer_train(1, false, true);
      buzzer_train(1, true, true);
      NRF_LOG_INFO( "power up" );
    }
    #ifdef DEBUG
    else
    {
      buzzer_train(1, true, true);
      buzzer_train(1, false, true);
      NRF_LOG_INFO( "Reset" );
    }
    #endif

    NRF_LOG_FLUSH();

    /*
    // test eprom //
    eprom_read_manufacture_id();
    eprom_read_device_id();

    // buffer size should be 4 bytes larger from the length
    read_ptr = eprom_read( read_buffer, 0xF0, 16);
    eprom_write( write_buffer, 0x10, 16);
    read_ptr = eprom_read( read_buffer, 0x10, 16);
    // end of test eprom //
    */

    ///////////////////////////
    // loading configuration //
    ///////////////////////////

    configuration_load();
    uint16_t calculated_crc = configuration_calculate_eprom_crc();
    if (calculated_crc != eprom_configuration.crc) 
    {
        configuration_reset_eprom();
        configuration_set_default();
        configuration_save_with_crc();
    }

    if (eprom_configuration.security_state == SECURITY_STATE_DISARMED)
    {
      change_security_state(SECURITY_STATE_DISARMED);
      if (eprom_configuration.garage_mode == 1)
      {
        change_key_state(KEYSTATE_GARAGE_MENU);
        change_security_state(SECURITY_STATE_GARAGE);
      }
      else 
        change_key_state(KEYSTATE_MAIN_MENU);
    }
    else
    {
      if (eprom_configuration.is_locked)
      {
        change_key_state(KEYSTATE_LOCKED);
        is_blocking_keypad_found = true;
      }
      else
        change_key_state(KEYSTATE_IDLE);

      change_security_state(SECURITY_STATE_ARMED);
    }

    last_reset_code = eprom_configuration.last_reset_code;
    eprom_configuration.last_reset_code = 0;
    eprom_configuration.garage_mode = 0;
    if (power_up)
      eprom_configuration.power_reset_count++;
    eprom_configuration.reset_count++;
    configuration_save_with_crc();

    ///////////////////////////

    configuration_load_address_list();
    configuration_print();
    vTaskDelay(100);
    configuration_loaded = true;


    // on reset, blink all leds together
    peripheral_turn_on_all_led(true);
    vTaskDelay(100);
    peripheral_turn_off_all_led(true);

    // set initial ignition state
    if (eprom_configuration.ignition_state)
      peripherals_ignition_on();
    else
      peripherals_ignition_off();

    while (1) {
      monitor_task_set(TASK_MONITOR_BIT_MONITOR);

      /*
      // ?????????
      count_x++;
      NRF_LOG_INFO("monitor message %d", count_x);
      vTaskDelay(1);
      continue;
      */
      keypad_state_machine();

      count++;
    }
}
//bool ignition_flag_pri =0;
void keypad_state_machine(void)
{
  static uint32_t last_pressed_time = 0;
  static uint32_t last_security_state_change_time = 0;
  static uint32_t last_locked_time = 0;
  static uint32_t last_password_enter_time = 0;
  static uint32_t disarmed_state_timeout = 0;
  static uint32_t garage_state_timeout = 0;
  static uint32_t sending_state_timeout = 0;  
  static uint32_t keep_alive_timeout = 0;  
  static uint32_t keep_alive_count = 0;
  static uint32_t connection_test_time = 0;  
 
  static char password_received[8];
  static char previous_password_received[8];
  static char candidate_password_received[8];
  static uint8_t wrong_password_count = 0;
  static uint8_t new_password_count = 0;
  uint32_t duration;
  KeypadMessage msg_key;
  SecurityState new_security_state;
  bool ret;
  bool ignition_flag;
  uint8_t i;

  #ifdef RUPTELA_SERVICE_SIMULATION

  new_security_state = read_new_security_state();
  if (new_security_state != SECURITY_STATE_NONE)
  {
    change_security_state(new_security_state);

    switch (security_state)
    {
      case SECURITY_STATE_DISARMED:
        disarmed_state_timeout = xTaskGetTickCount();
        disarmed_state_timeout_value = ruptela_command.value1;
        change_key_state(KEYSTATE_MAIN_MENU);
        break;

      case SECURITY_STATE_ARMED:
        back_to_armed_state(true);
        break;
    }
  }

  #endif

  ret = keypad_receive_key(&msg_key);
  ignition_flag = peripheral_is_switch_ignition_on();
   //if(ignition_flag && ignition_flag_pri ==0)NRF_LOG_INFO("ignition_on - keypad_state_machine");  
  // ignition_flag_pri = ignition_flag;

  // handle keypad lighting
  if (ret)
  {    
    //NRF_LOG_INFO("received key");

    if (last_pressed_time == 0)
      peripheral_turn_on_all_led(true);

    last_pressed_time = xTaskGetTickCount();    
  }

  // refresh the ignition state every second
  duration = timeDiff(xTaskGetTickCount(), sending_state_timeout);
  if (duration > 1000)
  {
    sending_state_timeout = xTaskGetTickCount();
    set_leach_state();
  }

  #ifdef RUPTELA_SERVICE_SIMULATION

  duration = timeDiff(xTaskGetTickCount(), keep_alive_timeout) / 1000;
  if (duration >=  60 || (keep_alive_count<=10 && duration>3) )
  {
    keep_alive_timeout = xTaskGetTickCount();
    keep_alive_count++;
    // ruptela_send_log(RUPTELA_LOG_KEEP_ALIVE, (uint8_t*)&keep_alive_count, NULL);     
    ruptela_send_log(RUPTELA_LOG_COUNTTERS, (uint8_t*)&eprom_configuration.reset_count, NULL, last_reset_code);                        
  }

  #endif

  //if (!ignition_flag && msg_key.is_long_key && msg_key.key == 6)
  if ( msg_key.is_long_key && msg_key.key == 6) //Ran 30/06/2025 enable arm state also after Disarm with IGN ON
  {
    if (key_state != KEYSTATE_LOCKED && security_state != SECURITY_STATE_ARMED)
      //NRF_LOG_INFO("back_to_armed_state");  
      back_to_armed_state(true);
  }

  duration = timeDiff(xTaskGetTickCount(), last_password_enter_time) / 1000;
  if ( duration > LOCKED_STATE_TIME_SEC)
  {
    last_password_enter_time = xTaskGetTickCount();
     wrong_password_count = 0;    
  }
    

  //////////////////////////
  // manage keypad states //
  //////////////////////////
  
  switch (key_state)
  {
    case KEYSTATE_IDLE:
      
      if (ret /*&& check_switch_on_entering_password() */)   
      //if (msg_key.key_release)       
      {
        NRF_LOG_INFO("ret: %d" , ret);
        if (!msg_key.is_long_key && msg_key.key == 6)
       // if (msg_key.time_ms >100 && msg_key.key == 6)
        {
          NRF_LOG_INFO("***************msg_key.key: %d" , msg_key.key);
          if (msg_key.key_release)
         // if (ret)
            buzzer_long(100, true, true);
            change_key_state(KEYSTATE_ASTRIX_RECEIVED);
         }
          else
            buzzer_long(100, true, true);
            //msg_key.key_release=0;
       //else msg_key.key_release=0;
      }
      break;

    case KEYSTATE_ASTRIX_RECEIVED:
      if (msg_key.key_release /*&& check_switch_on_entering_password()*/)
      {
         NRF_LOG_INFO("msg_key.key_release: %d" , msg_key.key_release);
         buzzer_long(50, true, true);
         msg_key.key_release=0;
        if (!msg_key.is_long_key)
        {       
          if (msg_key.key == 6)
          {          
            nrf_queue_reset(&my_queue);
          }
          else
          {
            nrf_queue_push(&my_queue, &msg_key);
            NRF_LOG_INFO("key pushed: %d" , msg_key.key);

            if (nrf_queue_is_full(&my_queue) )
            {
              i = 0;
              memset( password_received, 0x00, 8);
              while ( nrf_queue_pop(&my_queue,&msg_key ) == NRF_SUCCESS )
              {
                password_received[i] = 0x30 + msg_key.key;
                i++;
              }

              NRF_LOG_INFO("Entered: %s" , password_received);

              nrf_queue_reset(&my_queue);

              // check password
              if ( security_state == SECURITY_STATE_NEW_PASSWORD)
              {
                new_password_count++;
                change_key_state(KEYSTATE_IDLE);
                garage_state_timeout = xTaskGetTickCount();

                if (new_password_count == 1)
                {                  
                  disarmed_state_timeout = xTaskGetTickCount();
                  garage_state_timeout = xTaskGetTickCount();
                  strcpy( candidate_password_received, password_received);
                  success_buzzer();
                }
                else if (new_password_count == 2)
                {
                  disarmed_state_timeout = xTaskGetTickCount();
                  garage_state_timeout = xTaskGetTickCount();

                  if ( strcmp( password_received , candidate_password_received ) == 0 )
                  {
                    strcpy( eprom_configuration.password, candidate_password_received );                   
                    configuration_save_with_crc();

                    NRF_LOG_INFO("password change succeed");
                    success_buzzer();
                    //send_log("Pass Replaced");
                    ruptela_send_log(RUPTELA_LOG_PASSWORD_CHANGED, password_received, NULL, 0);                    
                  }
                  else
                  {
                    NRF_LOG_INFO("password change failed");
                    failure_buzzer();
                  }
                }
              }
              else if ( strcmp( password_received , eprom_configuration.password ) == 0 )
              {
                NRF_LOG_INFO("Password OK");
                change_key_state(KEYSTATE_MAIN_MENU);
                change_security_state(SECURITY_STATE_DISARMED);
                wrong_password_count = 0;
                disarmed_state_timeout = xTaskGetTickCount();
                last_security_state_change_time = xTaskGetTickCount();
                last_password_enter_time = xTaskGetTickCount();
                success_buzzer();
                uint32_t value2 = 0;
                ruptela_send_log(RUPTELA_LOG_DISARMED, eprom_configuration.password, (uint8_t*)&value2, 0);                    

                // alon: 28-02-2034, patch - perform reset if number of connected device is not
                // matching the peripheral list in eprom in order to improve ble connectivity
                if (!check_all_peripheral_connected() )
                  reset_device(RESET_CONNECTION_COUNT_FAIL_AFTER_PASSWORD);
              }
              else
              {
                NRF_LOG_INFO("Wrong Password");
                change_key_state(KEYSTATE_IDLE);
                last_password_enter_time = xTaskGetTickCount();
                failure_buzzer();

                if (wrong_password_count == 0)
                {
                  wrong_password_count++;
                }
                else
                {
                  if ( strcmp( password_received , previous_password_received ) != 0 )
                    wrong_password_count++;
                }

                strcpy( previous_password_received, NULL);

                // send_log("Password Wrong");
                ruptela_send_log(RUPTELA_LOG_WRONG_CODE, password_received, NULL, 0);                    

                if (wrong_password_count >= WRONG_PASSWORD_TRIES)
                {
                  change_key_state(KEYSTATE_LOCKED);
                  is_blocking_keypad_found = true;

                  eprom_configuration.is_locked = 1;
                  configuration_save_with_crc();

                  last_locked_time = xTaskGetTickCount();
                  peripheral_turn_off_all_led(true);

                  //send_log("Lock keypad");
                  ruptela_send_log(RUPTELA_LOG_KEYPAD_LOCKED, NULL, NULL, 0);                    
                }
                else
                {
                  
                }
              }
            }
          }
        }
        else
        {
          buzzer_long(50, true, true);
        }
      }
      break;

      case KEYSTATE_MAIN_MENU:

        if (ret && check_switch_on_in_garage() && msg_key.is_long_key && msg_key.key == 5)
        {
          scan_stop(true);
          disconnect_and_delete_all_peripheral(false);

          garage_state_timeout = xTaskGetTickCount();
          disarmed_state_timeout = xTaskGetTickCount();
          change_key_state(KEYSTATE_GARAGE_MENU);
          change_security_state(SECURITY_STATE_GARAGE);
          buzzer_train(2, false, true);
          // send_log("Garage mode");          
        }
        else if (ret && check_switch_on_in_garage() && msg_key.is_long_key && msg_key.key == 3)
        {
          // new password
          disarmed_state_timeout = xTaskGetTickCount();
          garage_state_timeout = xTaskGetTickCount();
          change_key_state(KEYSTATE_IDLE);
          change_security_state(SECURITY_STATE_NEW_PASSWORD);
          new_password_count = 0;
          success_buzzer();
        }

        break;

      case KEYSTATE_GARAGE_MENU:

        if (ret && check_switch_on_in_garage() && msg_key.is_long_key)
        {
          disarmed_state_timeout = xTaskGetTickCount();

          switch (msg_key.key)
          {
            case 1:
              garage_state_timeout = xTaskGetTickCount();
              disarmed_state_timeout = xTaskGetTickCount();

              configuration_reset_eprom();
              configuration_set_default();
              configuration_save_with_crc();
              NRF_LOG_INFO("Back to default settings");

              ruptela_send_log(RUPTELA_LOG_BONDING_DELETE, NULL, NULL, 0);                    
              vTaskDelay(1000); // delay - give chance to peripherals to receive this log

              disconnect_and_delete_all_peripheral(true);
              NRF_LOG_INFO("Bondings deleted");
                          
              success_buzzer();
              // send_log("Delete bonding");            

              // alon: 28-02-2034, patch - perform reset after cleaning eprom
              // in order to improve ble connectivity in the bonding process
              eprom_configuration.garage_mode = 1;
              change_security_state(SECURITY_STATE_DISARMED);
              vTaskDelay(1000); 
              reset_device(RESET_CLEAR_BONDING);
              break;

            case 4:
              garage_state_timeout = xTaskGetTickCount();
              disarmed_state_timeout = xTaskGetTickCount();

              peripheral_turn_off_all_led(false);
              NRF_LOG_INFO("Start bonding..."); 
              buzzer_train(2, false, true);               
              vTaskDelay(500);                
              scan_init_by_uuid(false);
              scan_start(true);
              activate_bonding_state(true);
              // send_log("Start bonding");
              afterStudy = true;
              break;

            case 5:
              // stay in garage mode
              garage_state_timeout = xTaskGetTickCount();
              disarmed_state_timeout = xTaskGetTickCount();
              buzzer_train(2, false, true); 
              // send_log("Garage mode");
              break;

            case 6:
              back_to_armed_state(true);
              break;

           default:
            NRF_LOG_INFO("unknown option");            
            break;
          }
        }
        break;
  }

  if (ret && msg_key.is_long_key && msg_key.key == 2)
  {
    success_buzzer();
    uint8_t i = 0;
    while (i < 15)
    {
      blink_bonded_device_count(false, true);
      vTaskDelay(200);
      i++;
    }          
  }

  // handle lighting kypad after keupad is clicked
  if ((!in_bonding_state)||(allLedsAreOn == true))  //Ran add allLedsAreOn  09/10/25
  {
    duration = timeDiff(xTaskGetTickCount(), last_pressed_time) / 1000;
    if (duration > KEYPADLIGHT_AFTER_KEY_PRESS_SEC)
    {
      peripheral_turn_off_all_led(false);
      last_pressed_time = 0;
    }
  }

  /////////////////////////////
  // manage secuirity states //
  /////////////////////////////


  switch(security_state)
  {
    
    case SECURITY_STATE_DISARMED:
      
      if (ignition_flag)
        disarmed_state_timeout = xTaskGetTickCount();
      else
      {
        duration = timeDiff(xTaskGetTickCount(), disarmed_state_timeout) / 1000;
        if (duration > disarmed_state_timeout_value)
          back_to_armed_state(true);
      }
     
      break;

    case SECURITY_STATE_ARMED:
      if (key_state != KEYSTATE_LOCKED)
      {
        duration = timeDiff(xTaskGetTickCount(), connection_test_time) / 1000;
        if (duration > CONNECTION_TEST_TIME_IN_SEC )
        {
          connection_test_time = xTaskGetTickCount();
          // alon: 28-02-2034, patch - perform reset if number of connected device is not
          // matching the peripheral list in eprom in order to improve ble connectivity
          if (!check_all_peripheral_connected() )
            reset_device(RESET_CONNECTION_COUNT_FAIL_IN_ARM_MODE);
        }
      if(connections_array[2].peripheral_type == PERIPHERAL_TYPE_LEACH) //Ran add this 09/10/25
      {
       NRF_LOG_INFO("Found application leach reseting the data");
       connections_array[2].ignition_state =0;
       connections_array[2].connection_time =0;
       connections_array[2].is_connected = 0;
       connections_array[2].conn_handler = 0;
       connections_array[2].peripheral_type= 0;
       connections_array[2].peer_address.addr_type=0;
       for(int i =0;i<=5;i++)
          connections_array[2].peer_address.addr[i]=0;
       connected_count--;
       needToDoReset = true;
      //Ran add this compare of serial number 
       NRF_LOG_INFO("Compare the serial numbers");
       unsigned char leach1[14]={NULL};
       memcpy(leach1,device_Serial_write.deviceSerial[0],15); 
       unsigned char App[16]={NULL};
       memcpy(App,device_Serial_write.deviceSerial[1],15); 
       NRF_LOG_INFO("serial leach:     %s", leach1);
       NRF_LOG_INFO("serial leach app: %s", App);
       needToDoReset = true;
       if(memcmp(device_Serial_write.deviceSerial[0],device_Serial_write.deviceSerial[1],15) == 0){
            NRF_LOG_INFO("Serials are the same");
            change_security_state(SECURITY_STATE_DISARMED);
            vTaskDelay(500);
            NRF_LOG_INFO("________________Change security_state to disarmed by the App______________");
         }     
       }else if(needToDoReset == true){
          vTaskDelay(500);
          needToDoReset=false;
          reset_device(0);
          }   
      }

      if (key_state == KEYSTATE_LOCKED)
      {
        // beep when driver trying to turn on the car during block
        // buzzer_train(3, true, true);
        lock_buzzer();

        if (is_blocking_keypad_found)
          vTaskDelay(1000);
        else
          vTaskDelay(5000);
        is_blocking_keypad_found = false;

        duration = timeDiff(xTaskGetTickCount(), last_locked_time) / 1000;
        if (duration > LOCKED_STATE_TIME_SEC )
        {
          // eprom_configuration.is_locked = 0;
          // configuration_save_with_crc();
          wrong_password_count = 0;
          back_to_armed_state(true);
        }        
      }
      else if (ignition_flag)
      {
        vTaskDelay(10);        //Ran change the timer to improve keyboard response 
        oneScondCounter++;
        if(oneScondCounter > 100){
        oneScondCounter =0;
        // one beep when driver trying to transfer money in armed state
        duration = timeDiff(xTaskGetTickCount(), armed_state_enter_time) / 1000;

        if ((leach_bonded_count==0)&&(modem_bonded_count)) //electrical car Ran add this line 09/06/2025
        {
           NRF_LOG_INFO("fond ignition_flag %d \n", duration);  //Ran add this line 02/07/2025
          if (duration <= 5)
            buzzer_train(1, true, true);
        }
        else if (leach_bonded_count) //benzine car
        {
          NRF_LOG_INFO("fond ignition_flag %d \n", duration);  //Ran add this line 03/06/2025
          //(driveing_without_password_alert == true) //Ran add this line  04/06/2025  //Ran comment it out on 07/07/25
            buzzer_train(1, true, true);  
        }
        else if((leach_bonded_count==0)&&(modem_bonded_count==0)) //no bonded at all Ran add this line 09/06/2025
        {
           NRF_LOG_INFO("fond ignition_flag without bonded devices %d \n", duration);  //Ran add this line 03/06/2025
           buzzer_train(3, false, true);  
        }

        if ((duration > (3*60)) && !driveing_without_password_alert ) //Ran changed from 5 to 3*60 03/06/2025
        {
          driveing_without_password_alert = true;
          buzzer_train(1, true, true);  //Ran Add this line 03/06/2025
          NRF_LOG_INFO("fond ignition_flag for more than %d second \n", duration);  //Ran add this line 03/06/2025
          ruptela_send_log(RUPTELA_LOG_IGNITION_ON_IN_ARM, (uint8_t*)&time_since_2020, NULL, 0);          
        }
        if (duration > (3*60)) //Ran add it 06/07/2025
        {
          buzzer_train(1, true, true);  //Ran Add this line 03/06/2025
        }
       }
      }
      else
      {
        driveing_without_password_alert = 0;
        armed_state_enter_time = xTaskGetTickCount();
        driveing_without_password_alert = false;
      }

      break;

      case SECURITY_STATE_GARAGE:

        duration = timeDiff(xTaskGetTickCount(), garage_state_timeout) / 1000;
        if (duration > DISARM_GARAGE_TIMEOUT_SEC)
          back_to_armed_state(true);

       #ifdef USE_SWITCH_IN_GARAGE_MODE
          if (!ignition_flag)
            back_to_armed_state(true);
       #endif

       break;

      case SECURITY_STATE_NEW_PASSWORD:

        duration = timeDiff(xTaskGetTickCount(), garage_state_timeout) / 1000;
        if (duration > DISARM_NEW_PASSWORD_TIMEOUT_SEC)
          back_to_armed_state(true);

        #ifdef USE_SWITCH_IN_GARAGE_MODE
          if (!ignition_flag)
            back_to_armed_state(true);
        #endif

        break;
  }

  NRF_LOG_FLUSH();
}


static void change_key_state( KeyState new_state)
{
    key_state = new_state;

    static const char *Names[] = {
        "IDLE",
        "ASTRIX",
        "MAIN_MENU",
        "GARAGE_MENU",
        "LOCKED"
    };

    if (key_state >= 0 && key_state < KEYSTATE_LAST) {
        NRF_LOG_INFO("Key state: %s\n", Names[key_state]);
    }
}

void set_leach_state(void)
{
    ble_peripheral_info_t* peripheral_info;

    static uint32_t miss_message_count = 0;
    static uint32_t count;
    static int8_t prev_ignition_value = -1;

    uint32_t value = 0;
    uint8_t i = 0;
    bool ignition_changed = false;

    switch (security_state)
    {
      case SECURITY_STATE_ARMED:
        value &= ~(1 << IGNITION_STATE_SWITCH_BIT);
        break;

      case SECURITY_STATE_DISARMED:
      case SECURITY_STATE_GARAGE:
      case SECURITY_STATE_NEW_PASSWORD:
        value |= (1 << IGNITION_STATE_SWITCH_BIT);
        break;
    }

    if (value != prev_ignition_value)
    {
      vTaskDelay(100); // patch:delay on sending switch changed log
      ruptela_send_log(RUPTELA_LOG_SWITCH_CHANGED, (uint8_t*)&value, NULL, 0);
      vTaskDelay(100); // patch:delay on sending switch changed log
      prev_ignition_value = value;
      ignition_changed = true;
    }

    value |= (1 << IGNITION_STATE_SWITCH_ENABLE_BIT);

    count++;
    if (count%2)
      value |= (1 << IGNITION_STATE_RELAY_MODE_BIT);
    else
      value &= ~(1 << IGNITION_STATE_RELAY_MODE_BIT);

    if (connections_state_lock(1) )        
    {
      connected_count = 0;
      for (i=0 ; i<bonded_count ; i++)
      {
        peripheral_info = &connections_array[i];
        if (peripheral_info->is_connected)
        {
          connected_count++;
          //Ran Add this print 31/07/25 
          if((peripheral_info->peripheral_type == PERIPHERAL_TYPE_LEACH)||(peripheral_info->peripheral_type == PERIPHERAL_TYPE_CANBUS_LEACH)){
           leachConnectedFlag[i] = true;
          }
           NRF_LOG_INFO("connected_count: %d, Connecten type: %d ", connected_count, peripheral_info->peripheral_type); //Ran add print 28/07/25
          if (peripheral_info->peripheral_type == PERIPHERAL_TYPE_MODEM)
          { 
            modemConnectedFlag = true;
            if (ignition_changed)
              send_ignition_state_to_peripheral(value, peripheral_info->conn_handler);
          }
          else
          {
            send_ignition_state_to_peripheral(value, peripheral_info->conn_handler);
          }
        }
        else  //peripheral became disconnect
        {
          miss_message_count++;
          //Ran add this check in case the peripheral leach wase disconnected send notification to host by the modem. 
          if((peripheral_info->peripheral_type == PERIPHERAL_TYPE_LEACH)||(peripheral_info->peripheral_type == PERIPHERAL_TYPE_CANBUS_LEACH)){
           if(leachConnectedFlag[i] == true){
            ruptela_send_log( RUPTELA_LOG_LEACH_DISCONNECTED, NULL, NULL, 0);
            leachConnectedFlag[i] = false;
            vTaskDelay(6000); //Ran add it to wait for the Ruptela to send the message then do reset
            reset_device(RESET_CONNECTION_LEACH_DISCONNECTED);
           }
          }
          //Ran add this check in case the peripheral modemh wase disconnected  12.09.25
          if(peripheral_info->peripheral_type == PERIPHERAL_TYPE_MODEM){
           if( modemConnectedFlag == true){
            modemConnectedFlag = false;
            vTaskDelay(6000); //Ran add it to wait for the Ruptela to send the message then do reset
            reset_device(RESET_CONNECTION_MODEM_DISCONNECTED);
           }
          }
        }
      }

      connections_state_unlock();
     if(security_state == SECURITY_STATE_ARMED){ //Ran add this to enable connection to the app.    
     if(startScanOnOperation ==7){ 
      scan_init_by_uuid(false);
      scan_start(true);
      activate_bonding_state(false);
      startScanOnOperation =0;
      stopScanDone = 0;
      enableToConnectApp = 1;
      NRF_LOG_INFO("######### Enable to operate Smartphone App #############");
      }else if(startScanOnOperation <= 7)startScanOnOperation++;
     }else if(security_state == SECURITY_STATE_DISARMED){
            if(stopScanDone == 0){
               scan_stop(true);
               stopScanDone = 1;
               enableToConnectApp = 0;
               de_activate_bonding_state();
               }
          }
    }
}                                          

void send_log(uint8_t* log_message)
{
    ble_peripheral_info_t* peripheral_info;

    uint8_t i = 0;
    //NRF_LOG_INFO( "connections_state_lock: %d", connections_state_lock(1)); //Ran 02/09/25
    //if (connections_state_lock(1))        
    {
      //for (i=0 ; i<bonded_count ; i++)
      for (i=0 ; i<4 ; i++)  //Ran change it to be 4 for testing 02/09/25
      {
        peripheral_info = &connections_array[i];
        if (peripheral_info->is_connected)
        {
          send_log_to_peripheral(log_message, peripheral_info->conn_handler);
          NRF_LOG_INFO( "send log 0x%X", i);
        }
      }

      connections_state_unlock();
    }
}                                          


void change_security_state(SecurityState new_state)
{
    bool need_saving = false;

    security_state = new_state;

    set_leach_state();
    
    switch (security_state)
    {
      case SECURITY_STATE_ARMED:
      case SECURITY_STATE_DISARMED:
        eprom_configuration.security_state = security_state;
        need_saving = true;
        break;
    }

    if (need_saving)
      configuration_save_with_crc();

    static const char *Names[] = {
        "NONE",
        "ARMED",
        "DISARMED",
        "GARAGE",
        "NEW_PASSWORD",
    };

    if (security_state >= 0 && security_state < SECURITY_STATE_LAST) {
        NRF_LOG_INFO("Security state changed to: %s\n", Names[security_state]);
    }
}


void back_to_armed_state(bool with_buzzer)
{
  eprom_configuration.is_locked = 0;
  configuration_save_with_crc();

  if (with_buzzer)
  {
    buzzer_long(1000,false, true);
  }

  disarmed_state_timeout_value = DISARM_WITHOUT_DRIVING_SEC;

  save_bondings();  

  change_key_state(KEYSTATE_IDLE);
  change_security_state(SECURITY_STATE_ARMED);
  nrf_queue_reset(&my_queue);
  
  if (need_refresh_scan_filter)
  {    
    disconnect_and_delete_all_peripheral(false);
    vTaskDelay(100);
    configuration_load_address_list();
    // scan_init_by_white_list();
    scan_init_by_uuid(true);
    scan_start(true);
    need_refresh_scan_filter = false;
  }

  // send_log("Armed");
  ruptela_send_log(RUPTELA_LOG_ARMED, NULL, NULL, 0);                    
}


void SetTimeFromString(uint8_t *date_str, uint8_t *time_str)
{
    Calendar *calendar = &absolute_time;

    calendar->year  = DD(date_str + 4);
    calendar->month = DD(date_str + 2);
    calendar->day   = DD(date_str);

    calendar->hour    = DD(time_str);
    calendar->minute  = DD(time_str + 2);
    calendar->seconds = DD(time_str + 4);
}

void GetSystemTime(Calendar *c)
{
    memcpy((void *)c, (uint8_t *)&absolute_time, sizeof(Calendar));
}

void InitClock(void)
{
    ret_code_t ret;

    ret = app_timer_create(&m_clock_timer, APP_TIMER_MODE_REPEATED, clock_tick_handler);
    APP_ERROR_CHECK(ret);
    app_timer_start(m_clock_timer, APP_TIMER_TICKS(1000), NULL);
}

uint32_t GetTimeStampFromDate(void)
{
    Calendar *c = &absolute_time;

    uint32_t timestamp;
    uint32_t days;
    uint8_t  i;

    days = c->year * 365;
    days += (c->year - 1) / 4 + 1;

    for (i = 1; i < c->month; i++) {
        days += GetDaysInMonth(c->year, i);
    }

    days += c->day - 1;

    timestamp = days * 24 * 3600;
    timestamp += c->hour * 3600;
    timestamp += c->minute * 60;
    timestamp += c->seconds;

    return timestamp;
}

void AddDay(Calendar *c)
{

    uint8_t days_in_month = GetDaysInMonth(c->year, c->month);

    if (c->day < days_in_month) {
        c->day++;
    } else {
        c->day = 1;
        if (c->month < 12) {
            c->month++;
        } else {
            c->month = 1;
            c->year++;
        }
    }
}

void AddHour(Calendar *c)
{
    if (c->hour + 1 < 24) {
        c->hour++;
    } else {
        c->hour = 0;
        AddDay(c);
    }
}

void AddMinute(Calendar *c)
{
    if (c->minute + 1 < 60) {
        c->minute++;
    } else {
        c->minute = 0;
        AddHour(c);
    }
}

void AddSeconds(Calendar *c)
{
    if (c->seconds + 1 < 60) {
        c->seconds++;
    } else {
        c->seconds = 0;
        AddMinute(c);
    }
}

void AddSecondsToDate(Calendar *c, uint32_t seconds)
{
    uint8_t day = c->day;

    uint8_t days_in_month;

    while (seconds >= (24 * 3600)) {
        AddDay(c);
        seconds -= (24 * 3600);
    }

    while (seconds >= (3600)) {
        AddHour(c);
        seconds -= (3600);
    }

    while (seconds >= (60)) {
        AddMinute(c);
        seconds -= (60);
    }

    while (seconds > (0)) {
        AddSeconds(c);
        seconds--;
    }
}

uint8_t GetDaysInMonth(uint8_t year, uint8_t month)
{
    uint8_t days = 30;

    switch (month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        days = 31;
        break;

    case 2:
        if (year % 4) {
            days = 28;
        } else {
            days = 29;
        }
        break;
    }

    return days;
}


void failure_buzzer(void)
{
  play_melody(melody_error,2500, true);

}

void success_buzzer(void)
{
  play_melody(melody_success,1200, true);
  //play_melody(melody_success,700, true);
}

void lock_buzzer(void)
{
  play_melody(melody_lock,1200, true);
  play_melody(melody_lock,700, true);
}


static bool check_switch_on_in_garage(void)
{
  #ifdef USE_SWITCH_IN_GARAGE_MODE

    bool ignition_flag = peripheral_is_switch_ignition_on();
    // if(ignition_flag && ignition_flag_pri ==0) NRF_LOG_INFO("ignition_on - check_switch_on_in_garage"); 
    // ignition_flag_pri=ignition_flag; 
    return ignition_flag;

  #else
    return true;

  #endif
}

static bool check_switch_on_entering_password(void)
{
  bool res = false;

  bool ignition_flag = peripheral_is_switch_ignition_on();
  // if(ignition_flag && ignition_flag_pri ==0) NRF_LOG_INFO("ignition_on - check_switch_on_entering_password");  
 // ignition_flag_pri=ignition_flag; 
  switch (security_state)
  {
    case SECURITY_STATE_DISARMED:
      if (ignition_flag)
        res = true;
      break;

    case SECURITY_STATE_NEW_PASSWORD:

      #ifdef USE_SWITCH_IN_GARAGE_MODE
        if (ignition_flag)
          res = true;
      #else
        res = true;
      #endif
      break;

    case SECURITY_STATE_ARMED:

      if (!ignition_flag)
        res = true;
      break;
  }

  return res;
}


////////////////////////////
// task monitor functions //
////////////////////////////

void monitor_task_set(uint8_t task_id)
{
    xSemaphoreTake(watchdog_monitor_semaphore, portMAX_DELAY);

     // monitor_state.task_bits |= (1 << task_bit);
     task_keep_alive_array[task_id] = 1;

    xSemaphoreGive(watchdog_monitor_semaphore);
}

void monitor_task_check(void)
{
    uint32_t duration;
    uint8_t  i;
    bool     status = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xHigherPriorityTaskWoken1 = pdFALSE;

#ifdef DISABLE_WATCHDOG
    return true;
#endif

    if ( xSemaphoreTakeFromISR(watchdog_monitor_semaphore, &xHigherPriorityTaskWoken) )
    {
      i = 0;
      while (i < TASK_MONITOR_NUM) 
      {
        if (task_keep_alive_array[i] == 0)
        {
          status = false;
          break;
        }

        i++;        
      }

      xSemaphoreGiveFromISR( watchdog_monitor_semaphore, &xHigherPriorityTaskWoken1 );
    }

    if (!status) 
    {
      // force reset on stucked task
      NRF_LOG_INFO("task %d stuck", (i));
      NRF_LOG_FLUSH();
      #ifndef DEBUG_MODE
        reset_device(RESET_WATCHDOG);
      #endif
    }

    if ( xHigherPriorityTaskWoken )
    {
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else if ( xHigherPriorityTaskWoken1 )
    {
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken1);
    }

    state_machine_feed_watchdog();
}

bool check_all_peripheral_connected(void)
{
  bool res = true;

/*  if (bonded_count > 0)
  {
    if (bonded_count != connected_count)
      res = false;
  }
 */
 //Ran change to this 02/08/25
  if((bonded_count > 0)&& (connected_count==0)) res = false;

  return res; 
}

bool checkLeachConnectionEvent(void){

}


static char const * const resetCodes[] =
{
    "Reset no code",
    "Reset by wachdog",
    "Reset by by clear bonding",
    "Reset by by bonding complete",
    "Reset by connection count fail after paswword",
    "Reset by connection count fail in arm mode",
    "Reset by leach disconnect"  //Ran add it on 21/08/2025
    "Reset by modem disconnect"  //Ran add it on 12/09/2025
};

void reset_device(uint8_t reset_code)
{
  eprom_configuration.last_reset_code = reset_code;
  configuration_save_with_crc();
  NRF_LOG_INFO("%s",resetCodes[reset_code]);
  vTaskDelay(10);
  NVIC_SystemReset();                  
}

