#pragma once

// Name of device (Will be included in the advertising data)
#define DEVICE_NAME "Leach"
extern char version[];

// Manufacturer Name String (Device Information Service)
#define MANUFACTURER_NAME "Cloud-Wise"

// Model Number String (Device Information Service)
#if defined(BOARD_PCA10040)
#define MODEL_NUM "DevKit"
#define HARDWARE_REV "1.0"

#elif defined(BOARD_CUSTOM)

#define MODEL_NUM STRINGIFY(BOARD_CUSTOM)

#if BOARD_CUSTOM == LEACH_REV_1
#define HARDWARE_REV "1"

#else

#error "Cloud-Wise Leach Custom Board is unknown " ## BOARD_CUSTOM

#endif

#else

#error "Unknown MODEL_NUM"
#endif

// Firmware Revision String (Device Information Service)
//#define FIRMWARE_REV "1.3.15"  // keypad  Ran change to 15 updates to electric car 04/06/25
//#define FIRMWARE_REV "1.3.16"  // keypad  Ran change to 16 swap spi clk and miso  10/06/25
//#define FIRMWARE_REV "1.3.17"  // keypad  Ran change to 17 enable long key 6 on DISARM  and enable keyboard on IGN 03/06/25 enable bips after 3 minuts on IGN without entering code
//#define FIRMWARE_REV "1.3.18"  // keypad  Ran change to 18 for standard car with leach add bips on IGN active.
//#define FIRMWARE_REV "1.3.19"  // keypad  Ran change to 19 when wlectric car is connected without leach, on IGN without code, it will do 5 bips and after 3 seconds continuasly bips 
                                                           //when benzine car is connected with leach, on IGN without code, it will do continuasly bips 
//#define FIRMWARE_REV "1.3.20"  //instead of blinking leds on peripherial scan and found , switch the leds ON. 
//#define FIRMWARE_REV "1.3.21"  //Add option to enable leach learning by command from the modem
//#define FIRMWARE_REV "1.3.22"  //Add leach disconnected message to the modem. 02/08/25
//#define FIRMWARE_REV "1.3.23"  //stop toggeling red LED on ARM mode  10/08/25 hiddenKeyboard
//#define FIRMWARE_REV "1.3.24"  // Add a reset in case the leach was disconnected and after send message to server 21/08/25 , not hiddenKeyboard, enable pairing 
//#define FIRMWARE_REV "1.3.25"    // solve the problem that the keyboard is not sending notification to server
//#define FIRMWARE_REV "1.3.26"    //Add reset when the modem is disconnected , add option to detect iPhone 
//#define FIRMWARE_REV "1.3.27"    //fix recognision of 2 leach + modem, shave them and connect again after reset.
//#define FIRMWARE_REV "1.3.28"    //Add support to to disarm by the application. in this case the keyboard will do reset before go to arm again. 
//#define FIRMWARE_REV "1.3.29"      //Add support to to disarm by the application with Rubtela serial number
//#define FIRMWARE_REV "1.3.30"      //Add support to to disarm by the IOS application 
#define FIRMWARE_REV "1.3.31"      //Enable to do dual phase for learning, shorten the time to start chaek disarm by App  02/11/25