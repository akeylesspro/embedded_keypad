#pragma once

#include "hal/hal_data_types.h"

typedef struct{                    //Ran add this structure to support save davice name into the configuration. 
char deviceSerial[4][16];
uint8_t macAddr[4][8];
}deviceSerialRead; 

typedef struct{                    //Ran add this structure to support save davice name into the configuration. 
char deviceSerial[4][16];
uint8_t macAddr[4][8];
}deviceSerialWrite;

typedef struct {
    unsigned short max_sleep_time;
     unsigned char DeviceName[16]; // BLEID
    unsigned char DeviceID[28];
} DeviceConfiguration;

typedef struct {
    // 0x00
    unsigned char ignition_state;
    unsigned char relay_normaly_open;
    unsigned char security_state;
    unsigned char is_locked;
    unsigned char garage_mode;
    unsigned char last_reset_code;
    unsigned char reserved_02;
    unsigned char reserved_03;
    unsigned char password[8];

    // 0x10
    unsigned char Leach_address_1[8];
    unsigned char Leach_address_2[8];
    
    // 0x20
    unsigned char canbus_Leach_address[8];
    unsigned char modem_address[8];

    // 0x30
    unsigned char key2[16];
    // 0x40
    //unsigned char key3[64];
    unsigned char serial_1[16];
    unsigned char serial_2[16];
    unsigned char serial_3[16];
    unsigned char serial_4[16];
    // 0x80
    unsigned char key4[32];

    // 0xA0
    unsigned char key5[16];

    // 0xB0
    unsigned       reserved_07;
    unsigned       reserved_08;
    unsigned       reset_count;
    unsigned short power_reset_count;

    unsigned short crc; // crc of first 0xC0-2 bytes

    // 0xC0
    unsigned char reserved_11[16];

    // 0xD0
    unsigned char reserved_12[16];

    // 0xE0
    unsigned char reserved_13[16];

    // 0xF0
    unsigned       reserved_14;
    unsigned       reserved_15;
    unsigned short reserved_16;

    // 0xFA
    unsigned short eprom_manufacture_id;

    // 0xFC
    unsigned eprom_device_id;
} EpromConfiguration;

extern EpromConfiguration eprom_configuration;
void     resetDrviceSerial(void);
void     configuration_init(void);
void     configuration_load(void);
void     configuration_save(void);
void     configuration_set_default(void);
void     configuration_save_with_crc(void);
void     configuration_save_address_list(void);
void     configuration_load_address_list(void);
void     configuration_reset_eprom(void);
void     configuration_print(void);
void     configuration_delete_all_addresses(void);
bool     configuration_is_address_empty(uint8_t* address);
void     configuration_reset_connection_list(void);

uint16_t configuration_calculate_eprom_crc(void);
bool     configuration_get_address( uint8_t address[6], int8_t index );
