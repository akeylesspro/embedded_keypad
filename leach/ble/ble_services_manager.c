#include "ble_services_manager.h"

#include "FreeRTOS.h"
#include "app_error.h"
#include "app_timer.h"
#include "ble.h"
#include "ble/ble_leach.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"
#include "ble_task.h"
#include "logic/commands.h"
#include "logic/configuration.h"
#include "logic/ignition.h"
#include "logic/peripherals.h"
#include "logic/serial_comm.h"
#include "logic/tracking_algorithm.h"
#include "nrf_ble_gatt.h"
#include "nrf_log_ctrl.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_freertos.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdm.h"
#include "semphr.h"
#include "task.h"
#include "version.h"
#include "drivers/buzzer.h"
#include "ble_gap.h"
#include "event_groups.h"
#include "main.h"

#include <string.h>

extern DeviceConfiguration  device_config;
extern DriverBehaviourState driver_behaviour_state;
extern QueueHandle_t queue_connections;
extern EventGroupHandle_t event_ignition_characteristic_changed;
extern deviceSerialWrite device_Serial_write;

#define NRF_LOG_MODULE_NAME ble_services_manager
#define NRF_LOG_LEVEL CLOUD_WISE_DEFAULT_LOG_LEVEL
#include "nrfx_log.h"
NRF_LOG_MODULE_REGISTER();

#include "ble_db_discovery.h"
#include "ble_lbs_c.h"
#include "ble_dis_c.h"
#include "nrf_ble_scan.h"

BLE_LBS_C_ARRAY_DEF(m_lbs_c, NRF_SDH_BLE_CENTRAL_LINK_COUNT);           /**< LED button client instances. */
BLE_DB_DISCOVERY_ARRAY_DEF(m_db_disc, NRF_SDH_BLE_CENTRAL_LINK_COUNT);  /**< Database discovery module instances. */
NRF_BLE_SCAN_DEF(m_scan);                                               /**< Scanning Module instance. */
NRF_BLE_GQ_DEF(m_ble_gatt_queue,                                        /**< BLE GATT Queue instance. */
               NRF_SDH_BLE_CENTRAL_LINK_COUNT,
               NRF_BLE_GQ_QUEUE_SIZE);
BLE_DIS_C_DEF(m_ble_dis_c);

static char const m_target_periph_name[] = "Nordic_Blinky";             /**< Name of the device to try to connect to. This name is searched for in the scanning report data. */
static bool forced_stop_scan = false;

EventGroupHandle_t event_ble_connection_changed;

#define UUID128_FILTER    LBS_UUID_BASE;
#define UUID16_FILTER   BLE_UUID_LEACH_SERVICE

#define BLE_DIS_C_STRING_MAX_LEN 30

#ifdef RUPTELA_SERVICE_SIMULATION
extern QueueHandle_t command_message_queue;
#endif

bool use_mac_address_filter = true;

typedef struct
{
  uint8_t addr[BLE_GAP_ADDR_LEN];                                   
} mac_address_t;

extern ble_peripheral_info_t current_conn_info;

// #define CONSTANT_MAC_ADDRESS

#ifdef CONSTANT_MAC_ADDRESS
// this table for testing constant mac assress
static mac_address_t target_addresses[MAX_TARGET_ADDRESSES] = {    
    { .addr = {0xC4, 0x21, 0xB9, 0xE8, 0x1E, 0xC6} }, // example
    { .addr = {0x62, 0x06, 0x26, 0xBD, 0x4D, 0x74} }, // ruptela
};
#endif


static void db_discovery_init(void);
static void db_disc_handler(ble_db_discovery_evt_t * p_evt);
static void lbs_c_init(void);
static void dis_c_init(void);
static void lbs_c_evt_handler(ble_lbs_c_t * p_lbs_c, ble_lbs_c_evt_t * p_lbs_c_evt);
static void lbs_error_handler(uint32_t nrf_error);
static void scan_init(void);
static void scan_evt_handler(scan_evt_t const * p_scan_evt);
static uint32_t ble_dis_c_all_chars_read(void);
static void ble_dis_c_evt_handler(ble_dis_c_t * p_ble_dis_c, ble_dis_c_evt_t const * p_ble_dis_evt);
static void service_error_handler(uint32_t nrf_error);
static void send_connection_event(uint16_t event_type);
static void ble_dis_c_string_char_log(ble_dis_c_char_type_t            char_type,
                                      ble_dis_c_string_t const * const p_string);

void print_adv_data(const uint8_t *p_data, uint16_t len, bool is_extended);

static char const * const m_dis_char_names[] =
{
    "Manufacturer Name String",
    "Model Number String     ",
    "Serial Number String    ",
    "Hardware Revision String",
    "Firmware Revision String",
    "Software Revision String",
    //"System ID",
    //"IEEE 11073-20601 Regulatory Certification Data List",
    //"PnP ID"
};


// Time from initiating event (connect or start of notification) to first time
// sd_ble_gap_conn_param_update is called (ms)
#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(5000)
// Time between each call to sd_ble_gap_conn_param_update after the first call (ms)
#define NEXT_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(30000)
// Number of attempts before giving up the connection parameter negotiation
#define MAX_CONN_PARAMS_UPDATE_COUNT 3

#define APP_ADV_FAST_INTERVAL 100 // 400 // ????????? // The advertising interval (in units of 0.625 ms. This value corresponds to 250 ms)
#define APP_ADV_FAST_TIMEOUT_IN_SECONDS 0 // (10 * 100) // The Fast advertising duration in units of 10 milliseconds

#define APP_ADV_SLOW_INTERVAL 1800                 // The advertising interval (in units of 0.625 ms. This value corresponds to 562.5 ms)
#define APP_ADV_SLOW_TIMEOUT_IN_SECONDS (60 * 100) // The Slow advertising duration in units of 10 milliseconds

#define APP_BLE_CONN_CFG_TAG 1                     // A tag identifying the SoftDevice BLE configuration
#define APP_BLE_OBSERVER_PRIO 3                    // Application's BLE observer priority. You shouldn't need to modify this value

#define MIN_CONN_INTERVAL (uint16_t) MSEC_TO_UNITS(7.5, UNIT_1_25_MS) // Minimum acceptable connection interval (0.001 seconds)
#define MAX_CONN_INTERVAL (uint16_t) MSEC_TO_UNITS(200, UNIT_1_25_MS) // Maximum acceptable connection interval (0.375 seconds)
#define CONN_SUP_TIMEOUT (uint16_t) MSEC_TO_UNITS(4000, UNIT_10_MS)   // Connection supervisory timeout (2 seconds)
#define SLAVE_LATENCY 0                                               // Slave latency

BLE_ADVERTISING_DEF(m_advertising);                                   // Advertising module instance
NRF_BLE_GATT_DEF(m_gatt);                                             // GATT module instance
BLE_BAS_DEF(m_battery_service);                                       // Battery service


static void init_leach_service(void);

static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context);

static void gatt_init(void);
static void gap_params_init(void);
static void advertising_init(void);
static void conn_params_init(void);
static void conn_evt_len_ext_set(bool status);

static void dfu_init(void);

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; // Handle of the current connection

// Universally unique service identifiers.
static ble_uuid_t m_rsp_uuids[] = {
    {BLE_UUID_LEACH_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN},
};

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went
 * wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code           = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

static void sdh_task_hook(void *p_context)
{
    //
}

void ble_services_init_0(void)
{
    static ble_gap_addr_t mac_address;

    uint32_t res;

    ble_stack_init();

    res = sd_ble_gap_addr_get(&mac_address);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection
 * Parameters Module which are passed to the application.
 *          @note All this function does is to disconnect. This could have been
 * done by simply setting the disconnect_on_fail config parameter, but instead
 * we use the event handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t *p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
        err_code = sd_ble_gap_disconnect(p_evt->conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed
 * to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt) {
    case BLE_ADV_EVT_FAST:
        NRFX_LOG_INFO("%s Fast advertising", __func__);

        break;

    case BLE_ADV_EVT_SLOW:
        NRFX_LOG_INFO("%s Slow advertising", __func__);
        break;

    case BLE_ADV_EVT_IDLE:
        NRFX_LOG_INFO("%s Idle advertising", __func__);
        break;

    default:
        break;
    }
}

static void advertising_config_get(ble_adv_modes_config_t *p_config)
{
    *p_config = (ble_adv_modes_config_t){0};

    p_config->ble_adv_fast_enabled = true;

    // TODO Enable peer manager whitelist
    // p_config->ble_adv_whitelist_enabled = true;

    p_config->ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
    p_config->ble_adv_fast_timeout  = APP_ADV_FAST_TIMEOUT_IN_SECONDS;

    if (APP_ADV_SLOW_TIMEOUT_IN_SECONDS > 0) {
        p_config->ble_adv_slow_enabled  = true;
        p_config->ble_adv_slow_interval = APP_ADV_SLOW_INTERVAL;
        p_config->ble_adv_slow_timeout  = APP_ADV_SLOW_TIMEOUT_IN_SECONDS;
    }
}

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t               err_code;
    ble_advertising_init_t   init = {0};
    ble_advdata_manuf_data_t adv_manuf_data;

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = true;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_rsp_uuids) / sizeof(m_rsp_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_rsp_uuids;

    advertising_config_get(&init.config);

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{    
    ret_code_t err_code = NRF_SUCCESS;

    ble_gap_evt_t const *p_gap_evt = &p_ble_evt->evt.gap_evt;
    uint8_t              rssi;

    switch (p_ble_evt->header.evt_id) 
    {
    case BLE_GAP_EVT_DISCONNECTED:
        NRFX_LOG_INFO("%s Connection %d has been disconnected. Reason: %d", __func__, p_ble_evt->evt.gap_evt.conn_handle,
                      p_ble_evt->evt.gap_evt.params.disconnected.reason);

        m_conn_handle = BLE_CONN_HANDLE_INVALID;

        current_conn_info.conn_handler = p_ble_evt->evt.gap_evt.conn_handle;
        send_connection_event(0x0002);

        // scan_start(false);
        break;

    case BLE_GAP_EVT_CONNECTED:
        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

        current_conn_info.peripheral_type = PERIPHERAL_TYPE_UNKNOWN;

        NRFX_LOG_INFO("Connection %d established, starting DB discovery.", p_gap_evt->conn_handle);
        APP_ERROR_CHECK_BOOL(p_gap_evt->conn_handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT);

        err_code = ble_lbs_c_handles_assign(&m_lbs_c[p_gap_evt->conn_handle],
                                                p_gap_evt->conn_handle,
                                                NULL);
        APP_ERROR_CHECK(err_code);

        err_code = ble_db_discovery_start(&m_db_disc[p_gap_evt->conn_handle], p_gap_evt->conn_handle);
        APP_ERROR_CHECK(err_code);
        
        current_conn_info.conn_handler = m_conn_handle;
        current_conn_info.peer_address = p_gap_evt->params.connected.peer_addr;    

        // choose type according to matched uuid or device info
        current_conn_info.peripheral_type = PERIPHERAL_TYPE_NEW;
        current_conn_info.connection_time = xTaskGetTickCount();

        send_connection_event(0x0001);

        /*
        if (ble_conn_state_central_conn_count() == NRF_SDH_BLE_CENTRAL_LINK_COUNT)
        {
        }
        else
        {
          scan_start();
        }
        */

        // scan_start(false);
        break;

    case BLE_GATTC_EVT_TIMEOUT:
        // Disconnect on GATT Client timeout event.
        NRFX_LOG_DEBUG("GATT Client Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GATTS_EVT_TIMEOUT:
        // Disconnect on GATT Server timeout event.
        NRFX_LOG_DEBUG("GATT Server Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
        NRFX_LOG_INFO("PHY update request.");
        ble_gap_phys_t const phys = {
            .rx_phys = BLE_GAP_PHY_AUTO,
            .tx_phys = BLE_GAP_PHY_AUTO,
        };
        err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
        APP_ERROR_CHECK(err_code);
    } break;

    default:
        // No implementation needed.
        break;
    }
}

/*
bool ble_services_is_connected(void)
{
    return !(m_conn_handle == BLE_CONN_HANDLE_INVALID);
}
*/

bool ble_services_is_connected(void)
{
  uint8_t count = 0;
  
  count = ble_conn_state_central_conn_count();

  if (count>0)
    return true;
  else
    return false;
}



/**@brief Function for starting advertising.
 */
void ble_services_advertising_start(void)
{
    ret_code_t err_code;

    err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile)
 * parameters of the device including the device name, appearance, and the
 * preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    // err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
    err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)device_config.DeviceName, strlen(device_config.DeviceName));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

static void conn_evt_len_ext_set(bool status)
{
    ret_code_t err_code;
    ble_opt_t  opt = {0};

    opt.common_opt.conn_evt_ext.enable = status ? 1 : 0;

    err_code = sd_ble_opt_set(BLE_COMMON_OPT_CONN_EVT_EXT, &opt);
    APP_ERROR_CHECK(err_code);
}

static void disconnect(uint16_t conn_handle, void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_WARNING("Failed to disconnect connection. Connection handle: %d Error: %d", conn_handle, err_code);
    } else {
        NRF_LOG_DEBUG("Disconnected connection handle %d", conn_handle);
    }
}

void ble_services_init_central(void)
{
    gatt_init();
    db_discovery_init();
    lbs_c_init();
    dis_c_init();
    ble_conn_state_init();

    conn_params_init();

    scan_init_by_uuid(true);
    //scan_init_by_white_list();

    nrf_sdh_freertos_init(sdh_task_hook, NULL);

    // Start execution.
    NRF_LOG_INFO("Multilink example started.");
    NRF_LOG_DEBUG("debug log enabled");
    scan_start(true);
}


static void db_discovery_init(void)
{
    ble_db_discovery_init_t db_init;

    memset(&db_init, 0, sizeof(ble_db_discovery_init_t));

    db_init.evt_handler  = db_disc_handler;
    db_init.p_gatt_queue = &m_ble_gatt_queue;

    ret_code_t err_code = ble_db_discovery_init(&db_init);
    APP_ERROR_CHECK(err_code);
}

static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    NRF_LOG_DEBUG("call to ble_lbs_on_db_disc_evt for instance %d and link 0x%x!",
                  p_evt->conn_handle,
                  p_evt->conn_handle);

    ble_lbs_on_db_disc_evt(&m_lbs_c[p_evt->conn_handle], p_evt);
    ble_dis_c_on_db_disc_evt(&m_ble_dis_c, p_evt);
}


static void lbs_c_init(void)
{
    ret_code_t       err_code;
    ble_lbs_c_init_t lbs_c_init_obj;

    lbs_c_init_obj.evt_handler   = lbs_c_evt_handler;
    lbs_c_init_obj.p_gatt_queue  = &m_ble_gatt_queue;
    lbs_c_init_obj.error_handler = lbs_error_handler;

    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        err_code = ble_lbs_c_init(&m_lbs_c[i], &lbs_c_init_obj);
        APP_ERROR_CHECK(err_code);
    }
}

static void ble_dis_c_string_char_log(ble_dis_c_char_type_t            char_type,
                                      ble_dis_c_string_t const * const p_string)
{
    char response_data_string[BLE_DIS_C_STRING_MAX_LEN] = {0};

    if (sizeof(response_data_string) > p_string->len)
    {
        memcpy(response_data_string, p_string->p_data, p_string->len);
        //Ran add check if leach was found
        if(char_type == 1){
        volatile uint8_t check =1;
        char Ruptela[]= {"Ruptela"};
        if(strncmp(response_data_string,Ruptela,sizeof(Ruptela)) !=0){
            setLeachFound(1);
            NRF_LOG_INFO("leach found");
          }else setLeachFound(0);
        } //End Ran ......
        NRF_LOG_INFO("%s: %s",
                     m_dis_char_names[char_type],
                     nrf_log_push((char *) response_data_string));
    }
    else
    {
        NRF_LOG_ERROR("String buffer for DIS characteristics is too short.")
    }
}

static void savePeripherialName(ble_dis_c_char_type_t            char_type,
                                      ble_dis_c_string_t const * const p_string)
{
    char PeripherialName[BLE_DIS_C_STRING_MAX_LEN] = {0};

    if (sizeof(PeripherialName) > p_string->len)
    {
        memcpy(PeripherialName, p_string->p_data, p_string->len);
    }
     return;
}


static void dis_c_init(void)
{
    ret_code_t       err_code;
    ble_dis_c_init_t init;

    memset(&init, 0, sizeof(ble_dis_c_init_t));
    init.evt_handler   = ble_dis_c_evt_handler;
    init.error_handler = service_error_handler;
    init.p_gatt_queue  = &m_ble_gatt_queue;

    err_code = ble_dis_c_init(&m_ble_dis_c, &init);
    APP_ERROR_CHECK(err_code);
}

static uint32_t ble_dis_c_all_chars_read(void)
{
    ret_code_t err_code;
    uint32_t   disc_char_num = 0;

    for (ble_dis_c_char_type_t char_type = (ble_dis_c_char_type_t) 0;
         char_type < BLE_DIS_C_CHAR_TYPES_NUM;
         char_type++)
    {
        err_code = ble_dis_c_read(&m_ble_dis_c, char_type);

        // The NRF_ERROR_INVALID_STATE error code means that the characteristic is not present in DIS.
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            disc_char_num++;
        }
    }

    return disc_char_num;
}

static void lbs_c_evt_handler(ble_lbs_c_t * p_lbs_c, ble_lbs_c_evt_t * p_lbs_c_evt)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    switch (p_lbs_c_evt->evt_type)
    {
        case BLE_LBS_C_EVT_DISCOVERY_COMPLETE:
        {
            ret_code_t err_code;

            // LED Button Service discovered. Enable notification of Button.
            err_code = ble_lbs_c_button_notif_enable(p_lbs_c);
            APP_ERROR_CHECK(err_code);
        } 
        break;

        case BLE_LBS_C_EVT_BUTTON_NOTIFICATION:
        {
        /*
            NRF_LOG_INFO("Link 0x%x, Button state changed on peer to %d",
                         p_lbs_c_evt->conn_handle,
                         p_lbs_c_evt->params.button.button_state);
        */
            set_ignition_state( p_lbs_c_evt->params.button.button_state, p_lbs_c_evt->conn_handle);
        } 
        break;

        #ifdef RUPTELA_SERVICE_SIMULATION
          case BLE_LBS_C_EVT_COMMAND_NOTIFICATION:
          {
            
            xQueueSendFromISR(command_message_queue, p_lbs_c_evt->params.command.command_state, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

              /*
              NRF_LOG_DEBUG("Link 0x%x, command state changed on peer to %d",
                           p_lbs_c_evt->conn_handle,
                           p_lbs_c_evt->params.button.button_state);
              */
          } 
          break;
        #endif
        

        default:
            // No implementation needed.
            break;
    }
}

char testName[30];
char testSerial[30];
char deviceSerialCo = 0;
uint8_t foundRuptela = 0;
extern bool enableToConnectApp;
static void ble_dis_c_evt_handler(ble_dis_c_t * p_ble_dis_c, ble_dis_c_evt_t const * p_ble_dis_evt)
{
    ret_code_t      err_code;
    static uint32_t disc_chars_num     = 0;
    static uint32_t disc_chars_handled = 0;

    switch (p_ble_dis_evt->evt_type)
    {
        case BLE_DIS_C_EVT_DISCOVERY_COMPLETE:
            err_code = ble_dis_c_handles_assign(p_ble_dis_c,
                                                p_ble_dis_evt->conn_handle,
                                                p_ble_dis_evt->params.disc_complete.handles);
            APP_ERROR_CHECK(err_code);

            disc_chars_num     = ble_dis_c_all_chars_read();
            NRF_LOG_INFO("Device Information Service discovered.");
            break;

        case BLE_DIS_C_EVT_DIS_C_READ_RSP:
        {
            ble_dis_c_evt_read_rsp_t const * p_read_rsp = &p_ble_dis_evt->params.read_rsp;

            //Print header log.
            if ((disc_chars_handled == 0) && (disc_chars_num != 0))
            {
                NRF_LOG_INFO("");
                NRF_LOG_INFO("Device Information:");
            }
            
            switch (p_read_rsp->char_type)
            {
                case BLE_DIS_C_MANUF_NAME:
                case BLE_DIS_C_MODEL_NUM:
                case BLE_DIS_C_SERIAL_NUM:
                case BLE_DIS_C_HW_REV:
                case BLE_DIS_C_FW_REV:
                case BLE_DIS_C_SW_REV:
                    NRF_LOG_INFO("device info: %d", p_read_rsp->char_type);
                    if(p_read_rsp->char_type == 1)
                    {            
                    memcpy(testName,p_read_rsp->content.string.p_data,p_read_rsp->content.string.len);
                    }
                    if(p_read_rsp->char_type == BLE_DIS_C_SERIAL_NUM){
                     
                     memcpy(testSerial,p_read_rsp->content.string.p_data,p_read_rsp->content.string.len);    
                       if(memcmp( testName, "Ruptela" , 7) == 0){
                        foundRuptela =1;  
                        char tmpModemSerial[16]={0};
                        tmpModemSerial[0]=testSerial[0];
                        tmpModemSerial[1]=testSerial[1];
                        for(int i=2;i<=7;i++){
                         tmpModemSerial[i]=testSerial[i+6];
                        }
                        //memcpy(device_Serial_write.deviceSerial[deviceSerialCo],testSerial,p_read_rsp->content.string.len);
                        memcpy(device_Serial_write.deviceSerial[deviceSerialCo],tmpModemSerial,16);
                        NRF_LOG_INFO("found Ruptela serial number");
                        deviceSerialCo++;
                       }
                      if((foundRuptela == 1)&&(memcmp( testName, "Leach" , 5) == 0)&&(enableToConnectApp ==1)){
                        memcpy(device_Serial_write.deviceSerial[deviceSerialCo],testSerial,p_read_rsp->content.string.len);
                        NRF_LOG_INFO("found App serial number");
                      }
                     }
                     
                    
                    ble_dis_c_string_char_log(p_read_rsp->char_type, &p_read_rsp->content.string);
                    break;

                    /*
                case BLE_DIS_C_SYS_ID:
                    ble_dis_c_system_id_log(&p_read_rsp->content.sys_id);
                    break;

                case BLE_DIS_C_CERT_LIST:
                   ble_dis_c_cert_list_log(&p_read_rsp->content.cert_list);
                    break;

                case BLE_DIS_C_PNP_ID:
                    ble_dis_c_pnp_id_log(&p_read_rsp->content.pnp_id);
                    break;
                    */

                default:
                    break;
            }

            disc_chars_handled++;
            if(disc_chars_handled == disc_chars_num)
            {
                NRF_LOG_INFO("");
                disc_chars_handled = 0;
                disc_chars_num     = 0;
            }
         }
         break;

        case BLE_DIS_C_EVT_DIS_C_READ_RSP_ERROR:
            NRF_LOG_ERROR("Read request for: %s characteristic failed with gatt_status");
            /*
            NRF_LOG_ERROR("Read request for: %s characteristic failed with gatt_status: 0x%04X.",
                          m_dis_char_names[p_ble_dis_evt->params.read_rsp.char_type],
                          p_ble_dis_evt->params.read_rsp_err.gatt_status);
                          */
            break;

        case BLE_DIS_C_EVT_DISCONNECTED:
            break;
    }
}

static void service_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

static void lbs_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


static void scan_init_by_BLE_name(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;


    memset(&init_scan, 0, sizeof(init_scan));

    init_scan.connect_if_match = true;
    init_scan.conn_cfg_tag     = APP_BLE_CONN_CFG_TAG;
    init_scan.p_conn_param = NULL;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_NAME_FILTER, m_target_periph_name);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_NAME_FILTER, false);
    APP_ERROR_CHECK(err_code);
}


void scan_init_by_uuid(bool with_mac_address_filter) 
{
    ret_code_t err_code;
    nrf_ble_scan_init_t scan_init = {0};
    ble_uuid_t uuid;

    use_mac_address_filter = with_mac_address_filter;

    err_code = nrf_ble_scan_filters_disable(&m_scan);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Failed to disable scan filters: %d", err_code);
        return;
    }

    // Set up the 128-bit UUID filter
    ble_uuid128_t uuid128_filter = UUID128_FILTER;
    err_code = sd_ble_uuid_vs_add(&uuid128_filter, &uuid.type);
    APP_ERROR_CHECK(err_code);

    uuid.uuid = UUID16_FILTER;

    // Initialize the scan parameters.
    scan_init.connect_if_match = true;
    scan_init.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;

    err_code = nrf_ble_scan_init(&m_scan, &scan_init, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Set up the UUID filter
    err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_UUID_FILTER, &uuid);
    APP_ERROR_CHECK(err_code);

    // Enable the UUID filter
    err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_UUID_FILTER, true);
    APP_ERROR_CHECK(err_code);
}

void scan_init_by_white_list(void)
{
    uint8_t mac_address[6];
    bool flag;

    ret_code_t err_code;
    nrf_ble_scan_init_t scan_init = {0};

    err_code = nrf_ble_scan_filters_disable(&m_scan);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Failed to disable scan filters: %d", err_code);
        return;
    }

    scan_init.connect_if_match = true;
    scan_init.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;

    err_code = nrf_ble_scan_init(&m_scan, &scan_init, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Set the address filter
    for (uint8_t i = 0; i < MAX_TARGET_ADDRESSES; i++)
    {
      #ifdef CONSTANT_MAC_ADDRESS        
      if (true)
      #else
      flag = configuration_get_address(mac_address, i);
      if (flag)
      #endif
      {
        #ifdef CONSTANT_MAC_ADDRESS
          err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_ADDR_FILTER, &target_addresses[i]);
        #else
          err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_ADDR_FILTER, mac_address);
        #endif
        APP_ERROR_CHECK(err_code);       
      }
    }

    // Enable the address filter
    err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_ADDR_FILTER, true);
    APP_ERROR_CHECK(err_code);
}


void scan_start(bool force)
{
    ret_code_t ret;
    bool is_start = false;

    if (force)
    {
      forced_stop_scan = false;
      is_start = true;
    }
    else if (!forced_stop_scan)
    {
      is_start = true;
    }

    if (is_start)
    {
      ret = nrf_ble_scan_start(&m_scan);
      APP_ERROR_CHECK(ret);
      NRF_LOG_INFO("Start scanning");
    }
}

void scan_stop(bool force)
{
    NRF_LOG_INFO("Stop scanning");

    nrf_ble_scan_stop();
    
    if (force)
    {
      forced_stop_scan = true;
    }
}

static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
    ret_code_t err_code;

    switch(p_scan_evt->scan_evt_id)
    {
        case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:

            NRF_LOG_INFO("scan error event: %d" , p_scan_evt->scan_evt_id );
        
            err_code = p_scan_evt->params.connecting_err.err_code;
            switch (err_code)
            {
              case NRF_ERROR_CONN_COUNT:
                break;

              default:
                APP_ERROR_CHECK(err_code);
            }                               
        break;

        case NRF_BLE_SCAN_EVT_NOT_FOUND:
            break;

        case NRF_BLE_SCAN_EVT_FILTER_MATCH:
            NRF_LOG_INFO("Filter match");

            const ble_gap_evt_adv_report_t * p_adv_report = p_scan_evt->params.filter_match.p_adv_report;
            print_adv_data(p_adv_report->data.p_data, p_adv_report->data.len, false);
            
            break;

        default:
            NRF_LOG_INFO("scan event: %d" , p_scan_evt->scan_evt_id );
            break;
    }

    
}

bool send_ignition_state_to_peripheral(uint32_t value, uint8_t conn_handler )
{
    ret_code_t err_code;

    // for (uint32_t i = 0; i< NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
      if (conn_handler < NRF_SDH_BLE_CENTRAL_LINK_COUNT)
      {
        if (m_lbs_c[conn_handler].conn_handle == conn_handler)
        {
          err_code = ble_lbs_led_status_send(&m_lbs_c[conn_handler], value);

          if (err_code != NRF_SUCCESS &&
              err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
              err_code != NRF_ERROR_INVALID_STATE)
          {
            //return err_code;
            return false;
          }
        }
      }
    }

    return true;
}

#ifdef RUPTELA_SERVICE_SIMULATION 


bool send_log_to_peripheral(uint8_t* log_message, uint8_t conn_handler )
{
    ret_code_t err_code;

    // for (uint32_t i = 0; i< NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
      if (conn_handler < NRF_SDH_BLE_CENTRAL_LINK_COUNT)
      {
        if (m_lbs_c[conn_handler].conn_handle == conn_handler)
        {
          err_code = ble_lbs_log_send(&m_lbs_c[conn_handler], log_message);
          NRF_LOG_INFO("Update peripheral %d with new state", conn_handler);
          if (err_code != NRF_SUCCESS &&
              err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
              err_code != NRF_ERROR_INVALID_STATE)
          {
            //return err_code;
            return false;
          }
        }
      }
    }

    return true;
}


#endif

static void send_connection_event(uint16_t event_type)
{
  BaseType_t xHigherPriorityTaskWoken, xResult;
  xHigherPriorityTaskWoken = pdFALSE;

  xResult = xEventGroupSetBitsFromISR(
                                event_ble_connection_changed, 
                                event_type,
                                &xHigherPriorityTaskWoken );

  if( xResult != pdFAIL )
  {
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
  }
}

void print_adv_data(const uint8_t *p_data, uint16_t len, bool is_extended) 
{
    uint16_t offset = 0;

    // Parse advertising data for Device Name
    while (offset < len) {
        uint8_t field_length = p_data[offset];
        
        if (field_length == 0 || offset + field_length >= len) {
            break;
        }
        
        uint8_t field_type = p_data[offset + 1];
        const uint8_t *field_data = &p_data[offset + 2];
        uint8_t data_length = field_length - 1;

        // Extract device name
        if (field_type == 0x09 || field_type == 0x08) {  // Complete or Shortened Local Name
            static char name_buf[32]={0};
            memset(name_buf, 0, sizeof(name_buf));
            if (data_length < sizeof(name_buf)) {
                memcpy(name_buf, field_data, data_length);
                name_buf[data_length] = '\0';
                NRF_LOG_INFO("Device Name: %s", name_buf);
                memcpy(device_Serial_write.deviceSerial[deviceSerialCo],name_buf,16);

            }
            break;  // Found name, no need to continue
        }
        
        offset += field_length + 1;
    }
}

void decode_peripheral_device_by_dis(uint8_t* value, uint8_t length, ble_dis_c_char_type_t char_type)
{
  switch (char_type)
  {
    case BLE_DIS_C_MODEL_NUM:
      if ( memcmp( value, "Ruptela" , 7) == 0)
        current_conn_info.peripheral_type = PERIPHERAL_TYPE_MODEM;
      else if ( memcmp( value, "Leach" , 5) == 0)
        current_conn_info.peripheral_type = PERIPHERAL_TYPE_LEACH;
      else if ( memcmp( value, "iPhone" , 6) == 0)  //Ran add according to pol request. 13.09.25
        current_conn_info.peripheral_type = PERIPHERAL_TYPE_LEACH;
      else if ( memcmp( value, "CAN-Leach" , 9) == 0)
        current_conn_info.peripheral_type = PERIPHERAL_TYPE_CANBUS_LEACH;
      break;
  }
}




