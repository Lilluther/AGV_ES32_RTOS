#include "ir_receiver.h"
#include "driver/rmt_rx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "IR_RECEIVER";

static rmt_channel_handle_t rx_channel = NULL;
static QueueHandle_t ir_event_queue = NULL;


/*Helper function: Convert raw NEC 32-bit payload into actionable command state. */
static ir_cmd_state_t decode_nec_to_cmd(uint32_t nec_code, const ir_remote_keymap_t *keymap) {
    // Standard NEC 32-bit frame: [Address (8-bit) | ~Address (8-bit) | Command (8-bit) | ~Command (8-bit)]
    uint8_t command = (nec_code >> 8) & 0xFF;

    if (command == keymap->forward_code)  return IR_CMD_FORWARD;
    if (command == keymap->backward_code) return IR_CMD_BACKWARD;
    if (command == keymap->left_code)     return IR_CMD_LEFT;
    if (command == keymap->right_code)    return IR_CMD_RIGHT;
    if (command == keymap->stop_code)     return IR_CMD_STOP;
    if (command == keymap->reset_code)    return IR_CMD_RESET;

    return IR_CMD_UNKNOWN;
}

/*RMT RX Callback to fire when a frame is decoded. */
static bool rmt_rx_done_callback(rmt_channel_handle_t rx_chan, const rmt_rx_done_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_wakeup = pdFALSE;
	
    if (ir_event_queue != NULL) {
        // Post raw timing symbols or data length to processing queue
        xQueueSendFromISR(ir_event_queue, edata, &high_task_wakeup);
    }
    return high_task_wakeup == pdTRUE;
}

esp_err_t ir_receiver_init(const ir_receiver_config_t *config) {
    if (!config) return ESP_ERR_INVALID_ARG;

    // 1. Queue to hold decoded IR events
    ir_event_queue = xQueueCreate(10, sizeof(rmt_rx_done_event_data_t));
    if (!ir_event_queue) {
        ESP_LOGE(TAG, "Failed to create IR event queue");
        return ESP_ERR_NO_MEM;
    }

    // 2. Configure RMT RX Channel (ESP-IDF v6 Driver API)
    rmt_rx_channel_config_t rx_conf = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, // 1 MHz resolution (1 tick = 1 microsecond)
        .mem_block_symbols = 64,  // Standard NEC frame uses 34-68 symbols
        .gpio_num = config->ir_pin,
        .flags.invert_in = false,
        .flags.with_dma = false,
    };

    esp_err_t err = rmt_new_rx_channel(&rx_conf, &rx_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT RX channel");
        return err;
    }

    // 3. Register RX Done Callbacks
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_done_callback,
    };
    err = rmt_rx_register_event_callbacks(rx_channel, &cbs, NULL);
    if (err != ESP_OK) return err;

    // 4. Enable RMT RX Channel
    err = rmt_enable(rx_channel);
    if (err != ESP_OK) return err;

    // Start listening for pulse frames
    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 1250,      // Minimum pulse width filtering
        .signal_range_max_ns = 12000000,  // Maximum frame duration threshold
    };
    rmt_receive(rx_channel, NULL, 0, &receive_config);

    ESP_LOGI(TAG, "IR Receiver initialized on GPIO %d using RMT RX", config->ir_pin);
    return ESP_OK;
}

esp_err_t ir_receiver_read_cmd(const ir_receiver_config_t *config, ir_cmd_state_t *cmd_out, uint32_t timeout_ms) {
    if (!config || !cmd_out) return ESP_ERR_INVALID_ARG;

    rmt_rx_done_event_data_t rx_data;
    if (xQueueReceive(ir_event_queue, &rx_data, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        // Parse RMT symbols into raw bit representations
        rmt_symbol_word_t *symbols = rx_data.received_symbols;
        size_t symbol_num = rx_data.num_symbols;

        uint32_t nec_code = 0;
        // Standard NEC pulse width decoding logic:
        // Logic 0: ~560us pulse + ~560us space
        // Logic 1: ~560us pulse + ~1690us space
        if (symbol_num >= 33) { // 1 leader bit + 32 data bits
            for (size_t i = 1; i <= 32; i++) {
                uint32_t duration = symbols[i].duration1; // Space duration
                nec_code <<= 1;
                if (duration > 1000) { // If space duration > 1000us, it's bit '1'
                    nec_code |= 1;
                }
            }
            *cmd_out = decode_nec_to_cmd(nec_code, &config->keymap);

            // Re-arm RMT driver for the next frame
            rmt_receive_config_t receive_config = {
                .signal_range_min_ns = 1250,
                .signal_range_max_ns = 12000000,
            };
            rmt_receive(rx_channel, NULL, 0, &receive_config);
            return ESP_OK;
        }

        // Re-arm driver if frame was noise/incomplete
        rmt_receive_config_t receive_config = {
            .signal_range_min_ns = 1250,
            .signal_range_max_ns = 12000000,
        };
        rmt_receive(rx_channel, NULL, 0, &receive_config);
    }

    *cmd_out = IR_CMD_NONE;
    return ESP_ERR_TIMEOUT;
}

