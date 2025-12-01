


void keypad_thread(void *pvParameters);

typedef struct {

  uint16_t time_ms;

  uint8_t key;
  uint8_t is_long_key : 1;
  uint8_t key_release : 1;
  uint8_t reserved : 6;
} 
KeypadMessage;


void keypad_init_task(void);
bool keypad_receive_key(KeypadMessage* msg);