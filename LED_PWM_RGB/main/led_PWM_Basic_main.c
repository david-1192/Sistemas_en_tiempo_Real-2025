#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"

// Pines y canales para cada color
#define GPIO_RED    2
#define GPIO_GREEN  4
#define GPIO_BLUE   5
#define CHANNEL_RED     LEDC_CHANNEL_0
#define CHANNEL_GREEN   LEDC_CHANNEL_1
#define CHANNEL_BLUE    LEDC_CHANNEL_2
#define TIMER           LEDC_TIMER_0
#define DUTY_RES        LEDC_TIMER_13_BIT
#define FREQ            4000

void init_rgb_pwm()
{
    // Configura el temporizador PWM
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = DUTY_RES,
        .timer_num = TIMER,
        .freq_hz = FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    // Configura el canal PWM para el color rojo
    ledc_channel_config_t red = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = CHANNEL_RED,
        .timer_sel = TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_RED,
        .duty = 0,
        .hpoint = 0
    };
    // Configura el canal PWM para el color verde y azul 
    // (copiando la configuración de rojo y cambiando canal y pin)
    ledc_channel_config_t green = red;
    ledc_channel_config_t blue = red;
    green.channel = CHANNEL_GREEN; green.gpio_num = GPIO_GREEN;
    blue.channel = CHANNEL_BLUE; blue.gpio_num = GPIO_BLUE;

    ESP_ERROR_CHECK(ledc_channel_config(&red));
    ESP_ERROR_CHECK(ledc_channel_config(&green));
    ESP_ERROR_CHECK(ledc_channel_config(&blue));
}

void app_main(void)
{
    // Inicializa el temporizador y los canales PWM para el LED RGB
    init_rgb_pwm();

    // Enciende el LED en blanco al 100%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNEL_RED, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNEL_RED));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNEL_GREEN, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNEL_GREEN));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNEL_BLUE, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNEL_BLUE));
}
