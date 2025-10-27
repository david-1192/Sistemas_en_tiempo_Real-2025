#ifndef UART_THRESHOLDS_H
#define UART_THRESHOLDS_H

// Prototipo de la tarea que permite modificar los thresholds de color por UART.
// Esta tarea recibe comandos del usuario por el puerto serie para consultar o cambiar
// los umbrales de color (azul, verde, rojo) en tiempo real.
void tarea_uart_thresholds(void *pvParameters);

#endif