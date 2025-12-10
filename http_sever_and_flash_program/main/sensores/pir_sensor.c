#include "pir_sensor.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "globales.h"

// ============================================================================
// Variables globales declaradas estáticamente
// ============================================================================

// Handles de las tareas
static TaskHandle_t pir_read_task_handle = NULL;

// Configuración de pines
static uint32_t g_pir_gpio;

// ============================================================================
// Función: Tarea de lectura del sensor PIR
// ============================================================================
static void pir_read_task(void *pvParameters)
{
    (void)pvParameters;
    
    uint8_t previous_state = 0;

    printf("Tarea PIR Reader iniciada\n");

    while (1) {
        // Leer el estado del sensor PIR
        uint8_t current_state = gpio_get_level(g_pir_gpio);

        // Solo actualizar si hay cambio de estado
        if (current_state != previous_state) {
            pir_state = current_state ? 1 : 0;
            
            printf("[PIR READ] Estado=%d (Timestamp: %llu ms)\n", 
                   pir_state, (unsigned long long)(esp_timer_get_time() / 1000));

            previous_state = current_state;
        }

        // Esperar 100ms antes de la siguiente lectura
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// Función: Inicialización del sistema
// ============================================================================
void pir_sensor_init(uint32_t pir_gpio, uint32_t led_gpio)
{
    (void)led_gpio;  // No se usa; PWM controlado por NTC
    
    g_pir_gpio = pir_gpio;

    printf("\n========== INICIALIZANDO SISTEMA PIR ==========\n");
    printf("GPIO PIR: %" PRIu32 "\n", pir_gpio);
    printf("Nota: LED PWM controlado por NTC basado en temperatura + presencia PIR\n");

    // Configurar GPIO del sensor PIR como entrada
    gpio_config_t pir_config = {
        .pin_bit_mask = (1ULL << pir_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&pir_config);
    printf("✓ GPIO PIR configurado como entrada\n");

    // Inicializar pir_state
    pir_state = 0;
    printf("✓ pir_state inicializado en 0\n");

    // Crear tarea de lectura del PIR (Prioridad: 2, Stack: 2048 bytes)
    if (xTaskCreate(pir_read_task, "PIR_Reader", 2048, NULL, 2, &pir_read_task_handle) != pdPASS) {
        printf("ERROR: No se pudo crear la tarea PIR Reader\n");
        return;
    }
    printf("✓ Tarea PIR Reader creada\n");

    printf("========== SISTEMA PIR INICIALIZADO ==========\n\n");
}