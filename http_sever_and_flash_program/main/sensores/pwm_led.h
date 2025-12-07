#ifndef PWM_LED_H
#define PWM_LED_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Estructura para controlar el PWM del LED
 */
typedef struct {
    uint32_t duty;  // Ciclo de trabajo (0-8191 para 13 bits)
} pwm_command_t;

/**
 * @brief Tarea que controla el LED con PWM
 * Lee comandos de la cola y ajusta el duty cycle
 */
void tarea_pwm_led(void *pvParameters);

/**
 * @brief Inicializa el sistema PWM para el LED
 * Configura el timer, canal, GPIO y crea la tarea
 */
void pwm_led_init(int gpio_num);

#endif // PWM_LED_H
