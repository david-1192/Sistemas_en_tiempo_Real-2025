#include "ntc.h"
#include "globales.h"
#include "pwm_led.h"
#include "wifi_app.h"
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
    TickType_t last_pir_active_time = 0;
    TickType_t pir_holdtime_ms = 3000;  // Mantener PWM 3 segundos tras detección

    printf("Tarea Temperatura (NTC) iniciada\n");

    while (1) {
        if (xSemaphoreTake(semaforo_temp, portMAX_DELAY) == pdTRUE) {
            TickType_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            int adc_raw = adc1_get_raw(ADC_CHANNEL_NTC);
            float Vadc = (adc_raw / 4095.0f) * 3.3f;
            float R_serie = 10000.0f;

            // Calcula la resistencia del termistor
            float R_ntc = (Vadc * R_serie) / (3.3f - Vadc);

            // Fórmula INVERTIDA: Temperatura AUMENTA cuando R_ntc DISMINUYE
            temp = 25.0f - (R_ntc - 10000.0f) * (50.0f - 25.0f) / (15000.0f - 10000.0f);

            current_temperature = temp;

            // Registrar tiempo cuando hay presencia PIR
            if (pir_state == 1) {
                last_pir_active_time = current_time;
            }

            // Determinar si PIR está "activo" (incluyendo holdtime)
            TickType_t time_since_pir = current_time - last_pir_active_time;
            uint8_t pir_active_with_holdtime = (time_since_pir < pir_holdtime_ms) ? 1 : 0;

            // Control del LED PWM según temperatura o modo manual
            uint32_t duty = 0;

            if (current_mode == 0) { // MANUAL
                // Convertir porcentaje (0-100) a duty (0-8191)
                duty = (uint32_t)((manual_pwm_duty / 100.0f) * 8191.0f);
            } else if (current_mode == 1) { // AUTOMATICO
                // Mapeo lineal usando los límites configurables
                float minT = current_temp_min;
                float maxT = current_temp_max;

                if (minT >= maxT) {
                    minT = 20.0f; maxT = 40.0f;
                }

                // Determinar si PIR está "activo" (incluyendo holdtime)
                TickType_t time_since_pir = current_time - last_pir_active_time;
                uint8_t pir_active_with_holdtime = (time_since_pir < pir_holdtime_ms) ? 1 : 0;

                // Activar PWM si hay presencia PIR (con holdtime)
                if (pir_active_with_holdtime == 1) {
                    if (temp > minT && temp < maxT) {
                        duty = (uint32_t)((temp - minT) / (maxT - minT) * 8191.0f);
                        if (duty > 8191) duty = 8191;
                    } else if (temp >= maxT) {
                        duty = 8191;
                    } else {
                        duty = 0;
                    }
                } else {
                    duty = 0;  // Sin presencia (holdtime expirado): apagar
                }
            } else { // PROGRAMADO (modo 2)
                // Only enable if selected schedule is active
                if (wifi_app_is_selected_register_active()) {
                    if (temp > 20.0f && temp < 40.0f) {
                        duty = (uint32_t)((temp - 20.0f) / 20.0f * 8191.0f);
                    } else if (temp >= 40.0f) {
                        duty = 8191;
                    } else {
                        duty = 0;
                    }
                } else {
                    duty = 0;
                }
            }

            // Enviar comando PWM a la cola
            pwm_command_t cmd = { .duty = duty };
            xQueueSend(cola_pwm_led, &cmd, pdMS_TO_TICKS(100));

            // Imprimir información cada intervalo_print_temp
            if (++print_counter >= intervalo_print_temp) {
                printf("[NTC] Temperatura: %.2f °C | PIR: %s (tiempo restante: %lu ms)\n",
                       temp, 
                       pir_active_with_holdtime ? "ACTIVO" : "INACTIVO",
                       pir_active_with_holdtime ? (pir_holdtime_ms - time_since_pir) : 0);
                print_counter = 0;
            }

            // Enviar temperatura a la cola para otras tareas
            if (xQueueSend(cola_temperatura, &temp, pdMS_TO_TICKS(100)) != pdPASS) {
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

    // Crear el timer periódico para leer temperatura (período: 500ms en lugar de 100ms)
    timer_temp = xTimerCreate(
        "TimerTemp",           // Nombre
        pdMS_TO_TICKS(500),    // Período: 500ms (reducir contención)
        pdTRUE,                // Auto-reload: sí
        NULL,                  // ID
        timer_temp_callback    // Función callback
    );
    if (timer_temp == NULL) {
        printf("ERROR: No se pudo crear el timer de temperatura\n");
        return;
    }
    printf("✓ Timer de temperatura creado (período: 500ms)\n");

    // Iniciar el timer
    if (xTimerStart(timer_temp, pdMS_TO_TICKS(500)) != pdPASS) {
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

    // Cargar rango de temperatura desde flash
    load_temp_range();
    printf("✓ Rango de temperatura cargado\n");

    // Cargar PWM manual desde flash
    load_manual_pwm();
    printf("✓ PWM manual cargado\n");

    printf("========== SISTEMA NTC INICIALIZADO ==========\n\n");
}