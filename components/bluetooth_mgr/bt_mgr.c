#include "bt_mgr.h"
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"

static const char *TAG = "BT_MGR";

static QueueHandle_t app_bt_queue = NULL;
static uint32_t active_spp_handle = 0;
static bool client_connected = false;

// Convert string to uppercase for easy command matching
static void str_toupper(char *str) {
    while (*str) {
        *str = toupper((unsigned char)*str);
        str++;
    }
}

// SPP Event Callback
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            ESP_LOGI(TAG, "ESP_SPP_INIT_EVT: Starting SPP server...");
            esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "AGV_SPP_SERVER");
            break;

        case ESP_SPP_DATA_IND_EVT: // Incoming Bluetooth Serial Data
            if (param->data_ind.len > 0) {
                char recv_buf[32] = {0};
                size_t len = param->data_ind.len < 31 ? param->data_ind.len : 31;
                memcpy(recv_buf, param->data_ind.data, len);
                recv_buf[len] = '\0';

                // Strip trailing newline/carriage returns
                char *pos;
                if ((pos = strchr(recv_buf, '\r')) != NULL) *pos = '\0';
                if ((pos = strchr(recv_buf, '\n')) != NULL) *pos = '\0';

                ESP_LOGI(TAG, "Received raw BT message: '%s'", recv_buf);

                // Prepare message struct
                bt_msg_t msg;
                strncpy(msg.raw_msg, recv_buf, sizeof(msg.raw_msg));
                str_toupper(recv_buf);

                if (strcmp(recv_buf, "OFF") == 0) {
                    msg.cmd = BT_CMD_SET_OFF;
                } else if (strcmp(recv_buf, "MANUAL") == 0) {
                    msg.cmd = BT_CMD_SET_MANUAL;
                } else if (strcmp(recv_buf, "AUTO") == 0) {
                    msg.cmd = BT_CMD_SET_AUTO;
                } else {
                    msg.cmd = BT_CMD_UNKNOWN;
                }

                // Post command to inter-task queue
                if (app_bt_queue != NULL) {
                    xQueueSend(app_bt_queue, &msg, portMAX_DELAY);
                }
            }
            break;

        case ESP_SPP_SRV_OPEN_EVT: // Client Connected
            ESP_LOGI(TAG, "Bluetooth Client Connected handle: %lu", (unsigned long)param->srv_open.handle);
            active_spp_handle = param->srv_open.handle;
            client_connected = true;
            bt_mgr_send_response("AGV Connected. Commands: MANUAL, AUTO, OFF\r\n");
            break;

        case ESP_SPP_CLOSE_EVT: // Client Disconnected
            ESP_LOGI(TAG, "Bluetooth Client Disconnected");
            client_connected = false;
            active_spp_handle = 0;
            break;

        default:
            break;
    }
}

// GAP Callback (for device discovery/pairing)
static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (event == ESP_BT_GAP_AUTH_CMPL_EVT) {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "authentication success: %s", param->auth_cmpl.device_name);
        } else {
            ESP_LOGE(TAG, "authentication failed, status: %d", param->auth_cmpl.stat);
        }
    }
}

esp_err_t bt_mgr_init(const char *device_name, QueueHandle_t event_queue) {
    if (!device_name || !event_queue) return ESP_ERR_INVALID_ARG;

    app_bt_queue = event_queue;

    // Release memory reserved for BLE since we only need Classic BT SPP
    esp_err_t err = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "BLE Mem release failed or skipped: %s", esp_err_to_name(err));
    }

    // Initialize Controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK) return err;
    if ((err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) return err;

    // Initialize Bluedroid Stack
    if ((err = esp_bluedroid_init()) != ESP_OK) return err;
    if ((err = esp_bluedroid_enable()) != ESP_OK) return err;

    // Register Callbacks
    if ((err = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) return err;
    if ((err = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) return err;

    // Initialize SPP Mode
    esp_spp_cfg_t spp_cfg = {
        .mode = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = true,
        .tx_buffer_size = 0,
    };
    if ((err = esp_spp_enhanced_init(&spp_cfg)) != ESP_OK) return err;

    // Set Bluetooth Device Name
    esp_bt_gap_set_device_name(device_name);
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    ESP_LOGI(TAG, "Bluetooth SPP initialized as '%s'", device_name);
    return ESP_OK;
}

esp_err_t bt_mgr_send_response(const char *msg) {
    if (!client_connected || active_spp_handle == 0 || !msg) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_spp_write(active_spp_handle, strlen(msg), (uint8_t *)msg);
}