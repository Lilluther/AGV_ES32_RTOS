#ifndef BT_MGR_H
#define BT_MGR_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Bluetooth high-level system commands.
 */
typedef enum {
    BT_CMD_NONE = 0,
    BT_CMD_SET_OFF,
    BT_CMD_SET_MANUAL,
    BT_CMD_SET_AUTO,
    BT_CMD_UNKNOWN
} bt_cmd_type_t;

/**
 * @brief Message structure passed to the event queue.
 */
typedef struct {
    bt_cmd_type_t cmd;
    char raw_msg[32];
} bt_msg_t;

/**
 * @brief Initializes Bluetooth Classic SPP stack and registers the command queue.
 * 
 * @param device_name Bluetooth discovery name (e.g., "AGV_ESP32_BOT")
 * @param event_queue Queue handle where incoming BT commands will be posted.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t bt_mgr_init(const char *device_name, QueueHandle_t event_queue);

/**
 * @brief Sends a string message back over Bluetooth to the connected client terminal.
 * 
 * @param msg String payload to transmit.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t bt_mgr_send_response(const char *msg);

#endif // BT_MGR_H