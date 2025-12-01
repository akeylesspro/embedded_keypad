#pragma once

#include "hal/hal_data_types.h"
#include "nrfx_pwm.h"

bool buzzer_init(void);

bool buzzer_train(uint8_t repeats, bool high_pitch, bool force);
bool buzzer_long(uint16_t time_ms, bool high_pitch, bool force);


#define BUZZER_MODE_NONE 0
#define BUZZER_MODE_ON 1
#define BUZZER_MODE_OFFROAD 2
#define BUZZER_MODE_DEBUG 3

#define BUZZER_MODE_MAX 10

#define BUZZER_MODE_SLALUM 11
#define BUZZER_MODE_BUMPER 12

//////////////
// memodies //
//////////////

typedef struct {
    uint16_t frequency;
    uint16_t duration_ms;
} note_t;

void pwm_init(uint16_t top_value);
bool play_melody(note_t p_melody[], uint16_t base_freq, bool with_semaphore);

#define NOTE_C3  130
#define NOTE_D3  147
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_G3  196
#define NOTE_A3  220
#define NOTE_B3  247

#define NOTE_C4  261
#define NOTE_D4  294
#define NOTE_E4  329
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

#define NOTE_SILENT  1
#define NOTE_STOP  0

extern note_t melody_reset[];
extern note_t melody_error[];
extern note_t melody_success[];
extern note_t melody_lock[];

