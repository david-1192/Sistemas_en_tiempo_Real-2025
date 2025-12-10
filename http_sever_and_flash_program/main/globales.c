#include "globales.h"

// ============================================================================
// Definición de variables globales (aquí van las instancias reales)
// ============================================================================
// Colas para pasar datos entre tareas
QueueHandle_t cola_temperatura = NULL;
QueueHandle_t cola_pwm_led = NULL;

// Semáforos para sincronizar tareas
SemaphoreHandle_t semaforo_temp = NULL;

// Timers periódicos
TimerHandle_t timer_temp = NULL;

// Variables de configuración
int intervalo_print_temp = 20;  // 20 * 100ms = 20 segundos entre impresiones

// Variables de estado
float current_temperature = 0.0f;
int pir_state = 0;
int current_mode = 1; // por ejemplo AUTOMATICO por defecto
uint32_t current_pwm_duty = 0;

// PWM manual (0-100)
uint32_t manual_pwm_duty = 0;

// Valores por defecto para modo automático
float current_temp_min = 20.0f;
float current_temp_max = 40.0f;

// ============================================================================
// Funciones para almacenar rango de temperatura en NVS (Flash)
// ============================================================================
#include "nvs_flash.h"

void save_temp_range(float temp_min, float temp_max)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        printf("[GLOBALES] Error abriendo NVS\n");
        return;
    }

    // Guardar como blobs (datos binarios)
    err = nvs_set_blob(nvs_handle, "temp_min", &temp_min, sizeof(float));
    if (err == ESP_OK) {
        err = nvs_set_blob(nvs_handle, "temp_max", &temp_max, sizeof(float));
    }

    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            printf("[GLOBALES] Rango de temperatura guardado: %.2f - %.2f\n", temp_min, temp_max);
        }
    } else {
        printf("[GLOBALES] Error guardando rango de temperatura\n");
    }

    nvs_close(nvs_handle);
}

void load_temp_range(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    float temp_min = 20.0f, temp_max = 40.0f;  // Valores por defecto
    size_t required_size = sizeof(float);

    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("[GLOBALES] NVS no inicializado, usando valores por defecto\n");
        current_temp_min = temp_min;
        current_temp_max = temp_max;
        return;
    }

    // Intentar cargar valores guardados
    err = nvs_get_blob(nvs_handle, "temp_min", &temp_min, &required_size);
    if (err == ESP_OK) {
        err = nvs_get_blob(nvs_handle, "temp_max", &temp_max, &required_size);
    }

    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        current_temp_min = temp_min;
        current_temp_max = temp_max;
        printf("[GLOBALES] Rango de temperatura cargado desde flash: %.2f - %.2f\n", 
               current_temp_min, current_temp_max);
    } else {
        current_temp_min = temp_min;
        current_temp_max = temp_max;
        printf("[GLOBALES] Error cargando rango, usando valores por defecto: %.2f - %.2f\n", 
               current_temp_min, current_temp_max);
    }
}

void save_manual_pwm(uint32_t duty_pct)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    if (duty_pct > 100) duty_pct = 100;

    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        printf("[GLOBALES] Error abriendo NVS para PWM manual\n");
        return;
    }

    err = nvs_set_u32(nvs_handle, "pwm_manual", duty_pct);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            printf("[GLOBALES] PWM manual guardado: %lu%%\n", duty_pct);
        }
    } else {
        printf("[GLOBALES] Error guardando PWM manual\n");
    }

    nvs_close(nvs_handle);
}

void load_manual_pwm(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    uint32_t pwm_pct = 0;

    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("[GLOBALES] NVS no inicializado para PWM manual\n");
        manual_pwm_duty = 0;
        return;
    }

    err = nvs_get_u32(nvs_handle, "pwm_manual", &pwm_pct);
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        manual_pwm_duty = pwm_pct;
        printf("[GLOBALES] PWM manual cargado: %lu%%\n", manual_pwm_duty);
    } else {
        manual_pwm_duty = 0;
        printf("[GLOBALES] Sin PWM manual guardado, usando 0%%\n");
    }
}
