#include "hc_sr04.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "esp_log.h"

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0) 
#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)

static const char *TAG = "HC_SR04";

#define HC_SR04_TIMEOUT_US  1800     // ~1.8 ms timeout (approx 30 cm max range)
#define SOUND_SPEED_CM_US   0.0343f   // Speed of sound in cm per microsecond 

esp_err_t hc_sr04_init(const hc_sr04_config_t *config) {
	
	// Check against null pointer
	CHECK_ARG(config);

    // Configure Trigger Pin as Output
	CHECK(gpio_set_direction(config->trigger_pin, GPIO_MODE_OUTPUT));
  
    // Configure Echo Pin as Input
	CHECK(gpio_set_direction(config->echo_pin, GPIO_MODE_INPUT));
 
    gpio_set_level(config->trigger_pin, 0);
	
    return ESP_OK;
}

esp_err_t hc_sr04_read_cm(const hc_sr04_config_t *config, float *distance_cm) {

	// Check against null pointer
	CHECK_ARG(config && distance_cm);

	/*TODO: CONFIGURE THE FUNCTIONS TO CORRESPOND TO CHECK(x) WHEN I REFACTOR CODE*/
	
    // Send 10us HIGH pulse to Trigger Pin
    gpio_set_level(config->trigger_pin, 1);
    ets_delay_us(10);
    gpio_set_level(config->trigger_pin, 0);

    // Wait for Echo pin to go HIGH
    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(config->echo_pin) == 0) {
        if ((esp_timer_get_time() - start_time) > HC_SR04_TIMEOUT_US) {
            ESP_LOGW(TAG, "Echo start timeout");
            return ESP_ERR_TIMEOUT;
        }
    }

    // Measure duration while Echo pin is HIGH
    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(config->echo_pin) == 1) {
        if ((esp_timer_get_time() - echo_start) > HC_SR04_TIMEOUT_US) {
            ESP_LOGW(TAG, "Echo duration timeout");
            return ESP_ERR_TIMEOUT;
        }
    }
    int64_t echo_end = esp_timer_get_time();

    // Calculate distance: (time * speed_of_sound) / 2
    int64_t pulse_duration = echo_end - echo_start;
    *distance_cm = (pulse_duration * SOUND_SPEED_CM_US) / 2.0f;

    return ESP_OK;
}
