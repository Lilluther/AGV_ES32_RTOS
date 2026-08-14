#include "servo_driver.h"
#include "esp_log.h"

static const char *TAG = "SERVO_DRIVER";

#define SERVO_PWM_FREQ_HZ      50                 // 50 Hz = 20 ms period
#define SERVO_PWM_RESOLUTION   LEDC_TIMER_13_BIT  // 13-bit resolution 

/*Min and Max pulse widths in microseconds*/
#define SERVO_MIN_PULSE_WIDTH_US   500   // 0.5 ms for 0 degrees
#define SERVO_MAX_PULSE_WIDTH_US  2500   // 2.5 ms for 180 degrees

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)

 /*Helper function to convert pulse width in microseconds to 13-bit LEDC duty count.*/
static uint32_t us_to_duty(uint32_t us) {
    // Duty = (Pulse Width in us / Period in us) * Max Duty Count
    // Period = 20,000 us (50Hz)
    // Max Duty Count = 2^13 - 1 = 8191
    return (uint32_t)(((uint64_t)us * 8191) / 20000);
}

esp_err_t servo_init(const servo_config_t *config) {

	// Check against null pointer
	CHECK_ARG(config);

    // Configure LEDC Timer
    ledc_timer_config_t timer_conf = {
        .speed_mode      = config->speed_mode,
        .duty_resolution = SERVO_PWM_RESOLUTION,
        .timer_num       = config->timer,
        .freq_hz         = SERVO_PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(err));
        return err;
    }

    // Configure LEDC Channel
    ledc_channel_config_t channel_conf = {
        .gpio_num   = config->servo_pin,
        .speed_mode = config->speed_mode,
        .channel    = config->channel,
        //.intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = config->timer,
        .duty       = us_to_duty(1500), // Default center position (90 degrees)
        .hpoint     = 0
    };
    err = ledc_channel_config(&channel_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(err));
        return err;
    }

    // Initialize the servo at center position
    return servo_set_angle(config, 90);
}

esp_err_t servo_set_angle(const servo_config_t *config, uint32_t angle_degrees) {
	
	// Check against null pointer
	CHECK_ARG(config);
		
    if (angle_degrees > 180) {
        ESP_LOGE(TAG, "Invalid angle: %lu (must be 0-180)", (unsigned long)angle_degrees);
        return ESP_ERR_INVALID_ARG;
    }
	
	/*TODO: CONFIGURE THE FUNCTIONS TO CORRESPOND TO CHECK(x) WHEN I REFACTOR CODE*/
    // Calculate pulse width between 0-180 degrees
    uint32_t pulse_us = SERVO_MIN_PULSE_WIDTH_US + 
                        ((SERVO_MAX_PULSE_WIDTH_US - SERVO_MIN_PULSE_WIDTH_US) * angle_degrees) / 180;

    uint32_t duty = us_to_duty(pulse_us);

    esp_err_t err = ledc_set_duty(config->speed_mode, config->channel, duty);
    if (err != ESP_OK) return err;

    err = ledc_update_duty(config->speed_mode, config->channel);
    return err;
}