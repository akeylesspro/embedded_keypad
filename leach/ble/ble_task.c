#include "ble_task.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "ble/ble_services_manager.h"
#include "ble_leach.h"
#include "logic/ble_file_transfer.h"
#include "logic/commands.h"
#include "logic/ignition.h"
#include "logic/monitor.h"
#include "logic/serial_comm.h"
#include "nrf_log_ctrl.h"
#include "nrf_ringbuf.h"
#include "nrfx_atomic.h"
#include "queue.h"
#include "drivers/eprom.h"
#include "logic/configuration.h"
#include "logic/peripherals.h"
#include "drivers/buzzer.h"
#include "hal/hal_boards.h"
#include "Ruptela.h"

#define NRF_LOG_MODULE_NAME ble_task
#define NRF_LOG_LEVEL BLE_LEACH_BLE_LOG_LEVEL
#include "nrfx_log.h"
NRF_LOG_MODULE_REGISTER();

//extern EpromConfiguration eprom_configuration;

extern EventGroupHandle_t event_ignition_characteristic_changed;
extern EventGroupHandle_t event_ble_connection_changed;

extern bool configuration_loaded;
extern bool need_refresh_scan_filter;
extern bool afterStudy;


static uint32_t old_ignition_value;
static uint32_t new_ignition_value;

static ble_peripheral_info_t leach_peripheral_info;
static ble_peripheral_info_t canbus_leach_peripheral_info;
static ble_peripheral_info_t modem_peripheral_info;


//////////////////
// global state //
//////////////////

ble_peripheral_info_t current_conn_info;
xSemaphoreHandle connection_list_semaphore;
QueueHandle_t queue_connections;
ble_peripheral_info_t connections_array[MAX_CONNECTIONS];
uint8_t bonded_count = 0;
uint8_t leach_bonded_count = 0;
uint8_t modem_bonded_count = 0;
bool in_bonding_state = false;
uint32_t bonding_process_start_time = 0;
uint32_t connected_count = 0;
uint8_t leachfound =0; //Ran 29/07/25

bool ledsforBonding = false;

#ifdef RUPTELA_SERVICE_SIMULATION
  extern QueueHandle_t command_message_queue;
  Ruptela_Command ruptela_command;
#endif

//////////////////

static int8_t find_peripheral_recognized(ble_peripheral_info_t* peripheral_info);
static void verify_peripheral(ble_peripheral_info_t* peripheral_info);
static void remove_peripheral(ble_peripheral_info_t* peripheral_info);
static int8_t add_new_device_to_list(ble_peripheral_info_t* periphral_info);

void init_ble_task(void)
{
   old_ignition_value = 0xFFFFFFFF;
   current_conn_info.conn_handler = -1;

   // creating connection queue
    queue_connections = xQueueCreate(MAX_CONNECTIONS, sizeof(ble_peripheral_info_t));

    if (queue_connections == NULL)
    {
        // Failed to create the queue
        // Handle error
    }

    #ifdef RUPTELA_SERVICE_SIMULATION
      command_message_queue = xQueueCreate(3, 16);
    #endif
}

void ble_thread(void *pvParameters)
{
    static ble_peripheral_info_t peripheral_info;
    static uint8_t command_data[16];

    EventBits_t uxBits;
    ret_code_t err_code;
    bool flag;

    ble_services_init_0();

    while (!configuration_loaded)
    {
      vTaskDelay(100);
    }
#ifdef DEBUG
ble_services_init_central();
#else
 ble_services_init_central();

#endif
   
    while (1) 
    {
      monitor_task_set(TASK_MONITOR_BIT_BLE);
      NRF_LOG_FLUSH();

      uxBits = xEventGroupWaitBits(
               event_ble_connection_changed,   /* The event group being tested. */
               0xFFFF, /* The bits within the event group to wait for. */
               pdFALSE,        /* should be cleared before returning. */
               pdFALSE,       /* Don't wait for both bits, either bit will do. */
               1 );

      if (uxBits & 0x0001)
      {
        if (current_conn_info.peripheral_type != PERIPHERAL_TYPE_NEW )
        {
          // delay to accept the rest of DIS service characteristics        
          vTaskDelay(250);

          peripheral_info = current_conn_info;
          verify_peripheral(&peripheral_info);
         
          xEventGroupClearBits(event_ble_connection_changed, 0xFFFF);
          scan_start(false);
        }
      }
      else if (uxBits & 0x0002)
      {
        vTaskDelay(250);

        peripheral_info = current_conn_info;
        remove_peripheral(&peripheral_info);

        xEventGroupClearBits(event_ble_connection_changed, 0xFFFF);
        scan_start(false);
      }

      connections_state_lock(0);
      //flag = in_bonding_state;
      flag = ledsforBonding;
      connections_state_unlock();
      
      if (flag)
      {
        vTaskDelay(100);
        connections_state_lock(0);
        blink_bonded_device_count(true, true);
        connections_state_unlock();
      }

      // receive command
      #ifdef RUPTELA_SERVICE_SIMULATION
        if (xQueueReceive(command_message_queue, command_data, 1)) 
        {
            NRF_LOG_INFO("\r\n\r\nReceived command:");
            ruptela_decode_command(command_data, &ruptela_command);
        }
      #endif

      // leach ignition state
      uxBits = xEventGroupWaitBits(
               event_ignition_characteristic_changed,   /* The event group being tested. */
               0xFFFF, /* The bits within the event group to wait for. */
               pdTRUE,        /* should be cleared before returning. */
               pdFALSE,       /* Don't wait for both bits, either bit will do. */
               1 );

      if (uxBits)
      {
        NRF_LOG_INFO( "Leach keep alive" );
      }
    }
}


void save_bondings(void)
{
  bool flag;

  connections_state_lock(0);
  flag = in_bonding_state;
  connections_state_unlock();
   NRF_LOG_INFO("in_bonding_state %d", flag);
  // saving bonding list
  if (flag)
  {       
    scan_stop(true);

    NRF_LOG_INFO("bonded %d", bonded_count);
    NRF_LOG_INFO("configuratio_print");
    configuration_print();
     NRF_LOG_INFO("configuration_save_address_list");
    configuration_save_address_list();
    NRF_LOG_INFO("set_leach_state");
    set_leach_state();

    connections_state_lock(1);
    in_bonding_state = false;
    connections_state_unlock();

    ruptela_send_log(RUPTELA_LOG_BONDING_SAVED, (uint8_t*)&bonded_count, NULL, 0);                  
          
    disconnect_and_delete_all_peripheral(false); //Ran uncomment under test and checking 16/10/25 for now dont comment it

    // alon: 28-02-2034, patch - perform reset in order to improve ble connectivity
    // after bonding stage completed
    change_security_state(SECURITY_STATE_ARMED);
    vTaskDelay(3000);
    if(afterStudy == true){  //ran addthe condition  06/10/25
        reset_device(RESET_BONDING_COMPLETED);  
        }

    /*
    vTaskDelay(100);

    // scan_init_by_white_list();
    scan_init_by_uuid(true);
    scan_start(true);
    */
  }      
}


static void remove_peripheral(ble_peripheral_info_t* peripheral_info)
{
  uint8_t i;
  uint8_t conn_handler = peripheral_info->conn_handler;

  connections_state_lock(0);

  for (i=0; i<MAX_CONNECTIONS ; i++)
  {
    if ( connections_array[i].conn_handler == conn_handler && connections_array[i].is_connected)
    {
      print_address("disconnected device", &connections_array[i].peer_address);
      connections_array[i].is_connected = false;
      break;
    }
  }

  connections_state_unlock();
}

static void verify_peripheral(ble_peripheral_info_t* peripheral_info)
{
  static uint32_t session_count = 0;
  ret_code_t err_code;
  bool reject = false;
  int8_t new_device_index = -1;
                     
  connections_state_lock(0);

  session_count++;

  NRF_LOG_INFO("\r\n\r\nSession=%d" , session_count);
  print_address("checking device", &peripheral_info->peer_address);
  vTaskDelay(500); //Ran add delay 14.09.25
  int32_t conn_index = find_peripheral_recognized(peripheral_info);

  if ( conn_index >= 0)  
  {          
    NRF_LOG_INFO("Device in list: index=%d, session=%d", conn_index, session_count);

    peripheral_info->is_connected = true;
    connections_array[conn_index] = *peripheral_info;
    NRF_LOG_INFO("Device type: %d", connections_array[conn_index].peripheral_type); //Ran 14.09.25

    ruptela_send_log( RUPTELA_LOG_LEACH_CONNECTED, (uint8_t*)&conn_index, NULL, 0);
  }
  else  
  {
    if (in_bonding_state)
    {
      NRF_LOG_INFO("New peripheral_type %d", peripheral_info->peripheral_type);   
        if(leachfound == 0){  //Ran Add to get enable leach paring 28/07/25
        new_device_index = add_new_device_to_list(peripheral_info);
        if (new_device_index >= 0 && new_device_index < MAX_CONNECTIONS)
        {
          // add device in list list
          NRF_LOG_INFO("Accepted new device %d, type %d", new_device_index, peripheral_info->peripheral_type);
          peripheral_info->is_connected = true;
          connections_array[new_device_index] = *peripheral_info;
          bonded_count++;
          bonding_process_start_time = xTaskGetTickCount();  
          buzzer_train(1,true,true);
          vTaskDelay(100);
        }
       }
      else if((leachfound == 1)&&(getEnablePairing())){  //Ran Add to get enable leach paring 29/07/25
        peripheral_info->peripheral_type = PERIPHERAL_TYPE_LEACH;
        new_device_index = add_new_device_to_list(peripheral_info);
        if (new_device_index >= 0 && new_device_index < MAX_CONNECTIONS)
        {
          // add device in list list
          NRF_LOG_INFO("Accepted new device %d, type %d", new_device_index, peripheral_info->peripheral_type);
          peripheral_info->is_connected = true;
          //connections_array[3] = *peripheral_info;  //Ran just for testing Ran *****************************************************
          connections_array[new_device_index] = *peripheral_info;
          bonded_count++;
          bonding_process_start_time = xTaskGetTickCount();  
          buzzer_train(1,true,true);
          vTaskDelay(100);
        }
      }
      else
      {
        NRF_LOG_INFO("Device type is not allowed");
        if(getEnablePairing()) reject = true; //Ran add if((getEnablePairing())
      }
    }
    else
    {
      print_address("Device is not in list", &peripheral_info->peer_address);
      reject = true;
    }

    if (reject)
    {
      NRF_LOG_INFO("Device rejected");

      // ALON: 
      // without this delay, 
      // the following disconnection cause an exception
      // probably because the discovery
      vTaskDelay(100);

      err_code = sd_ble_gap_disconnect(peripheral_info->conn_handler, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
          
      if (err_code != NRF_SUCCESS) {
        // Handle error
        NRF_LOG_INFO("Failed to disconnect. Error code: %d", err_code);
      } else {
        NRF_LOG_INFO("Disconnect initiated successfully.");
      }      
    }
  }

  NRF_LOG_FLUSH();

  connections_state_unlock();

  set_leach_state();
}

static int8_t find_peripheral_recognized(ble_peripheral_info_t* search_peripheral_info /*ble_gap_addr_t* search_address, PeripheralType per_type*/)
{
  ble_peripheral_info_t* peripheral_info;
  PeripheralType t = PERIPHERAL_TYPE_UNKNOWN;
  uint8_t i = 0;
  int8_t found_idx = -1;

  for ( i=0 ; i<bonded_count ; i++)
  {
    peripheral_info = &connections_array[i];

    if ( (memcmp( peripheral_info->peer_address.addr, search_peripheral_info->peer_address.addr, 6 ) == 0)
           // && (peripheral_info->peripheral_type == search_peripheral_info->peripheral_type)) 
            && (true) )
    {
      NRF_LOG_INFO("find_peripheral_recognized %d, peripheral_type %d",i,search_peripheral_info->peripheral_type);
      found_idx = i;
      break;
    }
  }
  
  //search_peripheral_info->peripheral_type = connections_array[i].peripheral_type; //Ran Comment it out. 14.09.25

  return found_idx;
}


void print_address(char* text, ble_gap_addr_t *address) 
{
    static uint8_t buffer[48];

    for (uint8_t i = 0; i < 6; i++) {
      sprintf( buffer + 3*i, "%02X:", address->addr[i]);
    }

    buffer[3*6-1] = 0;

    NRF_LOG_INFO("%s: %s\r\n", text, buffer);
    NRF_LOG_FLUSH();
}


static int8_t add_new_device_to_list(ble_peripheral_info_t* periphral_info)
{
  uint8_t i = 0;
  int8_t new_handler = -1;
  uint8_t device_on_the_same_type_count = 0;
  uint8_t num_ = 1;
  uint8_t current_list_bonding_size = 0;

  for (i=0; i< bonded_count ; i++)
  {
    if (connections_array[i].conn_handler >=  0)
      current_list_bonding_size++;
  } 
  return current_list_bonding_size;
}

void activate_bonding_state(bool bonding_state) //Ran add bool bonding_state 04/10/25
{
  connections_state_lock(0);

  bonding_process_start_time = xTaskGetTickCount();  
  in_bonding_state = true; 
  ledsforBonding = bonding_state; 
  connections_state_unlock();
}

void de_activate_bonding_state(void) //Ran add this option 15/10/25
{
  connections_state_lock(0);
  in_bonding_state = false; 
  ledsforBonding = false; 
  connections_state_unlock();
}

void disconnect_and_delete_all_peripheral(bool including_address_list_in_eprom)
{
  uint8_t i;
  ret_code_t err_code;

  connections_state_lock(0);

  need_refresh_scan_filter = true;

  if (including_address_list_in_eprom)
    configuration_delete_all_addresses();

  for (i=0 ; i<bonded_count; i++)
  {
    err_code = sd_ble_gap_disconnect(connections_array[i].conn_handler, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    
    if (err_code != NRF_SUCCESS) 
    {
      // Handle error
      NRF_LOG_INFO("Failed to disconnect. Error code: %d", err_code);
    } 
    else 
    {
      NRF_LOG_INFO("Disconnect initiated successfully after delete peripheral.");
    }
  }

  bonded_count = 0;

  connections_state_unlock();

  configuration_save_with_crc();  
}

uint8_t blink_bonded_device_count(bool show_bonded_count, bool display_key_led)
{
  uint8_t pin_number = 0;
  uint8_t count;

  if (show_bonded_count)
    count = bonded_count;
  else
    count = connected_count;

  switch (count)
  {
    case 0:
      break;

    case 1:
      pin_number = HAL_LED_KEY1;
      break;

    case 2:
      pin_number = HAL_LED_KEY2;
      break;

    case 3:
      pin_number = HAL_LED_KEY3;
      break;

    case 4:
      pin_number = HAL_LED_KEY4;
      break;

    default:
      pin_number = HAL_LED_KEY5;
      break;
  }

  if (pin_number > 0 && display_key_led)
  {
    //peripheral_toggle_led(pin_number);
    peripheral_set_led(pin_number, true); //Ran change to switch on the led instead of blinking. 14/07/25
  }

  return count;
}

bool connections_state_lock(uint32_t wait_ms)
{
  bool res = false;

  if (wait_ms==0)
    wait_ms = portMAX_DELAY;

  if (xSemaphoreTake(connection_list_semaphore, wait_ms))
  {
    res = true;
  }

  return res;
}

void connections_state_unlock(void)
{
  xSemaphoreGive(connection_list_semaphore);
}

void set_ignition_state( uint32_t ignition_state, uint16_t connection_handle )
{
  for ( uint8_t i=0; i<MAX_CONNECTIONS; i++ )
  {
    if ( connections_array[i].conn_handler == connection_handle )
    {
      if ( connections_array[i].ignition_state != ignition_state )
        connections_array[i].is_ignition_state_changed = true;
      connections_array[i].ignition_state = ignition_state;
      break;
    }
  }
}
 //Ran 29/07/25
void setLeachFound(uint8_t found){
leachfound = found;
}

