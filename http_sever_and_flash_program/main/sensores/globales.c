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