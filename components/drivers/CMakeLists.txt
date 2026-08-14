idf_component_register(
    SRCS 
        "hc_sr04.c"
        "ir_receiver.c"
        "lcd1602.c"
        "line_sensor.c"
        "motor_driver.c"
        "servo_driver.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        esp_driver_gpio      # ESP-IDF GPIO, LEDC (PWM), and RMT peripherals
        esp_timer
        esp_driver_ledc
        esp_driver_rmt
)