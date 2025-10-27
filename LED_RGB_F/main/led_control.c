#include "led_control.h"
#include "globales.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Tarea encargada de controlar el color y brillo del LED RGB según la temperatura y el % del potenciómetro
void tarea_control_led(void *pvParameters) {
    bool programa_activo = false;
    comando_t cmd;
    float temp = 0, porcentaje = 0;
    thresholds_t thresholds = {20.0, 25.0, 40.0}; // Valores por defecto de thresholds

    while (1) {
        // Revisa si hay un nuevo comando
        if (xQueueReceive(cola_comando, &cmd, 0) == pdPASS) {
            if (cmd == CMD_ON) {
                programa_activo = true;
            } else if (cmd == CMD_OFF) {
                programa_activo = false;
            }
        }

        if (programa_activo) {
            // Lógica normal de control del LED
            // Espera y recibe el valor más reciente de temperatura desde la cola
            xQueueReceive(cola_temp, &temp, pdMS_TO_TICKS(10));
            // Espera y recibe el valor más reciente del potenciómetro desde la cola
            xQueueReceive(cola_pot, &porcentaje, pdMS_TO_TICKS(10));
            // Consulta los thresholds actuales (sin vaciar la cola)
            if (xQueuePeek(cola_thresholds, &thresholds, pdMS_TO_TICKS(10)) != pdTRUE) {
                // Si no hay thresholds en la cola, usa los valores por defecto
                thresholds.t_azul = 20.0;
                thresholds.t_verde = 25.0;
                thresholds.t_rojo = 40.0;
            }

            // Calcula el duty cycle para el PWM según el porcentaje del potenciómetro
            // (PWM invertido porque el LED es de ánodo común)
            uint32_t duty_on = 8191 - (porcentaje / 100.0) * 8191;

            // Selecciona el color del LED según la temperatura y los thresholds
            if (temp < thresholds.t_azul) {
                // Azul: solo el canal azul enciende proporcional al potenciómetro
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 8191);      // Rojo OFF
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 8191);      // Verde OFF
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, duty_on);   // Azul PWM
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
            } else if (temp < thresholds.t_verde) {
                // Verde: solo el canal verde enciende proporcional al potenciómetro
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 8191);      // Rojo OFF
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty_on);   // Verde PWM
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, 8191);      // Azul OFF
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
            } else {
                // Rojo: solo el canal rojo enciende proporcional al potenciómetro
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_on);   // Rojo PWM
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 8191);      // Verde OFF
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, 8191);      // Azul OFF
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
            }

            // Pequeño retardo para evitar saturar el CPU 
            vTaskDelay(pdMS_TO_TICKS(50));
        } else {
            // Apaga el LED (duty máximo para ánodo común)
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 8191);
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 8191);
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, 8191);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}