#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lcd1602.h"
#include "app_state.h"

/*Display configuration structure template. */
typedef struct {
    lcd1602_config_t lcd_cfg;
    QueueHandle_t display_queue;
} display_task_config_t;

/**
 * @brief Starts the display task to update the LCD1602.
 * @param config Pointer to display configuration and queue handle.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t display_task_start(const display_task_config_t *config);

/**
 * @brief Stops the display task.
 */
void display_task_stop(void);

/**
 * @brief Function to post a status update to the display queue.
 * @param queue Display queue handle.
 * @param status Pointer to status data.
 */
void display_task_update_status(QueueHandle_t queue, const display_status_t *status);


#endif