#include "auto_nav_task.h"
#include "esp_log.h"
#include "freertos/queue.h"

static const char *TAG = "AUTO_NAV_TASK";
static TaskHandle_t auto_nav_task_handle = NULL;
static auto_nav_config_t nav_cfg;
static bool is_running = false;

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0) 

/* Helper function to scan left and right using the servo and return clear direction */
static int scan_clear_path(void) {
    float left_dist = 0.0f;
    float right_dist = 0.0f;

    // Stop vehicle while scanning
    motor_stop(&nav_cfg.motor_cfg);

    // Look Left (150 degrees)
    servo_set_angle(&nav_cfg.servo_cfg, 150);
    vTaskDelay(pdMS_TO_TICKS(400));
    hc_sr04_read_cm(&nav_cfg.hc_sr04_cfg, &left_dist);

    // Look Right (30 degrees)
    servo_set_angle(&nav_cfg.servo_cfg, 30);
    vTaskDelay(pdMS_TO_TICKS(400));
    hc_sr04_read_cm(&nav_cfg.hc_sr04_cfg, &right_dist);

    // Return servo to center (90 degrees)
    servo_set_angle(&nav_cfg.servo_cfg, 90);
    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "Scan results - Left: %.1f cm, Right: %.1f cm", left_dist, right_dist);

    // Return +1 for Left evasion, -1 for Right evasion
    return (left_dist >= right_dist) ? 1 : -1;
}

/* Maneuver vehicle around obstacle until line is detected again */
static void execute_obstacle_evasion(int evade_dir) {
    line_sensor_state_t line_state;
    
    ESP_LOGI(TAG, "Bypassing obstacle towards %s...", (evade_dir > 0) ? "LEFT" : "RIGHT");

    //Turn away from obstacle
    if (evade_dir > 0) {
        motor_drive(&nav_cfg.motor_cfg, 60, 60, MOTOR_TURN_LEFT);
    } else {
        motor_drive(&nav_cfg.motor_cfg, 60, 60, MOTOR_TURN_RIGHT);
    }
    vTaskDelay(pdMS_TO_TICKS(400));

    //Drive arc forward around the obstacle
    motor_drive(&nav_cfg.motor_cfg, 60, 60, MOTOR_FORWARD);
    vTaskDelay(pdMS_TO_TICKS(600));

    //Turn back towards track direction
    if (evade_dir > 0) {
        motor_drive(&nav_cfg.motor_cfg, 60, 60, MOTOR_TURN_RIGHT);
    } else {
        motor_drive(&nav_cfg.motor_cfg, 60, 60, MOTOR_TURN_LEFT);
    }
    vTaskDelay(pdMS_TO_TICKS(400));

    //Drive forward while sweeping until line sensors catch dark line
    motor_drive(&nav_cfg.motor_cfg, 50, 50, MOTOR_FORWARD);
    while (is_running) {
        line_sensor_read(&nav_cfg.line_cfg, &line_state);
        
        // If any sensor hits the dark line, break loop to resume standard tracking
        if (line_state.left || line_state.center || line_state.right) {
            ESP_LOGI(TAG, "Line recovered after obstacle bypass.");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/*Main Autonomous Driving State Machine */
static void auto_nav_task_loop(void *pvParameters) {
    line_sensor_state_t line_state;
    float front_distance = 100.0f;

    ESP_LOGI(TAG, "Auto Navigation Task Running...");

    while (is_running) {
        // Check ultrasonic distance ahead
        if (hc_sr04_read_cm(&nav_cfg.hc_sr04_cfg, &front_distance) == ESP_OK) {
            if (front_distance > 0.0f && front_distance <= nav_cfg.obstacle_threshold_cm) {
                ESP_LOGW(TAG, "Obstacle detected at %.1f cm!", front_distance);
                int clear_direction = scan_clear_path();
                execute_obstacle_evasion(clear_direction);
                continue;
            }
        }

        //Line following state execution
        line_sensor_read(&nav_cfg.line_cfg, &line_state);
        line_position_t pos = line_sensor_evaluate_position(&line_state);

        switch (pos) {
            case LINE_POS_CENTERED:
                motor_drive(&nav_cfg.motor_cfg, 60, 60, MOTOR_FORWARD);
                break;

            case LINE_POS_LEFT:
                motor_drive(&nav_cfg.motor_cfg, 40, 65, MOTOR_SOFT_LEFT);
                break;

            case LINE_POS_RIGHT:
                motor_drive(&nav_cfg.motor_cfg, 65, 40, MOTOR_SOFT_RIGHT);
                break;

            case LINE_POS_ALL_DARK:
                motor_drive(&nav_cfg.motor_cfg, 50, 50, MOTOR_FORWARD);
                break;

            case LINE_POS_LOST:
            default:
                // Slow crawl search when off-track
                motor_drive(&nav_cfg.motor_cfg, 40, 40, MOTOR_FORWARD);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // Task tick delay (50 Hz control loop)
    }

    motor_stop(&nav_cfg.motor_cfg);
    vTaskDelete(NULL);
}

esp_err_t auto_nav_task_start(const auto_nav_config_t *config) {
    
	// Check against null pointer
	CHECK_ARG(config);
    
    if (is_running) {
        return ESP_OK; // Task already active
    }

    nav_cfg = *config;
    is_running = true;

    BaseType_t ret = xTaskCreate(
        auto_nav_task_loop,
        "auto_nav_task",
        4096,
        NULL,
        5, // High priority task
        &auto_nav_task_handle
    );

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

void auto_nav_task_stop(void) {
    is_running = false;
    if (auto_nav_task_handle != NULL) {
        motor_stop(&nav_cfg.motor_cfg);
        auto_nav_task_handle = NULL;
    }
}