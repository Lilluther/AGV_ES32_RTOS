#include "lcd1602.h"  
#include "rom/ets_sys.h"
#include "esp_log.h"

static const char *TAG = "LCD1602";
 
/*HD44780 LCD Commands. */
#define LCD_CMD_CLEAR           	0x01
#define LCD_CMD_RETURN_HOME     	0x02
#define LCD_CMD_ENTRY_MODE_SET 	 	0x06
#define LCD_CMD_DISPLAY_ON      	0x0C
#define LCD_CMD_FUNCTION_SET_4BIT 	0x28
#define LCD_CMD_SET_DDRAM_ADDR  	0x80

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0) 
#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)

static void lcd1602_pulse_enable(const lcd1602_config_t *config) {
	
    gpio_set_level(config->en_pin, 1);
    ets_delay_us(2); // >450ns pulse duration required
    gpio_set_level(config->en_pin, 0);
    ets_delay_us(50); // Settling time as recommended in the datasheet
	
}

static void lcd1602_write_nibble(const lcd1602_config_t *config, uint8_t nibble) {
	
    gpio_set_level(config->d4_pin, (nibble >> 0) & 0x01);
    gpio_set_level(config->d5_pin, (nibble >> 1) & 0x01);
    gpio_set_level(config->d6_pin, (nibble >> 2) & 0x01);
    gpio_set_level(config->d7_pin, (nibble >> 3) & 0x01);

    lcd1602_pulse_enable(config);	
}

static void lcd1602_send_byte(const lcd1602_config_t *config, uint8_t byte, uint8_t mode) {
    gpio_set_level(config->rs_pin, mode); // 0 = Command, 1 = Data

    // Send High Nibble first, then Low Nibble
    lcd1602_write_nibble(config, (byte >> 4) & 0x0F);
    lcd1602_write_nibble(config, byte & 0x0F);
}

esp_err_t lcd1602_init(const lcd1602_config_t *config) {

	// Check against null pointer
	CHECK_ARG(config);

    // Configure all 6 LCD control lines as outputs
    uint64_t pin_mask = (1ULL << config->rs_pin) |
                        (1ULL << config->en_pin) |
                        (1ULL << config->d4_pin) |
                        (1ULL << config->d5_pin) |
                        (1ULL << config->d6_pin) |
                        (1ULL << config->d7_pin);

    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
	CHECK(gpio_config(&io_conf));

    // Power-on LCD Initialization sequence (HD44780 Specs)
    ets_delay_us(50000); // Wait >40ms after power up

    gpio_set_level(config->rs_pin, 0);
    gpio_set_level(config->en_pin, 0);

    // Initial sequence to force 4-bit mode
    lcd1602_write_nibble(config, 0x03);
    ets_delay_us(4500);
    lcd1602_write_nibble(config, 0x03);
    ets_delay_us(4500);
    lcd1602_write_nibble(config, 0x03);
    ets_delay_us(150);
    
    // Switch to 4-bit interface
    lcd1602_write_nibble(config, 0x02);

    // Function Set: 4-bit, 2 lines, 5x8 font
    lcd1602_send_byte(config, LCD_CMD_FUNCTION_SET_4BIT, 0);
    // Display ON, Cursor OFF, Blink OFF
    lcd1602_send_byte(config, LCD_CMD_DISPLAY_ON, 0);
    // Entry Mode Set: Increment cursor, no shift
    lcd1602_send_byte(config, LCD_CMD_ENTRY_MODE_SET, 0);
    
    lcd1602_clear(config);

    ESP_LOGI(TAG, "LCD1602 initialized in 4-bit mode");
    return ESP_OK;
}

void lcd1602_clear(const lcd1602_config_t *config) {
    if (!config) return;
    lcd1602_send_byte(config, LCD_CMD_CLEAR, 0);
    ets_delay_us(2000); // Clear command requires approx 1.52ms execution time
}

void lcd1602_set_cursor(const lcd1602_config_t *config, uint8_t col, uint8_t row) {
    if (!config) return;
    static const uint8_t row_offsets[] = {0x00, 0x40};
    if (row > 1) row = 1;
    if (col > 15) col = 15;

    lcd1602_send_byte(config, LCD_CMD_SET_DDRAM_ADDR | (col + row_offsets[row]), 0);
}

void lcd1602_write_char(const lcd1602_config_t *config, char ch) {
    if (!config) return;
    lcd1602_send_byte(config, (uint8_t)ch, 1);
}

void lcd1602_write_string(const lcd1602_config_t *config, const char *str) {
    if (!config || !str) return;
    while (*str) {
        lcd1602_write_char(config, *str++);
    }
}