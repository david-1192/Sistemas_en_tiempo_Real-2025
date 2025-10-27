#include "uart_thresholds.h"
#include "globales.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

// Tarea que permite modificar los thresholds de color por UART.
// El usuario puede enviar comandos como "azul=25" para cambiar el umbral del color azul.
// También puede enviar "?" para consultar los valores actuales de thresholds.
void tarea_uart_thresholds(void *pvParameters) {
    char buffer[32]; // Buffer para almacenar el comando recibido por UART

    const int uart_num = UART_NUM_0;
    uart_config_t uart_config = {
        .baud_rate = 115200,                // Velocidad de transmisión UART
        .data_bits = UART_DATA_8_BITS,      // 8 bits de datos
        .parity    = UART_PARITY_DISABLE,   // Sin paridad
        .stop_bits = UART_STOP_BITS_1,      // 1 bit de parada
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE // Sin control de flujo
    };
    uart_param_config(uart_num, &uart_config); // Configura la UART
    uart_driver_install(uart_num, 1024, 0, 0, NULL, 0); // Instala el driver UART

    char rx_buffer[64];
    while (1) {
        // Lee datos recibidos por UART (espera hasta 100 ms)
        int len = uart_read_bytes(UART_NUM_0, (uint8_t*)rx_buffer, sizeof(rx_buffer) - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            rx_buffer[len] = '\0';

            // Copia el comando recibido a buffer para procesarlo correctamente
            strncpy(buffer, rx_buffer, sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';

            comando_t cmd;
            if (strcasestr(buffer, "ON")) {
                cmd = CMD_ON;
                xQueueSend(cola_comando, &cmd, 0);
                uart_write_bytes(UART_NUM_0, "Sistema encendido\r\n", 19);
            } else if (strcasestr(buffer, "OFF")) {
                cmd = CMD_OFF;
                xQueueSend(cola_comando, &cmd, 0);
                uart_write_bytes(UART_NUM_0, "Sistema apagado\r\n", 17);
            } else {
                float valor;
                char respuesta[128];

                // Elimina espacios y saltos de línea al inicio del comando
                char *ptr = buffer;
                while (*ptr == ' ' || *ptr == '\r' || *ptr == '\n') ptr++;

                // Si el usuario escribe "?" muestra los valores actuales de thresholds
                if (strchr(ptr, '?') != NULL && strlen(ptr) == 1) {
                    thresholds_t current;
                    if (xQueuePeek(cola_thresholds, &current, 0) == pdTRUE) {
                        snprintf(respuesta, sizeof(respuesta),
                            "Thresholds actuales:\r\n"
                            "  azul  = %.2f\r\n"
                            "  verde = %.2f\r\n"
                            "  rojo  = %.2f\r\n\r\n",
                            current.t_azul, current.t_verde, current.t_rojo);
                    } else {
                        snprintf(respuesta, sizeof(respuesta),
                            "No se pudo leer los thresholds actuales.\r\n");
                    }
                    uart_write_bytes(uart_num, respuesta, strlen(respuesta));
                    continue;
                }

                // Procesa comandos tipo "<color>=<valor>"
                char *igual = strchr(ptr, '=');
                if (igual) {
                    *igual = '\0'; // Separa clave y valor
                    char *clave = ptr;
                    char *valor_str = igual + 1;

                    // Elimina espacios al final de la clave
                    char *end = clave + strlen(clave) - 1;
                    while (end > clave && *end == ' ') {
                        *end = '\0';
                        end--;
                    }
                    // Elimina espacios al inicio del valor
                    while (*valor_str == ' ') valor_str++;

                    // Intenta convertir el valor a float
                    if (sscanf(valor_str, "%f", &valor) == 1) {
                        if (valor < 0.0 || valor > 100.0) {
                            // Valor fuera de rango permitido
                            snprintf(respuesta, sizeof(respuesta), "Valor fuera de rango (0-100): %.2f\r\n", valor);
                            uart_write_bytes(uart_num, respuesta, strlen(respuesta));
                        } else {
                            // Lee el valor actual de thresholds de la cola
                            thresholds_t current;
                            if (xQueuePeek(cola_thresholds, &current, 0) != pdTRUE) {
                                // Si la cola está vacía, usa valores por defecto
                                current.t_azul = 20.0;
                                current.t_verde = 25.0;
                                current.t_rojo = 40.0;
                            }

                            // Actualiza el threshold correspondiente según el color
                            if (strncasecmp(clave, "azul", 4) == 0) {
                                current.t_azul = valor;
                                snprintf(respuesta, sizeof(respuesta), "Nuevo threshold azul: %.2f\r\n", valor);
                            } else if (strncasecmp(clave, "verde", 5) == 0) {
                                current.t_verde = valor;
                                snprintf(respuesta, sizeof(respuesta), "Nuevo threshold verde: %.2f\r\n", valor);
                            } else if (strncasecmp(clave, "rojo", 4) == 0) {
                                current.t_rojo = valor;
                                snprintf(respuesta, sizeof(respuesta), "Nuevo threshold rojo: %.2f\r\n", valor);
                            } else {
                                // Si el color no es reconocido
                                snprintf(respuesta, sizeof(respuesta), "Color no reconocido: %s\r\n", clave);
                                uart_write_bytes(uart_num, respuesta, strlen(respuesta));
                                continue;
                            }

                            // Elimina el valor anterior si la cola está llena (tamaño 1)
                            if (uxQueueMessagesWaiting(cola_thresholds) > 0) {
                                thresholds_t dummy;
                                xQueueReceive(cola_thresholds, &dummy, 0);
                            }
                            // Envía el nuevo threshold actualizado a la cola
                            xQueueSend(cola_thresholds, &current, portMAX_DELAY);
                            // Envía mensaje de confirmación por UART
                            uart_write_bytes(uart_num, respuesta, strlen(respuesta));
                        }
                    } else {
                        // Si el valor no es válido
                        snprintf(respuesta, sizeof(respuesta), "Valor no valido: %s\r\n", valor_str);
                        uart_write_bytes(uart_num, respuesta, strlen(respuesta));
                    }
                } else {
                    // Si el comando no es reconocido
                    snprintf(respuesta, sizeof(respuesta), "Comando no reconocido: %s\r\n", buffer);
                    uart_write_bytes(uart_num, respuesta, strlen(respuesta));
                }
            }
        }
        // Pequeño retardo para evitar saturar el CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}