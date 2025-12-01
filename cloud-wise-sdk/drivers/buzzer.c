#include "buzzer.h"

#include "FreeRTOS.h"
#include "hal/hal.h"
#include "hal/hal_boards.h"
#include "hal/hal_drivers.h"
#include "nrf_delay.h"
#include "logic/tracking_algorithm.h"
#include "logic/configuration.h"
#include "semphr.h"
#include "task.h"

#define NRF_LOG_MODULE_NAME cloud_wise_sdk_drivers_buzzer
#define NRF_LOG_LEVEL CLOUD_WISE_DEFAULT_LOG_LEVEL
#include "nrfx_log.h"
NRF_LOG_MODULE_REGISTER();

nrfx_pwm_t  *hal_buzzer;

static xSemaphoreHandle buzzer_semaphore;
static bool             buzzer_on = false;

static uint16_t m_buzzerTune[2] = {HAL_BUZZER_PWM_CYCLE_COUNT / 20};

static nrf_pwm_sequence_t m_tuneSequence = {
    .values = m_buzzerTune, .length = NRF_PWM_VALUES_LENGTH(m_buzzerTune), .repeats = 80, .end_delay = 0};

static uint16_t m_buzzerTune1[2] = {HAL_BUZZER_PWM_CYCLE_COUNT / 20};

static nrf_pwm_sequence_t m_tuneSequence1 = {
    .values = m_buzzerTune1, .length = NRF_PWM_VALUES_LENGTH(m_buzzerTune), .repeats = 1000, .end_delay = 0};

extern DriverBehaviourState driver_behaviour_state;
extern DeviceConfiguration  device_config;

bool buzzer_init(void)
{
    buzzer_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(buzzer_semaphore);

    return true;
}

void pwm_init(uint16_t top_value) 
{
    static bool is_initialized = false;

    nrfx_pwm_config_t const config0 = {
        .output_pins = {HAL_BUZZER_PWM, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED},
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_1MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = top_value,
        .load_mode    = NRF_PWM_LOAD_COMMON,
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    const nrfx_pwm_t *m_pwm = hal_get_pwm();

    if (is_initialized)
      nrfx_pwm_uninit(m_pwm);
    nrfx_pwm_init(m_pwm, &config0, NULL);
    is_initialized = true;
}


bool play_melody(note_t p_melody[], uint16_t base_freq, bool with_semaphore) 
{
    nrf_pwm_values_common_t seq_values[1];
    int i;
    bool res = false;

    const nrfx_pwm_t *m_pwm = hal_get_pwm();

    if (!with_semaphore || xSemaphoreTake(buzzer_semaphore, (portTickType)1)) 
    {
      pwm_init(base_freq);

      i = 0; 
      while (true) 
      {
          if ( p_melody[i].frequency == 0 || p_melody[i].duration_ms == 0)
            break;

          uint16_t duty_cycle = p_melody[i].frequency; // Set duty cycle based on frequency
          seq_values[0] = duty_cycle;

          nrf_pwm_sequence_t const seq = {
              .values.p_common = seq_values,
              .length          = NRF_PWM_VALUES_LENGTH(seq_values),
              .repeats         = 0,
              .end_delay       = 0
          };        

          nrfx_pwm_simple_playback(m_pwm, &seq, 1, NRFX_PWM_FLAG_LOOP);
          nrf_delay_ms(p_melody[i].duration_ms);

          i++;
      }

      xSemaphoreGive(buzzer_semaphore);
    }

    nrfx_pwm_stop(m_pwm, true);

    return res;
}

static void play_note(uint16_t freq, uint16_t base_freq, uint16_t duration_ms) 
{
  note_t melody_note[] =
  { 
    {freq,duration_ms} ,
    {0,0} 
  };

  play_melody(melody_note, base_freq, false);
}


bool buzzer_train_test(uint8_t repeats)
{
#ifdef BUZZER_DISABLE
    nrfx_pwm_simple_playback(hal_buzzer, &m_tuneSequence, repeats, NRFX_PWM_FLAG_STOP);
    return true;
#endif
}


bool buzzer_train(uint8_t repeats, bool high_pitch, bool force)
{
    bool res = false;
    bool is_buzzer_on;

    uint16_t freq;
    uint8_t i;

    if (high_pitch)
        freq = 1000;
    else
        freq = 2000;

    if (xSemaphoreTake(buzzer_semaphore, (portTickType)1)) 
    {
        for (i=0; i<repeats ; i++)
        {
          play_note(NOTE_A4, freq, 100);
          nrf_delay_ms(50);

          xSemaphoreGive(buzzer_semaphore);
        }        

        res = true;
    }

    return res;
}


bool buzzer_long(uint16_t time_ms, bool high_pitch, bool force)
{
    uint16_t freq;
    bool res = false;

    if (high_pitch)
        freq = 1000;
    else
        freq = 2000;

    if (xSemaphoreTake(buzzer_semaphore, (portTickType)1)) 
    {
      play_note(NOTE_A4, freq, time_ms);
      nrf_delay_ms(50);

      xSemaphoreGive(buzzer_semaphore);

      res = true;
    }
}


//////////////
// melodies //
//////////////

note_t melody_reset[] = {
    {NOTE_C4, 50},
    {NOTE_D4, 50},
    {NOTE_E4, 50},
    {NOTE_F4, 50},
    {NOTE_G4, 50},
    {NOTE_A4, 50},
    {NOTE_B4, 50},
    {NOTE_C5, 50},
    {0, 0}    
};

note_t melody_error[] = {
    {NOTE_A4, 200}, 
    {NOTE_SILENT, 50}, 
    {NOTE_F4, 100}, 
    {NOTE_SILENT, 50}, 
    {NOTE_C4, 100}, 
    {0,0}
};

note_t melody_lock[] = {
    {NOTE_C4, 50},
    {NOTE_SILENT, 50}, 
    {NOTE_E4, 50},
    {NOTE_SILENT, 50}, 
    {NOTE_G4, 50},
    {NOTE_SILENT, 50}, 
    {NOTE_C5, 50},
    {0, 0}    
};

note_t melody_success[] = {
    {NOTE_C4, 50},
    {NOTE_SILENT, 20}, 
    {NOTE_E4, 50},
    {NOTE_SILENT, 20}, 
    {NOTE_C5, 50},
    {0, 0}    
};



