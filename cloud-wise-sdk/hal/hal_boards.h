#pragma once

#include "boards.h"

#define APP_MAJOR_VERSION 1
#define APP_MINOR_VERSION 1
#define APP_BUILD 3

#define HAL_UICR_DEVICE_SERIAL_NUMBER 0

#if defined(BOARD_PCA10040)

#error "Cloud-Wise Leach not configured for DevKit"

#elif defined(BOARD_CUSTOM)

#if BOARD_CUSTOM == LEACH_REV_1

#define HAL_LED_STATUS NRF_GPIO_PIN_MAP(0, 11)
#define HAL_LED_KEY1 NRF_GPIO_PIN_MAP(0, 17)
#define HAL_LED_KEY2 NRF_GPIO_PIN_MAP(0, 12)
#define HAL_LED_KEY3 NRF_GPIO_PIN_MAP(0, 27)
#define HAL_LED_KEY4 NRF_GPIO_PIN_MAP(0, 26)
#define HAL_LED_KEY5 NRF_GPIO_PIN_MAP(0, 25)

#define HAL_BUTTON_KEY1 NRF_GPIO_PIN_MAP(0, 13)
#define HAL_BUTTON_KEY2 NRF_GPIO_PIN_MAP(0, 14)
#define HAL_BUTTON_KEY3 NRF_GPIO_PIN_MAP(0, 15)
#define HAL_BUTTON_KEY4 NRF_GPIO_PIN_MAP(0, 16)

#define HAL_IGN_IN NRF_GPIO_PIN_MAP(0, 2)

#define HAL_BUZZER_PWM NRF_GPIO_PIN_MAP(0, 3)
#define HAL_BUZZER_PWM_CYCLE_COUNT 2000

#define HAL_SPI1_CLK NRF_GPIO_PIN_MAP(0, 7)
#define HAL_SPI1_MOSI NRF_GPIO_PIN_MAP(0, 9)
#define HAL_SPI1_MISO NRF_GPIO_PIN_MAP(0, 6)
#define HAL_SPI1_SS NRF_GPIO_PIN_MAP(0, 8)

#define HAL_SPI_FLASH_RESETN NRF_GPIO_PIN_MAP(0, 9)

#define HAL_PDM_CLK NRF_GPIO_PIN_MAP(0, 7)
#define HAL_PDM_DIN NRF_GPIO_PIN_MAP(0, 8)

#define HAL_ADC_VBATT NRF_SAADC_INPUT_VDD

#else

#error "Cloud-Wise Leach Custom Board is unknown " ## BOARD_CUSTOM

#endif

#else
#error "Cloud-Wise Leach Board is not defined"
#endif
