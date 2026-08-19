#ifndef REMOTE_IR_TASK_H
#define REMOTE_IR_TASK_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ir_receiver.h"
#include "motor_driver.h"

/*Remote task configuration structure. */
typedef struct {
    ir_receiver_config_t ir_cfg;
    motor_config_t motor_cfg;
    QueueHandle_t bt_cmd_queue; //Queue to post mode switches to app state 
} remote_ir_config_t;

/**
 * @brief Starts the Remote IR control task.
 * @param config Configuration containing IR receiver pins and motor driver handles.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t remote_ir_task_start(const remote_ir_config_t *config);

/**
 * @brief Stops the Remote IR task.
 */
void remote_ir_task_stop(void);

/**
 * @brief Determine if IR manual driving commands should actively drive the motors.
 * @param enabled True if system is in MANUAL mode.
 */
void remote_ir_set_manual_mode(bool enabled);


#endif