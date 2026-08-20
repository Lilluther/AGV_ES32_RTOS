#ifndef APP_STATE_H
#define APP_STATE_H

//#include <stdint.h>
#include <stdbool.h>

/**
 * @brief AGV operational modes.
 */
typedef enum {
    AGV_MODE_OFF = 0,
    AGV_MODE_MANUAL,
    AGV_MODE_AUTO
} agv_mode_t;

/**
 * @brief Status structure passed to the display task.
 */
typedef struct {
    agv_mode_t current_mode;
    float front_distance_cm;
    bool line_detected;
    char status_msg[32];
} display_status_t;

#endif