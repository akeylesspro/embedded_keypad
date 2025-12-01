#include "configuration.h"

#include "FreeRTOS.h"
#include "drivers/eprom.h"
#include "hal/hal_boards.h"
#include "hal/hal_drivers.h"
#include "semphr.h"
#include "task.h"
#include "tracking_algorithm.h"
#include "monitor.h"
#include "ble/ble_services_manager.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include <string.h>

DeviceConfiguration device_config;
EpromConfiguration  eprom_configuration;

xSemaphoreHandle configuration_semaphore;

deviceSerialRead device_Serial_read;
deviceSerialWrite device_Serial_write;

extern DriverBehaviourState driver_behaviour_state;
extern ble_peripheral_info_t connections_array[];
extern uint8_t bonded_count;
extern uint8_t leach_bonded_count;
extern uint8_t modem_bonded_count;
static uint16_t calculate_crc(uint8_t *ptrPct, uint16_t pctLen, uint16_t crc16);

extern bool afterStudy;

void resetDrviceSerial(void){  //Ran Add this 04/10/25
 for(int i=0; i<4;i++){
   for(int a=0; a<16;a++){
    device_Serial_read.deviceSerial[i][a] = 0xFF;
    device_Serial_write.deviceSerial[i][a] = 0xFF;
    } 
    for(int a=0; a<8;a++){ 
    device_Serial_read.macAddr[i][a] = 0xFF;
    device_Serial_write.macAddr[i][a] = 0xFF;
    }
  }
}

void configuration_init(void)
{
    configuration_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(configuration_semaphore);
}

void configuration_load(void)
{
    uint8_t  buffer[16 + 4];
    uint8_t *ptr;
    uint8_t  i;

    xSemaphoreTake(configuration_semaphore, portMAX_DELAY);

    for (i = 0; i < 16; i++) {
        ptr = eprom_read(buffer, (i * 16), 16);
        memcpy((uint8_t *)(&eprom_configuration) + i * 16, ptr, 16);
    }
    memcpy(device_Serial_read.deviceSerial[0],eprom_configuration.serial_1,16);
    memcpy(device_Serial_read.deviceSerial[1],eprom_configuration.serial_2,16);
    memcpy(device_Serial_read.deviceSerial[2],eprom_configuration.serial_3,16);
    memcpy(device_Serial_read.deviceSerial[3],eprom_configuration.serial_4,16);
    xSemaphoreGive(configuration_semaphore);
     NRF_LOG_INFO("configuration_loaded ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
}

void configuration_save(void)
{
    uint8_t  buffer[16];
    uint8_t *eprom_ptr = (uint8_t *)&eprom_configuration;
    uint8_t  i;

    xSemaphoreTake(configuration_semaphore, portMAX_DELAY);

    for (i = 0; i < 12; i++) {
        memcpy(buffer, eprom_ptr + i * 16, 16);
        eprom_write(buffer, (i * 16), 16);
    }

    xSemaphoreGive(configuration_semaphore);
}

void configuration_reset_eprom(void)
{
  xSemaphoreTake(configuration_semaphore, portMAX_DELAY);
  memset((uint8_t *)&eprom_configuration, 0xFF, (0xC0 - 2));
  xSemaphoreGive(configuration_semaphore);
}

void configuration_set_default(void)
{
    xSemaphoreTake(configuration_semaphore, portMAX_DELAY);

    // memset((uint8_t *)&eprom_configuration, 0xFF, (0xC0 - 2));

    eprom_configuration.ignition_state = 0;
    eprom_configuration.relay_normaly_open = 0;
    eprom_configuration.security_state = SECURITY_STATE_ARMED;
    eprom_configuration.is_locked = 0;
    eprom_configuration.garage_mode = 0;
    eprom_configuration.reset_count = 0;
    strcpy(eprom_configuration.password , "5321");
    
    xSemaphoreGive(configuration_semaphore);
}

uint16_t configuration_calculate_eprom_crc(void)
{
    uint8_t  i;
    uint16_t calculated_crc;

    xSemaphoreTake(configuration_semaphore, portMAX_DELAY);
    calculated_crc = calculate_crc((uint8_t *)(&eprom_configuration), (0xC0 - 2), 0xFFFF);
    xSemaphoreGive(configuration_semaphore);

    return calculated_crc;
}

static uint16_t calculate_crc(uint8_t *ptrPct, uint16_t pctLen, uint16_t crc16)
{
    static uint8_t OddParity[16] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};
    uint16_t       pwr = 0, data = 0;

    // if (pctLen >= 256)
    // 	return 0;

    if (pctLen > 1024) {
        return 0;
    }

    for (pwr = 0; pwr < pctLen; pwr++) {
        data = (uint16_t)ptrPct[pwr];
        data = ((data ^ (crc16 & 0xFF)) & 0xFF);
        crc16 >>= 8;

        if (OddParity[data & 0xF] ^ OddParity[data >> 4]) {
            crc16 ^= 0xC001;
        }

        data <<= 6;
        crc16 ^= data;
        data <<= 1;
        crc16 ^= data;
    }

    return (crc16);
}

void configuration_save_with_crc(void)
{
  NRF_LOG_INFO("configuration_save_with_crc **************************************************************************");
  uint32_t calculated_crc          = configuration_calculate_eprom_crc();
  eprom_configuration.crc = calculated_crc;        
  configuration_save();
}

static bool check_already_bonded( ble_peripheral_info_t* peripheral_info)
{
  if ( memcmp(peripheral_info->peer_address.addr, eprom_configuration.Leach_address_1, 6 ) == 0)
    return true;

  if ( memcmp(peripheral_info->peer_address.addr, eprom_configuration.Leach_address_2, 6 ) == 0)
    return true;

  if ( memcmp(peripheral_info->peer_address.addr, eprom_configuration.canbus_Leach_address, 6 ) == 0)
    return true;

  if ( memcmp(peripheral_info->peer_address.addr, eprom_configuration.modem_address, 6 ) == 0)
    return true;

  return false;
}

void configuration_reset_connection_list(void)
{
  uint8_t i;

  for (i=0 ; i<MAX_CONNECTIONS ; i++)
  {
    connections_array[i].peripheral_type = PERIPHERAL_TYPE_UNKNOWN;
    connections_array[i].is_connected = false;
    connections_array[i].is_ignition_state_changed = false;
    connections_array[i].conn_handler = -1;
  }
}


void configuration_save_address_list(void)
{
  ble_peripheral_info_t* peripheral_info;

  uint8_t i;
  if(afterStudy == true){  //Ran add this 09/10/25
  for (i=0 ; i<bonded_count ; i++)
  {
    peripheral_info = &connections_array[i];

    if ( check_already_bonded(peripheral_info) )
      continue;

    if (peripheral_info->conn_handler < 0 )
      continue;
    //else if (peripheral_info->is_ignition_state_changed )
    //  peripheral_info->peripheral_type = PERIPHERAL_TYPE_LEACH;
    //else
    //  peripheral_info->peripheral_type = PERIPHERAL_TYPE_MODEM;  
               
    NRF_LOG_INFO("peripheral_info->peripheral_type %d", peripheral_info->peripheral_type);
    if (peripheral_info->peripheral_type == PERIPHERAL_TYPE_LEACH && eprom_configuration.Leach_address_1[0] == 0xFF ){
      memcpy( eprom_configuration.Leach_address_1, peripheral_info->peer_address.addr, 6);
      memcpy( eprom_configuration.serial_1, device_Serial_write.deviceSerial[i], 16);
      }
    else if (peripheral_info->peripheral_type == PERIPHERAL_TYPE_LEACH && eprom_configuration.Leach_address_2[0] == 0xFF ){
      memcpy( eprom_configuration.Leach_address_2, peripheral_info->peer_address.addr, 6);
      memcpy( eprom_configuration.serial_2, device_Serial_write.deviceSerial[i], 16);
      }
    else if (peripheral_info->peripheral_type == PERIPHERAL_TYPE_CANBUS_LEACH){
      memcpy( eprom_configuration.canbus_Leach_address, peripheral_info->peer_address.addr, 6);
      memcpy( eprom_configuration.serial_3, device_Serial_write.deviceSerial[i], 16);
      }
    else if (peripheral_info->peripheral_type == PERIPHERAL_TYPE_MODEM){
      memcpy( eprom_configuration.modem_address, peripheral_info->peer_address.addr, 6);
      memcpy( eprom_configuration.serial_4, device_Serial_write.deviceSerial[i], 16);
      }
  }

  configuration_save_with_crc();
  NRF_LOG_INFO("configuration_address_save_after_study ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
  afterStudy = false;
  }
}

void configuration_load_address_list(void)
{
  ble_peripheral_info_t* peripheral_info;

  uint8_t i;

  bonded_count = 0;
  leach_bonded_count = 0;
  modem_bonded_count =0;

  configuration_reset_connection_list();

  if ( !configuration_is_address_empty(eprom_configuration.Leach_address_1) )
  {
    peripheral_info = &connections_array[0];
    peripheral_info->peripheral_type = PERIPHERAL_TYPE_LEACH;
    memcpy( &peripheral_info->peer_address.addr, eprom_configuration.Leach_address_1, 6);
    bonded_count++;
    leach_bonded_count++;
  }

  if ( !configuration_is_address_empty(eprom_configuration.Leach_address_2) )
  {
    peripheral_info = &connections_array[bonded_count];
    peripheral_info->peripheral_type = PERIPHERAL_TYPE_LEACH;
    memcpy( &peripheral_info->peer_address.addr, eprom_configuration.Leach_address_2, 6);
    bonded_count++;
    leach_bonded_count++;
  }
    
  if ( !configuration_is_address_empty(eprom_configuration.canbus_Leach_address) )
  {
    peripheral_info = &connections_array[bonded_count];
    peripheral_info->peripheral_type = PERIPHERAL_TYPE_CANBUS_LEACH;
    memcpy( &peripheral_info->peer_address.addr, eprom_configuration.canbus_Leach_address, 6);
    bonded_count++;
  }

  if ( !configuration_is_address_empty(eprom_configuration.modem_address) )
  {
    peripheral_info = &connections_array[bonded_count];
    peripheral_info->peripheral_type = PERIPHERAL_TYPE_MODEM;
    memcpy( &peripheral_info->peer_address.addr, eprom_configuration.modem_address, 6);
    bonded_count++;
    modem_bonded_count++;
  }
}

bool configuration_get_address( uint8_t address[6], int8_t index )
{
  bool res = false;

  memset(address, 0x00, 6);

  switch (index)
  {
    case 0:
      if ( !configuration_is_address_empty(eprom_configuration.Leach_address_1) )
      {
        memcpy( address, eprom_configuration.Leach_address_1, 6);
        res = true;
      }
      break;

    case 1:
      if ( !configuration_is_address_empty(eprom_configuration.Leach_address_2) )
      {
        memcpy( address, eprom_configuration.Leach_address_2, 6);
        res = true;
      }
      break;

    case 2:
      if ( !configuration_is_address_empty(eprom_configuration.canbus_Leach_address) )
      {
        memcpy( address, eprom_configuration.canbus_Leach_address, 6);
        res = true;
      }
      break;

    case 3:
      if ( !configuration_is_address_empty(eprom_configuration.modem_address) )
      {
        memcpy( address, eprom_configuration.modem_address, 6);
        res = true;
      }
      break;
  }

  return res;
}

void configuration_delete_all_addresses(void)
{
  memset( eprom_configuration.Leach_address_1, 0xFF, 8);
  memset( eprom_configuration.Leach_address_2, 0xFF, 8);
  memset( eprom_configuration.canbus_Leach_address, 0xFF, 8);
  memset( eprom_configuration.modem_address, 0xFF, 8);
}

bool configuration_is_address_empty( uint8_t* address)
{
  uint8_t empty_address[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  bool res = false;

  if (memcmp( address, empty_address, 6) == 0)
    res = true;

  return res;
}

void configuration_print(void)
{
  static char buffer[32];
  static char peipheral_type[16];

  ble_peripheral_info_t* peripheral_info;

  uint8_t i;

  NRF_LOG_INFO("\r\nBonding list:\r\n");

  if (bonded_count == 0)
  {
    NRF_LOG_INFO("bonded device list is empty\r\n");
  }
  else
  {
    for (i=0 ; i<bonded_count ; i++)
    {
      peripheral_info = &connections_array[i];

      memset(peipheral_type,0x00,16);

      switch(peripheral_info->peripheral_type)
      {
        case PERIPHERAL_TYPE_LEACH:
          strcpy(peipheral_type,"Leach");
          break;

        case PERIPHERAL_TYPE_CANBUS_LEACH:
          strcpy(peipheral_type,"CAN Leach");
          break;

        case PERIPHERAL_TYPE_MODEM:
          strcpy(peipheral_type,"Modem");
          break;

        default:
          strcpy(peipheral_type,"Unknown");
          break;
      }

      sprintf (buffer , "device: %s - %d" , peipheral_type, (i+1));
      print_address(buffer , &peripheral_info->peer_address);
    }
  }

  NRF_LOG_FLUSH();
}
