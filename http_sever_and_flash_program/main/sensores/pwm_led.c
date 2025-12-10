#include "pwm_led.h"
#include "globales.h"
#include "driver/ledc.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>

// Configuración PWM
#define PWM_TIMER       LEDC_TIMER_1
#define PWM_SPEED_MODE  LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL     LEDC_CHANNEL_3
#define PWM_DUTY_RES    LEDC_TIMER_13_BIT  // 13 bits = 0-8191
#define PWM_FREQUENCY   1000               // 1 kHz

// Variables globales
static int g_led_gpio = -1;

// ============================================================================
// Tarea que controla el LED con PWM
// ============================================================================
void tarea_pwm_led(void *pvParameters)
{
    (void)pvParameters;
    pwm_command_t comando;
    int print_counter = 0;

    printf("Tarea PWM LED iniciada\n");

    while (1) {
        // Espera un comando de la cola (bloquea indefinidamente)
        if (xQueueReceive(cola_pwm_led, &comando, portMAX_DELAY) == pdTRUE) {
            // Limita el duty cycle al rango válido (0-8191)
            uint32_t duty = comando.duty;
            if (duty > 8191) duty = 8191;

            current_pwm_duty = duty;

            // Aplica el nuevo duty cycle al LED
            ledc_set_duty(PWM_SPEED_MODE, PWM_CHANNEL, duty);
            ledc_update_duty(PWM_SPEED_MODE, PWM_CHANNEL);

            // Imprimir solo cada N comandos (control de velocidad)
            if (++print_counter >= 20) {  // Imprime cada 20 comandos (~2 segundos)
                printf("[PWM] Duty: %lu (%.1f%%)\n", duty, (duty / 8191.0f) * 100.0f);
                print_counter = 0;
            }
        }
    }
}

// ============================================================================
// Inicialización del sistema PWM
// ============================================================================
void pwm_led_init(int gpio_num)
{
    g_led_gpio = gpio_num;   // <-- inicializa la variable global del gpio

    printf("\n========== INICIALIZANDO SISTEMA PWM LED ==========\n");

    // Crear la cola PWM (capacidad: 20 comandos - aumentado para evitar saturación)
    cola_pwm_led = xQueueCreate(20, sizeof(pwm_command_t));
    if (cola_pwm_led == NULL) {
        printf("ERROR: No se pudo crear la cola PWM\n");
        return;
    }
    printf("✓ Cola PWM creada\n");

    // Configurar el timer LEDC
    ledc_timer_config_t timer = {
        .speed_mode = PWM_SPEED_MODE,
        .duty_resolution = PWM_DUTY_RES,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    if (ledc_timer_config(&timer) != ESP_OK) {
        printf("ERROR: No se pudo configurar el timer LEDC\n");
        return;
    }
    printf("✓ Timer LEDC configurado (Frecuencia: %d Hz)\n", PWM_FREQUENCY);

    // Configurar el canal LEDC
    ledc_channel_config_t canal = {
        .speed_mode = PWM_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .timer_sel = PWM_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = gpio_num,
        .duty = 0,
        .hpoint = 0
    };
    if (ledc_channel_config(&canal) != ESP_OK) {
        printf("ERROR: No se pudo configurar el canal LEDC\n");
        return;
    }
    printf("✓ Canal LEDC configurado (GPIO: %d, Frecuencia: %d Hz)\n", gpio_num, PWM_FREQUENCY);

    // Crear la tarea PWM (Prioridad: 2, Stack: 2048 bytes)
    if (xTaskCreate(tarea_pwm_led, "PWMLEDTask", 2048, NULL, 2, NULL) != pdPASS) {
        printf("ERROR: No se pudo crear la tarea PWM\n");
        return;
    }
    printf("✓ Tarea PWM LED creada\n");

    printf("========== SISTEMA PWM LED INICIALIZADO ==========\n\n");
}
