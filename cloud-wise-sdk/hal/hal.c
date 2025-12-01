#include "hal.h"

#include "app_timer.h"
#include "hal_boards.h"
#include "hal_drivers.h"
#include "nrf_delay.h"
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "nrf_log_ctrl.h"
#include "nrfx_gpiote.h"
#include "nrfx_saadc.h"
#include "nrfx_pwm.h"
#include "drivers/buzzer.h"

#include <string.h>

#define NRF_LOG_MODULE_NAME cloud_wise_sdk_hal
#define NRF_LOG_LEVEL CLOUD_WISE_DEFAULT_LOG_LEVEL
#include "nrfx_log.h"
NRF_LOG_MODULE_REGISTER();

static const nrfx_pwm_t  m_pwm0  = NRFX_PWM_INSTANCE(0);

volatile struct {
    bool lis3dh_int1;
    bool lis3dh_int2;
} m_int_enable = {0};

static void init_gpio(void);
static void init_spi(nrfx_spi_evt_handler_t handler);
static void init_saadc(void);
static void init_saadc_channels(void);
static void calibrate_saadc(void);

// static hal_evt_handler_t p_evt_handler;

static void spi_event_handler(nrf_drv_spi_evt_t const *p_event, void *p_context);

void hal_init(void)
{
    ret_code_t ret;

    init_gpio();

    // close/disable relay by default
    //nrf_gpio_pin_clear(HAL_RELAY);

    /*
    // blinks led to indicate a reset for the user
    nrf_gpio_pin_clear(HAL_LED);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(HAL_LED);
    */

    //nrf_gpio_pin_clear(HAL_LED_STATUS);
    nrf_gpio_pin_clear(HAL_LED_KEY1);
    nrf_gpio_pin_clear(HAL_LED_KEY2);
    nrf_gpio_pin_clear(HAL_LED_KEY3);
    nrf_gpio_pin_clear(HAL_LED_KEY4);
    nrf_gpio_pin_clear(HAL_LED_KEY5);

    /*
    init_twim();
    init_spi(spi_handler);
    init_pwm();
    init_uart();
    init_saadc();
    */

    // init timers module
    /*
    ret = app_timer_init();
    APP_ERROR_CHECK(ret);
    */

    buzzer_init();
}

static void init_gpio(void)
{
    ret_code_t err_code;
    if (nrfx_gpiote_is_init() == false) {
        err_code = nrfx_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    nrf_gpio_cfg_output(HAL_LED_STATUS);
    nrf_gpio_cfg_output(HAL_LED_KEY1);
    nrf_gpio_cfg_output(HAL_LED_KEY2);
    nrf_gpio_cfg_output(HAL_LED_KEY3);
    nrf_gpio_cfg_output(HAL_LED_KEY4);
    nrf_gpio_cfg_output(HAL_LED_KEY5);

    nrf_gpio_cfg_output(HAL_BUTTON_KEY1);
    nrf_gpio_cfg_output(HAL_BUTTON_KEY2);
    nrf_gpio_cfg_input(HAL_BUTTON_KEY3, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(HAL_BUTTON_KEY4, NRF_GPIO_PIN_NOPULL);

    nrf_gpio_cfg_input(HAL_IGN_IN, NRF_GPIO_PIN_PULLUP);
}

static void init_saadc(void)
{
    nrfx_err_t err_code;

    err_code = nrfx_saadc_init(NRFX_SAADC_CONFIG_IRQ_PRIORITY);
    APP_ERROR_CHECK(err_code);

    calibrate_saadc();
    init_saadc_channels();
}

static void calibrate_saadc(void)
{
    nrfx_err_t err_code;

    err_code = nrfx_saadc_offset_calibrate(NULL);
    APP_ERROR_CHECK(err_code);
}

static void init_saadc_channels(void)
{
    nrfx_err_t err_code;

    nrfx_saadc_channel_t channels[] = {NRFX_SAADC_DEFAULT_CHANNEL_SE(HAL_ADC_VBATT, HAL_SAADC_VBAT_CHANNEL)};

    channels[0].channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;
    channels[0].channel_config.gain      = NRF_SAADC_GAIN1_6;
    channels[0].channel_config.acq_time  = NRF_SAADC_ACQTIME_40US;

    err_code = nrfx_saadc_channels_config(channels, sizeof(channels) / sizeof(nrfx_saadc_channel_t));
    if (NRFX_SUCCESS != err_code) {
        NRFX_LOG_ERROR("%s nrfx_saadc_channels_config failed: %s", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    }
    APP_ERROR_CHECK(err_code);
}

uint32_t hal_read_device_serial_number(char *const p_serial, uint8_t max_len)
{
    uint32_t serial_number = NRF_UICR->CUSTOMER[HAL_UICR_DEVICE_SERIAL_NUMBER];

    if (serial_number == UINT32_MAX || serial_number == 0x0) {
        if (p_serial) {
            snprintf(p_serial, max_len, "N/A");
        }

        return 0;
    }

    if (p_serial) {
        snprintf(p_serial, max_len, "%0*X", max_len - 1, serial_number);
    }

    NRFX_LOG_INFO("%s Serial 0x%x (%u)", __func__, serial_number, serial_number);

    return serial_number;
}

static void saadc_event_handler(nrfx_saadc_evt_t const *p_event)
{
    //
}

int16_t hal_read_vdd_raw(void)
{
    nrfx_err_t        err_code;
    nrf_saadc_value_t battery_voltage[1];
    uint32_t          channels = (1 << HAL_SAADC_VBAT_CHANNEL);

    err_code = nrfx_saadc_simple_mode_set(channels, NRFX_SAADC_CONFIG_RESOLUTION, NRFX_SAADC_CONFIG_OVERSAMPLE, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = nrfx_saadc_buffer_set(battery_voltage, 1);
    APP_ERROR_CHECK(err_code);

    err_code = nrfx_saadc_mode_trigger();
    APP_ERROR_CHECK(err_code);

    if (err_code != NRFX_SUCCESS) {
        NRFX_LOG_ERROR("%s %s", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
        return UINT8_MAX;
    }

    return battery_voltage[0];
}

void isticker_bsp_board_sleep(void)
{
    nrf_gpio_pin_set(HAL_LED_STATUS);
}

const nrfx_pwm_t *hal_get_pwm(void)
{
    return &m_pwm0;
}