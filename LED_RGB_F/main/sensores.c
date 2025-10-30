#include "sensores.h"
#include "globales.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <stdio.h>

// Callback del timer de temperatura: libera el semáforo para que la tarea lea el sensor
void timer_temp_callback(TimerHandle_t xTimer) {
    xSemaphoreGive(semaforo_temp);
}

// Callback del timer de potenciómetro: libera el semáforo para que la tarea lea el sensor
void timer_pot_callback(TimerHandle_t xTimer) {
    xSemaphoreGive(semaforo_pot);
}

// Tarea que lee la temperatura del termistor cada vez que el semáforo es liberado
void tarea_temperatura(void *pvParameters) {
    extern int intervalo_print_temp;
    int print_counter = 0;
    while (1) {
        // Espera a que el semáforo sea liberado por el timer (cada 100 ms)
        if (xSemaphoreTake(semaforo_temp, portMAX_DELAY) == pdTRUE) {
            int adc_raw = adc1_get_raw(ADC_CHANNEL_TERMISTOR);      // Lee el valor ADC del termistor
            float Vadc = (adc_raw / 4095.0f) * 3.3f;                // Convierte a voltaje
            float R_serie = 10000.0f;                               // Resistencia en serie (ohmios)
            float R_termistor = (Vadc * R_serie) / (3.3f - Vadc);   // Calcula la resistencia del termistor
            float temp = 25.0f + (R_termistor - 10000.0f) * (50.0f - 25.0f) / (15000.0f - 10000.0f); // Estima la temperatura

            // Imprime la temperatura por UART cada intervalo_print_temp segundos
            if (++print_counter >= (intervalo_print_temp * 10)) {
                printf("Temperatura de termistor: %.2f C\n", temp);
                print_counter = 0;
            }

            xQueueSend(cola_temp, &temp, portMAX_DELAY); // Envía la temperatura a la cola para otras tareas
        }
    }
}

// Tarea que lee el valor del potenciómetro cada vez que el semáforo es liberado
void tarea_potenciometro(void *pvParameters) {
    int print_counter = 0;
    while (1) {
        // Espera a que el semáforo sea liberado por el timer (cada 30 ms)
        if (xSemaphoreTake(semaforo_pot, portMAX_DELAY) == pdTRUE) {
            int adc_raw = adc1_get_raw(ADC_CHANNEL_POT);            // Lee el valor ADC del potenciómetro
            float voltaje = (adc_raw / 4095.0f) * 100.0f;        // Convierte a porcentaje (0-100%)

            // Imprime el porcentaje por UART cada 67 lecturas (~2 segundos)
            if (++print_counter >= 67) {
                printf("Porcentaje del potenciometro: %.2f%%\n", voltaje);
                print_counter = 0;
            }

            xQueueSend(cola_pot, &voltaje, portMAX_DELAY); // Envía el voltaje a la cola para otras tareas
        }
    }
}