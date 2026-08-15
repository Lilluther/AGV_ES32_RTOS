#include "motor_driver.h"
#include "esp_log.h"

static const char *TAG = "MOTOR_DRIVER";

#define MOTOR_PWM_FREQ_HZ     5000               // 5 kHz PWM frequency
#define MOTOR_PWM_RESOLUTION  LEDC_TIMER_8_BIT   // 8-bit resolution

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0) 
#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)

/**
 * Helper function to map 0-100% duty cycle to 8-bit LEDC duty value (0-255).
 */
static uint32_t percent_to_duty(uint8_t speed_percent) {
    if (speed_percent > 100) speed_percent = 100; // Set max speed_percent as 100%
    return (uint32_t)((speed_percent * 255) / 100);
}

esp_err_t motor_driver_init(const motor_config_t *config) {

	// Check against null pointer
	CHECK_ARG(config);

    // 1. Configure Direction Pins (IN1, IN2, IN3, IN4) as GPIO Outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->in1_pin) |
                        (1ULL << config->in2_pin) |
                        (1ULL << config->in3_pin) |
                        (1ULL << config->in4_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
	
	
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure direction GPIOs");
        return err;
    }

    // Set initial direction pins to LOW (Stopped)
    gpio_set_level(config->in1_pin, 0);
    gpio_set_level(config->in2_pin, 0);
    gpio_set_level(config->in3_pin, 0);
    gpio_set_level(config->in4_pin, 0);

    // 2. Configure Shared LEDC PWM Timer
    ledc_timer_config_t timer_conf = {
        .speed_mode      = config->speed_mode,
        .duty_resolution = MOTOR_PWM_RESOLUTION,
        .timer_num       = config->timer,
        .freq_hz         = MOTOR_PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK
    };
   
	CHECK(ledc_timer_config(&timer_conf));
	

    // 3. Configure Left Motor PWM Channel (ENA)
    ledc_channel_config_t left_chan = {
        .gpio_num   = config->ena_pin,
        .speed_mode = config->speed_mode,
        .channel    = config->left_channel,
        .timer_sel  = config->timer,
        .duty       = 0,
        .hpoint     = 0
    };
  
	CHECK(ledc_channel_config(&left_chan));

    // 4. Configure Right Motor PWM Channel (ENB)
    ledc_channel_config_t right_chan = {
        .gpio_num   = config->enb_pin,
        .speed_mode = config->speed_mode,
        .channel    = config->right_channel,
        .timer_sel  = config->timer,
        .duty       = 0,
        .hpoint     = 0
    };

	CHECK(ledc_channel_config(&right_chan));

    ESP_LOGI(TAG, "L298N Dual Motor Driver initialized successfully");
    return ESP_OK;
}

esp_err_t motor_drive(const motor_config_t *config, uint8_t left_speed, uint8_t right_speed, motor_action_t action) {

	// Check against null pointer
	CHECK_ARG(config);

    // Set Direction logic
    switch (action) {
        case MOTOR_FORWARD:
            gpio_set_level(config->in1_pin, 1); gpio_set_level(config->in2_pin, 0);
            gpio_set_level(config->in3_pin, 1); gpio_set_level(config->in4_pin, 0);
            break;

        case MOTOR_BACKWARD:
            gpio_set_level(config->in1_pin, 0); gpio_set_level(config->in2_pin, 1);
            gpio_set_level(config->in3_pin, 0); gpio_set_level(config->in4_pin, 1);
            break;

        case MOTOR_TURN_LEFT: // Sharp Pivot Left
            gpio_set_level(config->in1_pin, 0); gpio_set_level(config->in2_pin, 1);
            gpio_set_level(config->in3_pin, 1); gpio_set_level(config->in4_pin, 0);
            break;

        case MOTOR_TURN_RIGHT: // Sharp Pivot Right
            gpio_set_level(config->in1_pin, 1); gpio_set_level(config->in2_pin, 0);
            gpio_set_level(config->in3_pin, 0); gpio_set_level(config->in4_pin, 1);
            break;

        case MOTOR_SOFT_LEFT: // Curve Left
            gpio_set_level(config->in1_pin, 1); gpio_set_level(config->in2_pin, 0);
            gpio_set_level(config->in3_pin, 1); gpio_set_level(config->in4_pin, 0);
            left_speed = left_speed / 2; // Reduce left motor speed
            break;

        case MOTOR_SOFT_RIGHT: // Curve Right
            gpio_set_level(config->in1_pin, 1); gpio_set_level(config->in2_pin, 0);
            gpio_set_level(config->in3_pin, 1); gpio_set_level(config->in4_pin, 0);
            right_speed = right_speed / 2; // Reduce right motor speed
            break;

        case MOTOR_STOP:
        default:
            return motor_stop(config);
    }

    // Apply Duty Cycle  via LEDC PWM
    ledc_set_duty(config->speed_mode, config->left_channel, percent_to_duty(left_speed));
    ledc_update_duty(config->speed_mode, config->left_channel);

    ledc_set_duty(config->speed_mode, config->right_channel, percent_to_duty(right_speed));
    ledc_update_duty(config->speed_mode, config->right_channel);

    return ESP_OK;
}

esp_err_t motor_stop(const motor_config_t *config) {

	// Check against null pointer
	CHECK_ARG(config);

    // Pull direction pins LOW
    gpio_set_level(config->in1_pin, 0);
    gpio_set_level(config->in2_pin, 0);
    gpio_set_level(config->in3_pin, 0);
    gpio_set_level(config->in4_pin, 0);

    // Set PWM Duty Cycle to 0
    ledc_set_duty(config->speed_mode, config->left_channel, 0);
    ledc_update_duty(config->speed_mode, config->left_channel);

    ledc_set_duty(config->speed_mode, config->right_channel, 0);
    ledc_update_duty(config->speed_mode, config->right_channel);

    return ESP_OK;
}