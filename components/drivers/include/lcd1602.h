#ifndef LCD1602_H
#define LCD1602_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

/*Configuration template structure for the LCD GPIO pins(4 bit mode). */
typedef struct {
    gpio_num_t rs_pin;   // Register Select
    gpio_num_t en_pin;   // Enable pin
    gpio_num_t d4_pin;   // Data bit 4
    gpio_num_t d5_pin;   // Data bit 5
    gpio_num_t d6_pin;   // Data bit 6
    gpio_num_t d7_pin;   // Data bit 7
} lcd1602_config_t;

/**
 * @brief Initializes the 1602 LCD in 4-bit mode.
 * @param config Pointer to LCD pin configuration structure.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t lcd1602_init(const lcd1602_config_t *config);

/**
 * @brief Clears the display screen and resets cursor position.
 * @param config Pointer to LCD configuration structure.
 */
void lcd1602_clear(const lcd1602_config_t *config);

/**
 * @brief Sets the cursor position on the 16x2 screen.						
 * @param config Pointer to LCD configuration structure.
 * @param col Column position (0 to 15).
 * @param row Row position (0 or 1).
 */
void lcd1602_set_cursor(const lcd1602_config_t *config, uint8_t col, uint8_t row);

/**
 * @brief Sends a null-terminated string to be printed on the LCD.	
 * @param config Pointer to LCD configuration structure.
 * @param str String array to display.
 */
void lcd1602_write_string(const lcd1602_config_t *config, const char *str);

/**
 * @brief Sends a single character to the LCD. 
 * @param config Pointer to LCD configuration structure.
 * @param ch Character byte.
 */
void lcd1602_write_char(const lcd1602_config_t *config, char ch);


#endif