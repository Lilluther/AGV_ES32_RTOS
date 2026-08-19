#include "remote_ir_task.h"
#include "esp_log.h"
#include "bt_mgr.h"

static const char *TAG = "REMOTE_IR_TASK";
static TaskHandle_t ir_task_handle = NULL;
static remote_ir_config_t task_cfg;
static bool is_running = false;
static bool manual_mode_active = false;

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0) 

/* Helper to convert decoded IR receiver command state into system actions */
static void process_ir_cmd(ir_cmd_state_t cmd) {
    bt_msg_t bt_msg;

    switch (cmd) {
        case IR_CMD_RESET:
            ESP_LOGI(TAG, "IR Remote requested RESET, so changing to BT command mode");
            motor_stop(&task_cfg.motor_cfg);
            bt_msg.cmd = BT_CMD_SET_OFF;
            strncpy(bt_msg.raw_msg, "IR_RESET", sizeof(bt_msg.raw_msg));
            if (task_cfg.bt_cmd_queue) {
                xQueueSend(task_cfg.bt_cmd_queue, &bt_msg, 0);
            }
            break;

        case IR_CMD_STOP:
            ESP_LOGI(TAG, "IR Remote requested STOP");
            motor_stop(&task_cfg.motor_cfg);
            break;

        /* Directional control, active only when MANUAL mode is enabled) */
        case IR_CMD_FORWARD:
            if (manual_mode_active) {
                ESP_LOGI(TAG, "Manual: Drive FORWARD");
                motor_drive(&task_cfg.motor_cfg, 70, 70, MOTOR_FORWARD);
            }
            break;

        case IR_CMD_BACKWARD:
            if (manual_mode_active) {
                ESP_LOGI(TAG, "Manual: Drive BACKWARD");
                motor_drive(&task_cfg.motor_cfg, 70, 70, MOTOR_BACKWARD);
            }
            break;

        case IR_CMD_LEFT:
            if (manual_mode_active) {
                ESP_LOGI(TAG, "Manual: Turn LEFT");
                motor_drive(&task_cfg.motor_cfg, 60, 60, MOTOR_TURN_LEFT);
            }
            break;

        case IR_CMD_RIGHT:
            if (manual_mode_active) {
                ESP_LOGI(TAG, "Manual: Turn RIGHT");
                motor_drive(&task_cfg.motor_cfg, 60, 60, MOTOR_TURN_RIGHT);
            }
            break;

        case IR_CMD_NONE:
        case IR_CMD_UNKNOWN:
        default:
            break;
    }
}

/*Task to implement reading of commands, timeout the read function if no commands are received and implement process_ir state machine */
static void remote_ir_task_loop(void *pvParameters) {
	
    ir_cmd_state_t ir_cmd = IR_CMD_NONE; //initialize as having no command

    ESP_LOGI(TAG, "Remote Task Running...");

    while (is_running) {
        /* Read next mapped IR command from receiver driver */
        if (ir_receiver_read_cmd(&task_cfg.ir_cfg, &ir_cmd, 100) == ESP_OK) {
            process_ir_cmd(ir_cmd);
        }
    }

    vTaskDelete(NULL);
}

esp_err_t remote_ir_task_start(const remote_ir_config_t *config) {
    BaseType_t ret;

	// Check against null pointer
	CHECK_ARG(config);

    if (is_running) {
        return ESP_OK; //Exit, don't create task if one already exists
    }

    task_cfg = *config;
    is_running = true;

    ret = xTaskCreate(
        remote_ir_task_loop,
        "remote_ir_task",
        3072,
        NULL,
        4,
        &ir_task_handle
    );

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL; //check that task was successfully created
}

void remote_ir_task_stop(void) {
    is_running = false;
    if (ir_task_handle != NULL) {
        ir_task_handle = NULL; //Null out the task handle so task is inaccessible when task is stopped 
    }
}

void remote_ir_set_manual_mode(bool enabled) {
    manual_mode_active = enabled;
    if (!enabled) {
        motor_stop(&task_cfg.motor_cfg);
    }
}
