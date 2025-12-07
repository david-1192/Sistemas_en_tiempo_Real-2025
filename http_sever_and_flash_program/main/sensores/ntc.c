#include "ntc.h"
#include "globales.h"
#include "pwm_led.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include <stdio.h>

// ============================================================================
// Callback del timer de temperatura
// ============================================================================
void timer_temp_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    xSemaphoreGive(semaforo_temp);
}

// ============================================================================
// Tarea que lee la temperatura del NTC
// ============================================================================
void tarea_temperatura(void *pvParameters)
{
    (void)pvParameters;
    float temp = 0.0f;
    int print_counter = 0;

    printf("Tarea Temperatura (NTC) iniciada\n");

    while (1) {
        if (xSemaphoreTake(semaforo_temp, portMAX_DELAY) == pdTRUE) {
            int adc_raw = adc1_get_raw(ADC_CHANNEL_NTC);
            float Vadc = (adc_raw / 4095.0f) * 3.3f;
            float R_serie = 10000.0f;

            // Calcula la resistencia del termistor
            float R_ntc = (Vadc * R_serie) / (3.3f - Vadc);

            // Fórmula INVERTIDA: Temperatura AUMENTA cuando R_ntc DISMINUYE
            temp = 25.0f - (R_ntc - 10000.0f) * (50.0f - 25.0f) / (15000.0f - 10000.0f);

            // ================================================================
            // Control del LED PWM según temperatura
            // Rango: 20°C (apagado) a 40°C (máximo brillo)
            // ================================================================
            uint32_t duty = 0;
            if (temp > 20.0f && temp < 40.0f) {
                // Mapear temperatura (20-40°C) a duty (0-8191)
                duty = (uint32_t)((temp - 20.0f) / 20.0f * 8191.0f);
            } else if (temp >= 40.0f) {
                duty = 8191;  // 100% brillo
            } else {
                duty = 0;     // Apagado si temp <= 20°C
            }

            // Enviar comando PWM a la cola
            pwm_command_t cmd = {.duty = duty};
            if (xQueueSend(cola_pwm_led, &cmd, pdMS_TO_TICKS(100)) != pdPASS) {
                printf("[NTC] Error: Cola PWM llena\n");
            }

            // Imprimir información cada intervalo_print_temp
            if (++print_counter >= intervalo_print_temp) {
                printf("[NTC] Temperatura: %.2f °C (ADC: %d, V: %.2f V, R: %.0f Ω, PWM: %lu - %.1f%%)\n",
                       temp, adc_raw, Vadc, R_ntc, duty, (duty / 8191.0f) * 100.0f);
                print_counter = 0;
            }

            // Enviar temperatura a la cola para otras tareas
            if (xQueueSend(cola_temperatura, &temp, pdMS_TO_TICKS(100)) != pdPASS) {
                printf("[NTC] Error: Cola de temperatura llena\n");
            }
        }
    }
}

// ============================================================================
// Inicialización del sistema de lectura de temperatura
// ============================================================================
void ntc_init(void)
{
    printf("\n========== INICIALIZANDO SISTEMA NTC ==========\n");

    // Crear la cola de temperatura (capacidad: 10 elementos)
    cola_temperatura = xQueueCreate(10, sizeof(float));
    if (cola_temperatura == NULL) {
        printf("ERROR: No se pudo crear la cola de temperatura\n");
        return;
    }
    printf("✓ Cola de temperatura creada\n");

    // Crear el semáforo binario para sincronización
    semaforo_temp = xSemaphoreCreateBinary();
    if (semaforo_temp == NULL) {
        printf("ERROR: No se pudo crear el semáforo de temperatura\n");
        return;
    }
    printf("✓ Semáforo de temperatura creado\n");

    // Configurar el ADC
    adc1_config_width(ADC_WIDTH_BIT_12);  // 12 bits (0-4095)
    adc1_config_channel_atten(ADC_CHANNEL_NTC, ADC_ATTEN_DB_11);  // Atenuación para rango 0-3.3V
    printf("✓ ADC configurado (Canal: %d, Rango: 0-3.3V)\n", ADC_CHANNEL_NTC);

    // Crear el timer periódico para leer temperatura (período: 100ms)
    timer_temp = xTimerCreate(
        "TimerTemp",           // Nombre
        pdMS_TO_TICKS(100),    // Período: 100ms
        pdTRUE,                // Auto-reload: sí
        NULL,                  // ID
        timer_temp_callback    // Función callback
    );
    if (timer_temp == NULL) {
        printf("ERROR: No se pudo crear el timer de temperatura\n");
        return;
    }
    printf("✓ Timer de temperatura creado (período: 100ms)\n");

    // Iniciar el timer
    if (xTimerStart(timer_temp, pdMS_TO_TICKS(100)) != pdPASS) {
        printf("ERROR: No se pudo iniciar el timer de temperatura\n");
        return;
    }
    printf("✓ Timer de temperatura iniciado\n");

    // Crear la tarea de lectura de temperatura (Prioridad: 2, Stack: 2048 bytes)
    if (xTaskCreate(tarea_temperatura, "TempTask", 2048, NULL, 2, NULL) != pdPASS) {
        printf("ERROR: No se pudo crear la tarea de temperatura\n");
        return;
    }
    printf("✓ Tarea de temperatura creada\n");

    printf("========== SISTEMA NTC INICIALIZADO ==========\n\n");
}