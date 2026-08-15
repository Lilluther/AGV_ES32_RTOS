#include "line_sensor.h"
#include "esp_log.h"

static const char *TAG = "LINE_SENSOR";

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0) 
#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)

esp_err_t line_sensor_init(const line_sensor_config_t *config) {
    
	// Check against null pointer
	CHECK_ARG(config);
	
	// Configure Pins as Input
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->left_pin) |
                        (1ULL << config->center_pin) |
                        (1ULL << config->right_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,   
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
	
	CHECK(gpio_config(&io_conf));
    
	//Log successful initialization.
    ESP_LOGI(TAG, "3-Channel Line Sensor initialized on GPIOs L:%d C:%d R:%d",
             config->left_pin, config->center_pin, config->right_pin);

    return ESP_OK;
}

esp_err_t line_sensor_read(const line_sensor_config_t *config, line_sensor_state_t *state) {
    
	// Check against null pointer and null state
	CHECK_ARG(config && state);

    int raw_left   = gpio_get_level(config->left_pin);
    int raw_center = gpio_get_level(config->center_pin);
    int raw_right  = gpio_get_level(config->right_pin);

    // Invert logic if module outputs LOW on line detection, else will mean configuration is ACTIVE LOW(check header)
    if (config->active_high) {
        state->left   = (raw_left == 1);
        state->center = (raw_center == 1);
        state->right  = (raw_right == 1);
    } else {
        state->left   = (raw_left == 0);
        state->center = (raw_center == 0);
        state->right  = (raw_right == 0);
    }

    return ESP_OK;
}

line_position_t line_sensor_evaluate_position(const line_sensor_state_t *state) {
    
	
	//Check against null state
	if (!state) return LINE_POS_LOST;

	//Evaluate position state
    uint8_t bitmask = (state->left << 2) | (state->center << 1) | (state->right << 0);

    switch (bitmask) {
        case 0b010: // 2: Center only
            return LINE_POS_CENTERED;

        case 0b100: // 4: Left only
        case 0b110: // 6: Left and Center
            return LINE_POS_LEFT;

        case 0b001: // 1: Right only
        case 0b011: // 3: Center and Right
            return LINE_POS_RIGHT;

        case 0b111: // 7: All active
            return LINE_POS_ALL_DARK;

        case 0b000: // 0: None active
        default:
            return LINE_POS_LOST;
    }
}
