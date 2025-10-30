#include <stdio.h>
#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include <string.h>
#include "globales.h"         // Incluye definiciones globales, pines y variables compartidas
#include "led_control.h"      // Prototipo de la tarea de control del LED
#include "sensores.h"         // Prototipos de tareas y callbacks de sensores
#include "uart_thresholds.h"  // Prototipo de la tarea de thresholds por UART
#include "boton_control.h"  // Prototipo de la tarea del botón BOOT

// Definición de variables globales (solo aquí, en otros archivos se usan como extern)
QueueHandle_t cola_temp;         // Cola para pasar la temperatura leída
QueueHandle_t cola_pot;          // Cola para pasar el valor del potenciómetro
QueueHandle_t cola_thresholds;   // Cola para pasar los thresholds de color
QueueHandle_t cola_comando;
SemaphoreHandle_t semaforo_temp; // Semáforo para sincronizar la lectura de temperatura
SemaphoreHandle_t semaforo_pot;  // Semáforo para sincronizar la lectura del potenciómetro
TimerHandle_t timer_temp;        // Timer periódico para la temperatura
TimerHandle_t timer_pot;         // Timer periódico para el potenciómetro

int intervalo_print_temp = 5;    // Intervalo de impresión de temperatura
void app_main(void)
{
    // Configura el ADC para 12 bits de resolución
    adc1_config_width(ADC_WIDTH_BIT_12);
    // Configura la atenuación para el canal del termistor (mayor rango de voltaje)
    adc1_config_channel_atten(ADC_CHANNEL_TERMISTOR, ADC_ATTEN_DB_11);
    // Configura la atenuación para el canal del potenciómetro
    adc1_config_channel_atten(ADC_CHANNEL_POT, ADC_ATTEN_DB_11);

    // Configuración del temporizador PWM para los LEDs RGB
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,      // Modo de alta velocidad
        .timer_num = LEDC_TIMER_0,               // Usa el timer 0
        .duty_resolution = LEDC_TIMER_13_BIT,    // Resolución de 13 bits (0-8191)
        .freq_hz = 5000,                         // Frecuencia de 5 kHz
        .clk_cfg = LEDC_AUTO_CLK                 // Selección automática de reloj
    };
    ledc_timer_config(&ledc_timer);              // Aplica la configuración del timer

    // Configuración de los tres canales PWM para los LEDs RGB
    ledc_channel_config_t ledc_channel[3] = {
        {
            .channel    = LEDC_CHANNEL_0,        // Canal 0: Rojo
            .duty       = 8191,                  // Duty inicial (apagado para ánodo común)
            .gpio_num   = GPIO_ROJO,             // Pin GPIO para LED rojo
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0
        },
        {
            .channel    = LEDC_CHANNEL_1,        // Canal 1: Verde
            .duty       = 8191,
            .gpio_num   = GPIO_VERDE,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0
        },
        {
            .channel    = LEDC_CHANNEL_2,        // Canal 2: Azul
            .duty       = 8191,
            .gpio_num   = GPIO_AZUL,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0
        }
    };
    // Inicializa los tres canales PWM y los deja apagados
    for (int ch = 0; ch < 3; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ledc_channel[ch].channel, 8191);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ledc_channel[ch].channel);
    }

    // Crea las colas para temperatura, potenciómetro y thresholds
    cola_temp = xQueueCreate(5, sizeof(float));           // Cola para temperatura (5 elementos)
    cola_pot = xQueueCreate(5, sizeof(float));            // Cola para potenciómetro (5 elementos)
    cola_thresholds = xQueueCreate(1, sizeof(thresholds_t)); // Cola para thresholds (solo 1 elemento)
    cola_comando = xQueueCreate(2, sizeof(comando_t)); // Cola para comandos ON/OFF

    // Inicializa thresholds por defecto y los envía a la cola (para que siempre haya un valor inicial)
    thresholds_t thresholds_init = {
        .azul_min = 0.0, .azul_max = 18.0,
        .verde_min = 18.0, .verde_max = 26.0,
        .rojo_min = 26.0, .rojo_max = 30.0
    };
    xQueueSend(cola_thresholds, &thresholds_init, 0);

    // Crea los semáforos binarios para sincronizar las tareas de sensores
    semaforo_temp = xSemaphoreCreateBinary();
    semaforo_pot = xSemaphoreCreateBinary();

    // Crea los timers periódicos para disparar las lecturas de sensores
    // Timer de temperatura: cada 100 ms
    timer_temp = xTimerCreate("timer_temp", pdMS_TO_TICKS(100), pdTRUE, NULL, timer_temp_callback);
    // Timer de potenciómetro: cada 30 ms
    timer_pot = xTimerCreate("timer_pot", pdMS_TO_TICKS(30), pdTRUE, NULL, timer_pot_callback);

    // Inicia los timers
    xTimerStart(timer_temp, 0);
    xTimerStart(timer_pot, 0);

    // Crea las tareas principales del sistema
    xTaskCreate(tarea_temperatura, "tarea_temperatura", 2048, NULL, 5, NULL);         // Tarea para leer temperatura
    xTaskCreate(tarea_potenciometro, "tarea_potenciometro", 2048, NULL, 5, NULL);     // Tarea para leer potenciómetro
    xTaskCreate(tarea_control_led, "tarea_control_led", 2048, NULL, 5, NULL);         // Tarea para controlar el LED RGB
    xTaskCreate(tarea_uart_thresholds, "tarea_uart_thresholds", 4096, NULL, 5, NULL); // Tarea para recibir thresholds por UART
    xTaskCreate(tarea_boton_boot, "tarea_boton", 2048, NULL, 5, NULL);                // Tarea para manejar el botón
}
