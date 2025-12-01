#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "logic/monitor.h"
#include "Ruptela.h"
#include "logic/monitor.h"
#include "logic/configuration.h"
#include "logic/state_machine.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "FreeRTOS.h"
#include "semphr.h"


#ifdef RUPTELA_SERVICE_SIMULATION 

extern xSemaphoreHandle security_state_semaphore;
extern uint32_t time_since_2020;

static SecurityState security_state = SECURITY_STATE_NONE;

static bool is_valid_password( uint8_t* new_password);
bool enablePairing = false;  //Ran added enablePairing 27/07/25  

void ruptela_send_log(uint32_t log_code, uint8_t* value1, uint8_t* value2, uint16_t sub_code)
{
  Ruptela_Log log;

  log.time_sec = get_time_in_seconds();
  log.log_code = (log_code >> 16);
  log.sub_code = (log_code & 0xFFFF);

  if (sub_code>0)
  {
    log.sub_code &= 0xFFFF0000;
    log.sub_code |= sub_code;
  }

  if (value1 != NULL)
    memcpy( (uint8_t*)&log.value1, value1, 4);
  else
    log.value1 = 0;

  if (value2 != NULL)
    memcpy( (uint8_t*)&log.value2, value2, 4);
  else
    log.value2 = 0;

  NRF_LOG_INFO( "Periphral send log: 0x%X, sub code:  0x%X, value1: 0x%X, value2: 0x%X", log.log_code, log.sub_code, log.value1, log.value2 );
  send_log( (uint8_t*)&log );
  
  NRF_LOG_INFO( "Ruptela send log 0x%X", log_code );
}

void ruptela_decode_command(uint8_t* command_message, Ruptela_Command* command)
{
  // static Ruptela_Command command;
  static uint8_t password[6];

  uint32_t value1;
  uint32_t value2;

  memcpy( (uint8_t*)&command->command_code, &command_message[0], 2);
  memcpy( (uint8_t*)&command->sub_code, &command_message[2], 2);
  memcpy( (uint8_t*)&command->value1, &command_message[4], 4);
  memcpy( (uint8_t*)&command->value2, &command_message[8], 4);
  memcpy( (uint8_t*)&command->value3, &command_message[12], 4);

  uint32_t command_code = (((uint32_t)command->command_code)<<16) + command->sub_code;

  switch(command_code)
  {
    case RUPTELA_COMMAND_DISARM:                 
      xSemaphoreTake(security_state_semaphore, portMAX_DELAY);
      security_state = SECURITY_STATE_DISARMED;
      xSemaphoreGive(security_state_semaphore);

      success_buzzer();
      NRF_LOG_INFO("Roptela Disarm: %dsec", command->value1);

      value2 = 1;
      ruptela_send_log(RUPTELA_LOG_DISARMED, eprom_configuration.password, (uint8_t*)&value2, 0);                    
      break;

    case RUPTELA_COMMAND_ARM:      
      xSemaphoreTake(security_state_semaphore, portMAX_DELAY);
      security_state = SECURITY_STATE_ARMED;
      xSemaphoreGive(security_state_semaphore);

      NRF_LOG_INFO( "Roptela Arm");
      break;

    case RUPTELA_COMMAND_NEW_PASSWORD:     
      memset(password, 0x00, 6);
      memcpy( password, (uint8_t*)&command->value1 , 4);

      value2 = 1;
      if ( is_valid_password(password) )
      {
        // replace passord
        strcpy( eprom_configuration.password, password );                   
        configuration_save_with_crc();
        NRF_LOG_INFO( "Roptela Password changed successfully");
      }
      else
      {
        memset(password, 0x00, 6);
        strcpy( password, eprom_configuration.password );                   
        NRF_LOG_INFO( "Roptela Invalid password");
        value2 = 2;
      }

      ruptela_send_log(RUPTELA_LOG_PASSWORD_CHANGED, password, (uint8_t*)&value2, 0);     
      break;

    case RUPTELA_COMMAND_GET_PASSWORD:
      memset(password, 0x00, 6);
      memcpy( password, eprom_configuration.password , 4);
      ruptela_send_log(RUPTELA_LOG_CURRENT_PASSWORD, password, NULL, 0);                                
      NRF_LOG_INFO( "Roptela Get password");
      break;  
      
   case RUPTELA_COMMAND_BACK_DEFAULT:
    configuration_set_default();
    configuration_save_with_crc();
    NRF_LOG_INFO("Roptela Back to default settings");
    break;
      
    case RUPTELA_COMMAND_SET_TIME:
      
      time_since_2020 = command->value1;
      NRF_LOG_INFO( "Roptela Time set by remote %d", command->value1 );      
      break; 
    //Ran added enablePairing 27/07/25  
    case RUPTELA_COMMAND_ENABLE_PAIRING:
      NRF_LOG_INFO( "Roptela Got enable pairing from server%d"); 
      enablePairing = true; 
    break; 

    default:
      NRF_LOG_INFO( "Unkown command" );
      break;
  }
   
  NRF_LOG_FLUSH();
}

//Ran Add the setter and getter of enablePairing 27/07/25
bool getEnablePairing(void){
return enablePairing;
}

void resetEnablePairing(void){
enablePairing = false;
}

SecurityState read_new_security_state(void)
{
  SecurityState new_security_state;

  xSemaphoreTake(security_state_semaphore, portMAX_DELAY);
  new_security_state = security_state;
  security_state = SECURITY_STATE_NONE;
  xSemaphoreGive(security_state_semaphore);

  return new_security_state;
}


static bool is_valid_password( uint8_t* new_password)
{
  bool res = true;
  uint8_t i;

  for (i=0; i<4 ; i++)
  {
    if ( new_password[i] < 0x31 || new_password[i] > 0x35 )
    {
      res = false;
      break;
    }
  }

  return res;
}

#endif