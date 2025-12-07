#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

#include <stdint.h>

/**
 * @brief Estructura para los datos de eventos del PIRs
 */
typedef struct {
    uint8_t pir_state;  // 0: sin movimiento, 1: movimiento detectado
    uint64_t timestamp;
} pir_event_t;

/**
 * @brief Inicializa el sistema de sensor PIR con colas y tareas
 * @param pir_gpio Pin GPIO del sensor PIR
 * @param led_gpio Pin GPIO del LED
 */
void pir_sensor_init(uint32_t pir_gpio, uint32_t led_gpio);

#endif // PIR_SENSOR_H