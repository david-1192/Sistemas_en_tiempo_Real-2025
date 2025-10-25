idf_component_register(SRCS "led_PWM_Basic_main.c" "led.library.c"
                    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio
                    INCLUDE_DIRS ".")
