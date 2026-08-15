#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

/*Configuration structure template for motor with ESP32 peripherals */
typedef struct {
    // Left Motor (Motor A)
    gpio_num_t ena_pin;         // PWM Speed control pin
    gpio_num_t in1_pin;         // Direction control pin 1
    gpio_num_t in2_pin;         // Direction control pin 2
    ledc_channel_t left_channel;

    // Right Motor (Motor B)
    gpio_num_t enb_pin;         // PWM Speed control pin
    gpio_num_t in3_pin;         // Direction control pin 3
    gpio_num_t in4_pin;         // Direction control pin 4
    ledc_channel_t right_channel;

    // Shared LEDC configuration
    ledc_timer_t timer;
    ledc_mode_t speed_mode;
} motor_config_t;

/*Action enumeration template for all possible motor actions for each of the 2 motors. */
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_BACKWARD,
    MOTOR_TURN_LEFT,    // Left motor backward, Right motor forward (Pivot Turn)
    MOTOR_TURN_RIGHT,   // Left motor forward, Right motor backward (Pivot Turn)
    MOTOR_SOFT_LEFT,    // Left motor stopped/slow, Right motor forward
    MOTOR_SOFT_RIGHT    // Left motor forward, Right motor stopped/slow
} motor_action_t;

/**
 * @brief Initializes GPIOs and LEDC channels for L298N dual H-briidge.
 * @param config Pointer to motor configuration structure.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t motor_driver_init(const motor_config_t *config);

/**
 * @brief Set speed and direction for individual motors.
 * @param config Pointer to motor configuration structure.
 * @param left_speed Speed percentage for left motor(0-100%).
 * @param right_speed Speed percentage for right motor.
 * @param action vehicle direction command, from drive algorithm.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t motor_drive(const motor_config_t *config, uint8_t left_speed, uint8_t right_speed, motor_action_t action);

/**
 * @brief Instantly stops both motors.
 * @param config Pointer to motor configuration structure.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t motor_stop(const motor_config_t *config);


#endif