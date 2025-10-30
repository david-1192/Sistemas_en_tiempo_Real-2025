#ifndef GLOBALES_H
#define GLOBALES_H

#include "freertos/FreeRTOS.h"   // Tipos y funciones base de FreeRTOS
#include "freertos/queue.h"      // Tipos y funciones para colas
#include "freertos/semphr.h"     // Tipos y funciones para semáforos
#include "freertos/timers.h"     // Tipos y funciones para timers

// Definiciones de pines GPIO para los LEDs RGB
#define GPIO_ROJO   25           // Pin GPIO para el LED rojo
#define GPIO_VERDE  26           // Pin GPIO para el LED verde
#define GPIO_AZUL   27           // Pin GPIO para el LED azul

// Definiciones de canales ADC para sensores
#define ADC_CHANNEL_TERMISTOR ADC1_CHANNEL_6   // Canal ADC para el termistor (GPIO34)
#define ADC_CHANNEL_POT       ADC1_CHANNEL_7   // Canal ADC para el potenciómetro (GPIO35)

// Estructura para almacenar los thresholds de cada color
typedef struct {
    float azul_min, azul_max;    // Umbral para el color azul (mínimo y máximo)
    float verde_min, verde_max;   // Umbral para el color verde (mínimo y máximo)
    float rojo_min, rojo_max;     // Umbral para el color rojo (mínimo y máximo)
} thresholds_t;

typedef enum { CMD_ON, CMD_OFF } comando_t;

// Declaración de variables globales compartidas entre módulos (solo declaración, no definición)
extern QueueHandle_t cola_temp;         // Cola para pasar la temperatura leída
extern QueueHandle_t cola_pot;          // Cola para pasar el valor del potenciómetro
extern QueueHandle_t cola_thresholds;   // Cola para pasar los thresholds de color
extern QueueHandle_t cola_comando;      // Cola para pasar comandos ON/OFF
extern SemaphoreHandle_t semaforo_temp; // Semáforo para sincronizar la lectura de temperatura
extern SemaphoreHandle_t semaforo_pot;  // Semáforo para sincronizar la lectura del potenciómetro
extern TimerHandle_t timer_temp;        // Timer periódico para la temperatura
extern TimerHandle_t timer_pot;         // Timer periódico para el potenciómetro
extern int intervalo_print_temp;    // Intervalo de impresión de temperatura

#endif