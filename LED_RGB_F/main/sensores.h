#ifndef SENSORES_H
#define SENSORES_H

#include "freertos/FreeRTOS.h"   // Tipos base de FreeRTOS
#include "freertos/timers.h"     // Tipos para timers de FreeRTOS

// Prototipo del callback del timer de temperatura.
// Esta función se llama automáticamente cada vez que el timer de temperatura expira.
void timer_temp_callback(TimerHandle_t xTimer);

// Prototipo del callback del timer de potenciómetro.
// Esta función se llama automáticamente cada vez que el timer de potenciómetro expira.
void timer_pot_callback(TimerHandle_t xTimer);

// Prototipo de la tarea que lee la temperatura del termistor.
// Esta tarea espera a que el semáforo sea liberado por el timer y luego lee el sensor.
void tarea_temperatura(void *pvParameters);

// Prototipo de la tarea que lee el valor del potenciómetro.
// Esta tarea espera a que el semáforo sea liberado por el timer y luego lee el sensor.
void tarea_potenciometro(void *pvParameters);

#endif