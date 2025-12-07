#ifndef NTC_H
#define NTC_H

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

/**
 * @brief Callback del timer de temperatura
 * Libera el semáforo para que la tarea lea el sensor
 */
void timer_temp_callback(TimerHandle_t xTimer);

/**
 * @brief Tarea que lee la temperatura del NTC
 * Espera a que el semáforo sea liberado por el timer y luego lee el sensor
 */
void tarea_temperatura(void *pvParameters);

/**
 * @brief Inicializa el sistema de lectura de temperatura
 * Configura el ADC, crea la cola, semáforo y timer
 */
void ntc_init(void);

#endif // NTC_H