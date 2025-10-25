#include <stdint.h>
#include "driver/ledc.h"

// Estructura para un solo LED
typedef struct {
    int gpio_num;
    ledc_channel_t channel;
    uint32_t duty; // Cambiado a uint32_t
} LED_t;

// Estructura para LED RGB
typedef struct {
    LED_t red;
    LED_t green;
    LED_t blue;
    ledc_timer_t timer_sel;
} LED_RGB_t;

// Prototipos de funciones para inicializar y controlar LEDs individuales
LED_t init_LED(int gpio_num, ledc_channel_t channel, ledc_timer_t timer_sel, uint32_t frequency);
void set_led_red(LED_t led, uint32_t duty);
void set_led_green(LED_t led, uint32_t duty);
void set_led_blue(LED_t led, uint32_t duty);

// Prototipo para inicializar un LED RGB completo
// Recibe los pines, canales, temporizador y frecuencia para cada color
LED_RGB_t init_LED_RGB(
    int gpio_num_red, int gpio_num_green, int gpio_num_blue,
    ledc_timer_t timer_sel, uint32_t frequency,
    ledc_channel_t channel_red, ledc_channel_t channel_green, ledc_channel_t channel_blue
);

// Prototipo para establecer el color del LED RGB
// Permite controlar el brillo de cada color por separado
void set_LED_RGB_color(LED_RGB_t led_rgb, uint32_t duty_red, uint32_t duty_green, uint32_t duty_blue);


