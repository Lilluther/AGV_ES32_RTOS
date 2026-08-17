#ifndef AUTO_NAV_TASK_H
#define AUTO_NAV_TASK_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hc_sr04.h"
#include "servo_driver.h"
#include "line_sensor.h"
#include "motor_driver.h"

/*Autonomous navigation configuration structure template. */
typedef struct {
    hc_sr04_config_t hc_sr04_cfg;
    servo_config_t servo_cfg;
    line_sensor_config_t line_cfg;
    motor_config_t motor_cfg;
    float obstacle_threshold_cm; // How far before obstacle evade kicks in
} auto_nav_config_t;

/**
 * @brief Starts the autonomous navigation task.
 * @param config Pointer to hardware configurations.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t auto_nav_task_start(const auto_nav_config_t *config);

/**
 * @brief Suspends or stops autonomous driving task.
 */
void auto_nav_task_stop(void);


#endif