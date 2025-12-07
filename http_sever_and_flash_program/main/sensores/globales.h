#ifndef GLOBALES_H
#define GLOBALES_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

// ============================================================================
// Definiciones de pines GPIO
// ============================================================================
#define GPIO_PIR    4    // Pin GPIO para el sensor PIR
#define GPIO_LED    5    // Pin GPIO para el LED indicador

// ============================================================================
// Definiciones de canales ADC
// ============================================================================
#define ADC_CHANNEL_NTC  ADC1_CHANNEL_6   // Canal ADC para el NTC (GPIO34)

// ============================================================================
// Declaración de variables globales (solo declaración, no definición)
// ============================================================================

// Colas para pasar datos entre tareas
extern QueueHandle_t cola_temperatura;   // Cola para pasar la temperatura leída del NTC

// Semáforos para sincronizar tareas
extern SemaphoreHandle_t semaforo_temp;  // Semáforo para sincronizar la lectura de temperatura

// Timers periódicos
extern TimerHandle_t timer_temp;         // Timer periódico para la lectura de temperatura

// Variables de configuración
extern int intervalo_print_temp;         // Intervalo de impresión de temperatura (en decisegundos, 0.1s)

#endif // GLOBALES_H