#ifndef LINE_SENSOR_H
#define LINE_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"


 /*Config pin definitions for the 3-channel line tracking module.*/
typedef struct {
    gpio_num_t left_pin;
    gpio_num_t center_pin;
    gpio_num_t right_pin;
    bool active_high;  // Set true if sensor is ACTIVE HIGH(HIGH output on dark surface)
} line_sensor_config_t;

 /*Represents digital states of all 3 sensors.*/
typedef struct {
    bool left;    // true if over black line
    bool center;  // true if over black line
    bool right;   // true if over black line
} line_sensor_state_t; //true members if over black line


 /*Evaluated position state for line follower logic*/
typedef enum {
    LINE_POS_CENTERED = 0, // Line under center sensor (0 1 0)
    LINE_POS_LEFT,     	  // Line drifted left (1 1 0 or 1 0 0)
    LINE_POS_RIGHT,      // Line drifted right (0 1 1 or 0 0 1)
    LINE_POS_LOST,      // All sensors off the line (0 0 0)
    LINE_POS_ALL_DARK  // All sensors on dark surface / intersection (1 1 1)
} line_position_t;

/**
 * @brief Initializes GPIO pins for the 3-channel tracking module.
 * @param config Pointer to line sensor configuration structure.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t line_sensor_init(const line_sensor_config_t *config);

/**
 * @brief Reads digital states from left, center, and right IR sensors.
 * @param config Pointer to line sensor configuration structure.
 * @param state Pointer to store individual raw boolean reads.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t line_sensor_read(const line_sensor_config_t *config, line_sensor_state_t *state);

/**
 * @brief Evaluates raw sensor reads into position state.
 * @param state Pointer to sensor state structure.
 * @return line_position_t Evaluated vehicle position state.
 */
line_position_t line_sensor_evaluate_position(const line_sensor_state_t *state);

#endif