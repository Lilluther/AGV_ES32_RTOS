#include "display_task.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "DISPLAY_TASK";
static TaskHandle_t display_task_handle = NULL;
static display_task_config_t task_cfg;
static bool is_running = false;

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0) 

/* Helper to display mode enum as string */
static const char* get_mode_str(agv_mode_t mode) {
    switch (mode) {
        case AGV_MODE_MANUAL: return "MAN";
        case AGV_MODE_AUTO:   return "AUT";
        case AGV_MODE_OFF:
        default:              return "OFF";
    }
}

/* Renders current state onto 16x2 character LCD */
static void render_screen(const display_status_t *status) {
    char line1_buf[17];
    char line2_buf[17];

    lcd1602_clear(&task_cfg.lcd_cfg);

    /* Line 1 (16 chars): "M:MAN  DIST:20cm" */
    snprintf(line1_buf, sizeof(line1_buf), "M:%-3s DIST:%2.0fcm", 
             get_mode_str(status->current_mode), 
             status->front_distance_cm);

    /* Line 2 (16 chars): line tracking state */
    if (strlen(status->status_msg) > 0) {
        snprintf(line2_buf, sizeof(line2_buf), "%-16.16s", status->status_msg);
    } else {
        snprintf(line2_buf, sizeof(line2_buf), "LINE: %-10s", 
                 status->line_detected ? "FOUND" : "LOST");
    }

    /* Output to physical 1602 LCD */
    lcd1602_set_cursor(&task_cfg.lcd_cfg, 0, 0);
    lcd1602_write_string(&task_cfg.lcd_cfg, line1_buf);

    lcd1602_set_cursor(&task_cfg.lcd_cfg, 0, 1);
    lcd1602_write_string(&task_cfg.lcd_cfg, line2_buf);
}

/* Display task */
static void display_task_loop(void *pvParameters) {
    display_status_t current_status;

    memset(&current_status, 0, sizeof(display_status_t)); //Initialize current_status members as zero
    current_status.current_mode = AGV_MODE_OFF;
    strncpy(current_status.status_msg, "System Ready", sizeof(current_status.status_msg) - 1);

    ESP_LOGI(TAG, "Display Task Running LCD1602...");

    /* Draw initial boot screen */
    render_screen(&current_status);

    while (is_running) {
        /* Wait for new display updates from the queue */
        if (xQueueReceive(task_cfg.display_queue, &current_status, pdMS_TO_TICKS(500)) == pdTRUE) {
            render_screen(&current_status);
        }
    }

    vTaskDelete(NULL);
}

esp_err_t display_task_start(const display_task_config_t *config) {
	
    BaseType_t ret;

	CHECK_ARG( config && config->display_queue );

    if (is_running) {
        return ESP_OK; //Don't create task if it already exists
    }

    task_cfg = *config;
    is_running = true;

    ret = xTaskCreate(
        display_task_loop,
        "display_task",
        3072,
        NULL,
        2, /* Priority 2 */
        &display_task_handle
    );
	
	return (ret == pdPASS) ? ESP_OK : ESP_FAIL; //check that task was successfully created
}

void display_task_stop(void) {
    is_running = false;
    if (display_task_handle != NULL) {
        display_task_handle = NULL;
    }
}

void display_task_update_status(QueueHandle_t queue, const display_status_t *status) {
	
    if (queue != NULL && status != NULL) {
        xQueueSend(queue, status, 0);
    }
}