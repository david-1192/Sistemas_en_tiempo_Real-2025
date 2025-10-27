#ifndef LED_CONTROL_H
#define LED_CONTROL_H

// Prototipo de la tarea que controla el color y brillo del LED RGB
// Esta tarea lee la temperatura y el valor del potenciómetro desde las colas,
// consulta los thresholds actuales y ajusta el color del LED en consecuencia.
void tarea_control_led(void *pvParameters);

#endif