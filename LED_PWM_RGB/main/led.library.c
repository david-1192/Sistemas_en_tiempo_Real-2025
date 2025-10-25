#include "led_library.h"
#include "driver/ledc.h"
#include <stdio.h>

/**
 * Configura un LED RGB con los parámetros especificados
 * Inicializa la estructura LED_RGB_t y configura el temporizador y los canales PWM.
 */
LED_RGB_t configure_LED_RGB(int gpio_num_red, int gpio_num_green, int gpio_num_blue, ledc_channel_t channel_red, ledc_channel_t channel_green, ledc_channel_t channel_blue, ledc_timer_t timer_sel, ledc_timer_bit_t duty_resolution, uint32_t frequency)
{
    /**
     * Inicializa la estructura LED_RGB_t con los valores proporcionados
     */
    LED_RGB_t led_rgb;
    led_rgb.red.gpio_num = gpio_num_red;
    led_rgb.red.channel = channel_red;
    led_rgb.red.duty = 0;

    led_rgb.green.gpio_num = gpio_num_green;
    led_rgb.green.channel = channel_green;
    led_rgb.green.duty = 0;

    led_rgb.blue.gpio_num = gpio_num_blue;
    led_rgb.blue.channel = channel_blue;
    led_rgb.blue.duty = 0;

    led_rgb.timer_sel = timer_sel;

    /**
     * Configura el temporizador LEDC para el LED RGB
     */
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = duty_resolution,
        .timer_num        = led_rgb.timer_sel,
        .freq_hz          = frequency,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    
    // Configura los canales PWM para los LEDs rojo, verde y azul
    ledc_channel_config_t ledc_channel= {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = led_rgb.red.channel,
        .timer_sel      = led_rgb.timer_sel,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = led_rgb.red.gpio_num,
        .duty           = 0,
        .hpoint         = 0
    };
    /**
     * Configura los canales LEDC para los LEDs rojo, verde y azul
     */
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    // Configura el canal para el LED verde
    ledc_channel.channel = led_rgb.green.channel;
    ledc_channel.gpio_num = led_rgb.green.gpio_num;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    // Configura el canal para el LED azul
    ledc_channel.channel = led_rgb.blue.channel;
    ledc_channel.gpio_num = led_rgb.blue.gpio_num;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel)); 

    return led_rgb;
}

void set_LED_RGB_color(LED_RGB_t led_rgb, uint32_t duty_red, uint32_t duty_green, uint32_t duty_blue)
{
    /**
     * Establece el ciclo de trabajo para cada canal del LED RGB
     */
    led_rgb.red.duty = duty_red;
    led_rgb.green.duty = duty_green;
    led_rgb.blue.duty = duty_blue;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, led_rgb.red.channel, led_rgb.red.duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, led_rgb.red.channel));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, led_rgb.green.channel, led_rgb.green.duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, led_rgb.green.channel));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, led_rgb.blue.channel, led_rgb.blue.duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, led_rgb.blue.channel));
}

void set_led_red(LED_t led, uint32_t duty)
{
    /**
     * Establece el ciclo de trabajo para el canal rojo del LED RGB
     */
    led.duty = duty;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, led.channel, led.duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, led.channel));
}

void set_led_green(LED_t led, uint32_t duty)
{
    /**
     * Establece el ciclo de trabajo para el canal verde del LED RGB
     */
    led.duty = duty;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, led.channel, led.duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, led.channel));
}

void set_led_blue(LED_t led, uint32_t duty)
{
    /**
     * Establece el ciclo de trabajo para el canal azul del LED RGB
     */
    led.duty = duty;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, led.channel, led.duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, led.channel));
}