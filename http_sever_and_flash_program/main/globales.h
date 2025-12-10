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

// Cola PWM LED
extern QueueHandle_t cola_pwm_led;       // Cola para enviar comandos PWM

// Semáforos para sincronizar tareas
extern SemaphoreHandle_t semaforo_temp;  // Semáforo para sincronizar la lectura de temperatura

// Timers periódicos
extern TimerHandle_t timer_temp;         // Timer periódico para la lectura de temperatura

// Variables de configuración
extern int intervalo_print_temp;         // Intervalo de impresión de temperatura (en decisegundos, 0.1s)

// Variables de estado
extern float current_temperature;        // Temperatura actual
extern int pir_state;                     // 0 = OFF, 1 = ON
extern int current_mode;                  // 0 = MANUAL, 1 = AUTOMATICO, 2 = PROGRAMADO
extern uint32_t current_pwm_duty;         // 0..8191

// Variable para PWM manual (0-100%)
extern uint32_t manual_pwm_duty;

// Estado modo AUTOMÁTICO
extern float current_temp_min;
extern float current_temp_max;

// Funciones para guardar/cargar rango de temperatura
void save_temp_range(float temp_min, float temp_max);
void load_temp_range(void);

// Funciones para guardar/cargar PWM manual
void save_manual_pwm(uint32_t duty_pct);
void load_manual_pwm(void);

#endif // GLOBALES_H
