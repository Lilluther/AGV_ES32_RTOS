#ifndef HC_SR04_H
#define HC_SR04_H


#include "esp_err.h"
#include "driver/gpio.h"

/*Configuration structure template. */
typedef struct {
    gpio_num_t trigger_pin;
    gpio_num_t echo_pin;
} hc_sr04_config_t;

/**
 * @brief Initializes the HC-SR04 trigger and echo GPIO pins.
 * 
 * @param config Pointer to the configuration structure.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t hc_sr04_init(const hc_sr04_config_t *config);

/**
 * @brief Triggers a distance measurement and measures echo time.
 * @param config Pointer to the initialized configuration structure.
 * @param[out] distance_cm Pointer to store the calculated distance in centimeters.
 * @return esp_err_t ESP_OK on success, ESP_ERR_TIMEOUT if no echo was received.
 */
esp_err_t hc_sr04_read_cm(const hc_sr04_config_t *config, float *distance_cm);



#endif // HC_SR04_H