#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "globales.h"

#define GPIO_BOOT 0 // Pin del botón BOOT 

void tarea_boton_boot(void *pvParameters) {
    // Configura el pin BOOT como entrada con pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_BOOT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    int estado_anterior = 1;
    comando_t ultimo_cmd = CMD_OFF;

    while (1) {
        int estado_actual = gpio_get_level(GPIO_BOOT);
        // Detecta flanco de bajada (presión de botón)
        if (estado_anterior == 1 && estado_actual == 0) {
            // Alterna el comando
            ultimo_cmd = (ultimo_cmd == CMD_OFF) ? CMD_ON : CMD_OFF;
            xQueueSend(cola_comando, &ultimo_cmd, 0);

            // Mensaje de sistema encendido o apagado
            if (ultimo_cmd == CMD_ON) {
                printf("Sistema encendido\n");
            } else {
                printf("Sistema apagado\n");
            }
        }
        estado_anterior = estado_actual;
        vTaskDelay(pdMS_TO_TICKS(50)); // Antirebote y bajo consumo
    }
}