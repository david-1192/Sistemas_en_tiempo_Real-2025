#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_BLUE GPIO_NUM_2 // LED Azul en la ESP32
#define BUTTON_BOOT GPIO_NUM_0  // Botón BOOT en la ESP32


uint8_t led_state = 0;
uint8_t blink_enabled = 1;  // Controla si el blink está activo
uint8_t last_button_state = 1; // Estado anterior del botón (pull-up)

esp_err_t init_led(void);
esp_err_t init_button(void);
esp_err_t blink_led(void);
void check_button(void);


void app_main(void)
{
    init_led();
    init_button();
    
    while(1) {
        check_button();  // Verificar estado del botón

        // Solo parpadear si blink_enabled está habilitado
        if (blink_enabled) {
            blink_led();
            printf("LED state: %u\n", led_state); 
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Retardo de 100 ms
    }
}

/**
 * @brief Inicializa el pin del LED como salida
 * @return esp_err_t Código de error
 */
esp_err_t init_led(void) {
    gpio_reset_pin(LED_BLUE);
    gpio_set_direction(LED_BLUE, GPIO_MODE_OUTPUT); // Configurar LED como salida
    return ESP_OK;
}

 /**
  * @brief Inicializa el pin del botón como entrada con pull-up
  * @return esp_err_t Código de error
 */
esp_err_t init_button(void) {
    gpio_reset_pin(BUTTON_BOOT);
    gpio_set_direction(BUTTON_BOOT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_BOOT, GPIO_PULLUP_ONLY); // Pull-up interno
    return ESP_OK;
}

/**
 * @brief Cambia el estado del LED
 * @return esp_err_t Código de error
 */
esp_err_t blink_led(void) {
    led_state = !led_state; // Alternar estado
    gpio_set_level(LED_BLUE, led_state); // Actualizar el estado del LED
    return ESP_OK;
}

/**
 * @brief Verifica el estado del botón y alterna el parpadeo del LED
 */
void check_button(void) {
    uint8_t current_button_state = gpio_get_level(BUTTON_BOOT);
    
    
    if (last_button_state == 1 && current_button_state == 0) {
        
        blink_enabled = !blink_enabled; // Invertir el estado del blink
        
        if (blink_enabled) {
            printf("Blink ACTIVADO\n");
        } else {
            printf("Blink DESACTIVADO\n");
            gpio_set_level(LED_BLUE, 0); // Apagar LED cuando se desactiva
        }
        
        // Pequeño delay para evitar rebotes
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    last_button_state = current_button_state; // Actualizar el estado del botón
}