
#include "app_config.h"


#ifdef RUPTELA_SERVICE_SIMULATION


typedef struct
{
  uint32_t time_sec;  // time ielasped in seconds since year 2020
  uint16_t log_code;
  uint16_t sub_code;
  uint32_t value1;
  uint32_t value2;
}  
Ruptela_Log;

typedef struct
{
  uint16_t command_code;
  uint16_t sub_code;
  uint32_t value1;
  uint32_t value2;
  uint32_t value3;
}  
Ruptela_Command;

// 0xAAAABBBB
// AAAA - log code
// BBBB - Sub code
#define RUPTELA_LOG_DISARMED            0x00000000
#define RUPTELA_LOG_WRONG_CODE          0x00000001
#define RUPTELA_LOG_KEYPAD_LOCKED       0x00000004
#define RUPTELA_LOG_SWITCH_CHANGED      0x00010000
#define RUPTELA_LOG_PASSWORD_CHANGED    0x00020000
#define RUPTELA_LOG_CURRENT_PASSWORD    0x00020001
#define RUPTELA_LOG_BONDING_DELETE      0x00030000
#define RUPTELA_LOG_BONDING_SAVED       0x00030001
#define RUPTELA_LOG_LEACH_CONNECTED     0x00030002
#define RUPTELA_LOG_LEACH_DISCONNECTED  0x00030003
#define RUPTELA_LOG_ARMED               0x00040000
#define RUPTELA_LOG_IGNITION_ON_IN_ARM  0x00050000               
#define RUPTELA_LOG_KEEP_ALIVE          0x00060000
#define RUPTELA_LOG_COUNTTERS           0x00070000
#define RUPTELA_LOG_ENDED_PAIRING       0x00080000  //Ran added ended Pairing 27/07/25  

// 0xAAAABBBB
// AAAA - command code
// BBBB - Sub code
#define RUPTELA_COMMAND_DISARM          0x00000000
#define RUPTELA_COMMAND_ARM             0x00000001
#define RUPTELA_COMMAND_GET_PASSWORD    0x00010000
#define RUPTELA_COMMAND_ENTER_GARAGE    0x00020000
#define RUPTELA_COMMAND_BACK_DEFAULT    0x00020001
#define RUPTELA_COMMAND_ENTER_GARAGE1   0x00020002 // what this command does ?
#define RUPTELA_COMMAND_NEW_PASSWORD    0x00020003
#define RUPTELA_COMMAND_EXIT_GARAGE     0x00020009
#define RUPTELA_COMMAND_SET_TIME        0x00030000
#define RUPTELA_COMMAND_ENABLE_PAIRING  0x00040000  //Ran added enable Pairing 27/07/25  


SecurityState read_new_security_state(void);
void ruptela_send_log(uint32_t log_code, uint8_t* value1, uint8_t* value2, uint16_t sub_code);
void ruptela_decode_command(uint8_t* command_message, Ruptela_Command* command);
bool getEnablePairing(void);
void resetEnablePairing(void);

#endif 