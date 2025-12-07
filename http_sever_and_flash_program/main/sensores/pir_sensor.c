#include "pir_sensor.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"

// ============================================================================
// Variables globales declaradas estáticamente
// ============================================================================

// Cola para pasar eventos del PIR
static QueueHandle_t pir_event_queue = NULL;

// Handles de las tareas
static TaskHandle_t pir_read_task_handle = NULL;
static TaskHandle_t led_control_task_handle = NULL;

// Configuración de pines
static uint32_t g_pir_gpio;
static uint32_t g_led_gpio;

// ============================================================================
// Función: Tarea de lectura del sensor PIR
// ============================================================================
static void pir_read_task(void *pvParameters)
{
    (void)pvParameters;
    
    pir_event_t pir_event;
    uint8_t previous_state = 0;

    printf("Tarea PIR Reader iniciada\n");

    while (1) {
        // Leer el estado del sensor PIR
        uint8_t current_state = gpio_get_level(g_pir_gpio);

        // Solo enviar evento si hay cambio de estado
        if (current_state != previous_state) {
            pir_event.pir_state = current_state;
            pir_event.timestamp = esp_timer_get_time() / 1000;  // Convertir a ms

            // Enviar evento a la cola
            if (xQueueSend(pir_event_queue, &pir_event, pdMS_TO_TICKS(100)) == pdPASS) {
                printf("[PIR READ] Evento enviado: Estado=%d, Timestamp=%llu ms\n", 
                       pir_event.pir_state, (unsigned long long)pir_event.timestamp);
            } else {
                printf("[PIR READ] Error: Cola llena\n");
            }

            previous_state = current_state;
        }

        // Esperar 100ms antes de la siguiente lectura
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// Función: Tarea de control del LED
// ============================================================================
static void led_control_task(void *pvParameters)
{
    (void)pvParameters;
    
    pir_event_t received_event;

    printf("Tarea LED Control iniciada\n");

    while (1) {
        // Esperar evento de la cola (con timeout de 1 segundo)
        if (xQueueReceive(pir_event_queue, &received_event, pdMS_TO_TICKS(1000)) == pdPASS) {
            if (received_event.pir_state == 1) {
                // Movimiento detectado: encender LED
                gpio_set_level(g_led_gpio, 1);
                printf("[LED CONTROL] ¡PIR ACTIVADO! LED Encendido (ts: %llu ms)\n", 
                       (unsigned long long)received_event.timestamp);
            } else {
                // Sin movimiento: apagar LED
                gpio_set_level(g_led_gpio, 0);
                printf("[LED CONTROL] PIR desactivado. LED Apagado (ts: %llu ms)\n", 
                       (unsigned long long)received_event.timestamp);
            }
        } else {
            // Timeout: no hay eventos
            printf("[LED CONTROL] Esperando eventos...\n");
        }
    }
}

// ============================================================================
// Función: Inicialización del sistema
// ============================================================================
void pir_sensor_init(uint32_t pir_gpio, uint32_t led_gpio)
{
    g_pir_gpio = pir_gpio;
    g_led_gpio = led_gpio;

    printf("\n========== INICIALIZANDO SISTEMA PIR ==========\n");
    printf("GPIO PIR: %" PRIu32 "\n", pir_gpio);
    printf("GPIO LED: %" PRIu32 "\n", led_gpio);

    // Crear la cola para eventos (capacidad: 10 eventos)
    pir_event_queue = xQueueCreate(10, sizeof(pir_event_t));
    if (pir_event_queue == NULL) {
        printf("ERROR: No se pudo crear la cola de eventos\n");
        return;
    }
    printf("✓ Cola de eventos creada\n");

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

    // Configurar GPIO del LED como salida
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << led_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_config);
    printf("✓ GPIO LED configurado como salida\n");

    // Iniciar LED apagado
    gpio_set_level(g_led_gpio, 0);
    printf("✓ LED inicializado apagado\n");

    // Crear tarea de lectura del PIR (Prioridad: 2, Stack: 2048 bytes)
    if (xTaskCreate(pir_read_task, "PIR_Reader", 2048, NULL, 2, &pir_read_task_handle) != pdPASS) {
        printf("ERROR: No se pudo crear la tarea PIR Reader\n");
        return;
    }
    printf("✓ Tarea PIR Reader creada\n");

    // Crear tarea de control del LED (Prioridad: 2, Stack: 2048 bytes)
    if (xTaskCreate(led_control_task, "LED_Control", 2048, NULL, 2, &led_control_task_handle) != pdPASS) {
        printf("ERROR: No se pudo crear la tarea LED Control\n");
        return;
    }
    printf("✓ Tarea LED Control creada\n");

    printf("========== SISTEMA PIR INICIALIZADO ==========\n\n");
}