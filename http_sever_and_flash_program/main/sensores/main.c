#include <stdio.h>
#include "pir_sensor.h"
#include "ntc.h"
#include "pwm_led.h"

void app_main(void)
{
    printf("\n========== INICIALIZANDO APLICACIÓN ==========\n");
    
    // Inicializar sistema PIR
    pir_sensor_init(4, 5);
    
    // Inicializar sistema NTC
    ntc_init();
    
    // Inicializar sistema PWM LED GPIO 18
    pwm_led_init(18);
    
    printf("========== APLICACIÓN INICIADA ==========\n\n");
}