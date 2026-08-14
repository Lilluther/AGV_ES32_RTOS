#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

/*Configuration structure template. */
typedef struct {
    gpio_num_t servo_pin;       // GPIO pin connected to servo control signal
    ledc_channel_t channel;     // LEDC channel (LEDC_CHANNEL_0)
    ledc_timer_t timer;         // LEDC timer (LEDC_TIMER_0)
    ledc_mode_t speed_mode;     // LEDC speed mode (LEDC_LOW_SPEED_MODE)
} servo_config_t;

/**
 * @brief Initializes the LEDC peripheral for 50Hz servo PWM control.
 * @param config Pointer to servo configuration structure.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t servo_init(const servo_config_t *config);

/**
 * @brief Sets the angle of the servo motor.
 * @param config Pointer to servo configuration structure.
 * @param angle_degrees Angle in degrees (0 to 180).
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG for invalid angles.
 */
esp_err_t servo_set_angle(const servo_config_t *config, uint32_t angle_degrees);

#endif