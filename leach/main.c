#include "FreeRTOS.h"
#include "app_error.h"
#include "app_timer.h"
#include "ble.h"
#include "ble/ble_services_manager.h"
#include "ble/ble_task.h"
#include "event_groups.h"
#include "hal/hal_boards.h"
#include "logic/commands.h"
#include "logic/configuration.h"
#include "logic/flash_data.h"
#include "logic/monitor.h"
#include "logic/peripherals.h"
#include "logic/serial_comm.h"
#include "logic/state_machine.h"
#include "logic/tracking_algorithm.h"
#include "logic/keypad.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrfx_log.h"
#include "semphr.h"
#include "task.h"
#include "version.h"
#include "drivers/buzzer.h"

#include <stdbool.h>
#include <stdint.h>

extern xSemaphoreHandle clock_semaphore;
extern xSemaphoreHandle watchdog_monitor_semaphore;
extern xSemaphoreHandle connection_list_semaphore;

extern EventGroupHandle_t event_ignition_characteristic_changed;
extern EventGroupHandle_t event_ble_connection_changed;

static uint32_t reset_count __attribute__((section(".non_init")));
static uint32_t test_value __attribute__((section(".non_init")));

bool power_up = false;

uint32_t reset_count_x;

#if NRF_LOG_ENABLED
static TaskHandle_t m_logger_thread; // Logger thread
#endif

TaskHandle_t driver_behaviour_task_handle;

void init_tasks(void);

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the various modules initialization.
 *
 * @details Initializes various modules and services.
 */

static void modules_init(void)
{
    ret_code_t err_code;

    // Initialize timer module.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    NRFX_LOG_INFO("%s");

    ret_code_t err_code;

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the nrf log module.
 */
__STATIC_INLINE void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Thread for handling the logger.
 *
 * @details This thread is responsible for processing log entries if logs are deferred.
 *          Thread flushes all log entries and suspends. It is resumed by idle task hook.
 *
 * @param[in]   arg   Pointer used for passing some arbitrary information (context) from the
 *                    osThreadCreate() call to the thread.
 */
static void logger_thread(void *arg)
{
    UNUSED_PARAMETER(arg);

    while (1) {
        // ????????? NRF_LOG_FLUSH();

        vTaskSuspend(NULL); // Suspend myself
    }
}

/**@brief A function which is hooked to idle task
 */
void vApplicationIdleHook(void)
{
#if NRF_LOG_ENABLED
    vTaskResume(m_logger_thread);
#endif
}

/**@brief Function for initializing the clock.
 */
static void clock_init(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);
}

static TaskHandle_t m_monitor_thread;
static TaskHandle_t m_ble_thread;
static TaskHandle_t m_keypad_thread;

#ifdef RUPTELA_SERVICE_SIMULATION
  QueueHandle_t command_message_queue;
  xSemaphoreHandle security_state_semaphore;
#endif

extern uint32_t monitor_timer;



int main(void)
{
    log_init();

    clock_init();

    if (test_value != 0x5A5A5A5A) {
        test_value  = 0x5A5A5A5A;
        reset_count = 0;
        power_up = true;

        SetTimeFromString("010120", "000000");
    } else {
        reset_count++;
    }

    reset_count_x = reset_count;
    monitor_timer = 0;

    // Start execution.
    if (pdPASS != xTaskCreate(logger_thread, "NRF_LOG", 256, NULL, 1, &m_logger_thread)) {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    // Activate deep sleep mode.
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    modules_init();
    NRFX_LOG_INFO("Starting: %s %s (%s) FreeRTOS %s\n", DEVICE_NAME, FIRMWARE_REV, MODEL_NUM, tskKERNEL_VERSION_NUMBER);
    // NRFX_LOG_INFO("Debug level %d", NRF_LOG_DEFAULT_LEVEL);
    NRFX_LOG_DEBUG("debug level on");
    NRF_LOG_FLUSH();

    peripherals_init();

    state_machine_init();
    resetDrviceSerial(); //Ran add this 04/10/25

    NRF_LOG_FLUSH();

    if (pdPASS != xTaskCreate(monitor_thread, "Monitor", 256, NULL, 1, &m_monitor_thread)) {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    if (pdPASS != xTaskCreate(ble_thread, "BLE", 256, NULL, 1, &m_ble_thread)) {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    if (pdPASS != xTaskCreate(keypad_thread, "Keypad", 256, NULL, 2, &m_keypad_thread)) {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    init_tasks();

    NRFX_LOG_INFO("%s Free Heap: %u", __func__, xPortGetFreeHeapSize());

    // Start FreeRTOS scheduler.
    vTaskStartScheduler();

    for (;;) {
        APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
    }
}

void SuspendAllTasks(void)
{
    // xTaskSuspend(monitor_thread);
}


void init_tasks(void)
{
    clock_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(clock_semaphore);

    connection_list_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(connection_list_semaphore);
   
    watchdog_monitor_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(watchdog_monitor_semaphore);

    event_ignition_characteristic_changed = xEventGroupCreate();
    event_ble_connection_changed = xEventGroupCreate();

    #ifdef RUPTELA_SERVICE_SIMULATION    
    security_state_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(security_state_semaphore);
    #endif
    
    init_ble_task();
    configuration_init();
    keypad_init_task();
}

void send_event_from_isr( EventGroupHandle_t event, EventBits_t uxBitsToSet )
{
    BaseType_t xHigherPriorityTaskWoken, xResult;

    /* xHigherPriorityTaskWoken must be initialised to pdFALSE. */
    xHigherPriorityTaskWoken = pdFALSE;

    /* Set bit 0 and bit 4 in xEventGroup. */
    xResult = xEventGroupSetBitsFromISR(
                                event,   /* The event group being updated. */
                                uxBitsToSet, /* The bits being set. */
                                &xHigherPriorityTaskWoken );

    /* Was the message posted successfully? */
    if( xResult != pdFAIL )
    {
        /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
           switch should be requested. The macro used is port specific and will
           be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
           the documentation page for the port being used. */
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}