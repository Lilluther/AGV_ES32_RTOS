#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

/*Command states enum template for High Level usecase*/
typedef enum {
    IR_CMD_NONE = 0,
    IR_CMD_FORWARD,
    IR_CMD_BACKWARD,
    IR_CMD_LEFT,
    IR_CMD_RIGHT,
    IR_CMD_STOP,
    IR_CMD_RESET,
    IR_CMD_UNKNOWN
} ir_cmd_state_t;

/**
 *Structure template of the NEC Command keymap for the IR remote, receiver is VS1838B on HW-477 module.
 *TODO: Update the struct values for my remote usecase
*/
typedef struct {
    uint8_t forward_code;   //Correspond to motor actions in Manual Mode
    uint8_t backward_code;  
    uint8_t left_code;      
    uint8_t right_code;     
    uint8_t stop_code;      
    uint8_t reset_code;     //To change RESET system ready to receive the Mode Setting(Manual/Auto) via Bluetooth
} ir_remote_keymap_t;

/*Structure template for all IReceiver inputs & pin map */
typedef struct {
    gpio_num_t ir_pin;             // GPIO connected to HW-477 OUT
    ir_remote_keymap_t keymap;     // Hex map for key codes
} ir_receiver_config_t;

/**
 * @brief Initializes RMT peripheral rx channel for NEC IR decoding.
 * @param config Pointer to IR receiver configuration structure.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t ir_receiver_init(const ir_receiver_config_t *config);

/**
 * @brief Reads incoming IR data and resolves it into a mapped command.
 * @param config Pointer to IR receiver configuration structure.
 * @param cmd_out Pointer to store resolved command state.
 * @param timeout_ms Maximum time to wait for an IR signal (0 for non-blocking).
 * @return esp_err_t ESP_OK on valid command, ESP_ERR_TIMEOUT if no command received.
 */
esp_err_t ir_receiver_read_cmd(const ir_receiver_config_t *config, ir_cmd_state_t *cmd_out, uint32_t timeout_ms);


#endif