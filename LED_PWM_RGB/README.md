# LED PWM RGB con ESP32
# Cristian David Chalaca Salas

Este proyecto muestra cómo controlar un LED RGB usando el periférico LEDC (PWM) del ESP32. Permite encender y mezclar colores del LED RGB mediante modulación por ancho de pulso (PWM).

## Conexión del LED RGB

- **Ánodo común:** Conecta la pata común del LED RGB a 3.3V.
- **Cátodo común:** Conecta la pata común del LED RGB a GND.
- Las otras tres patas (rojo, verde, azul) se conectan a los GPIOs del ESP32 a través de una resistencia (220Ω–330Ω recomendada).

| Color  | GPIO ESP32 |
|--------|------------|
| Rojo   | GPIO 2     |
| Verde  | GPIO 4     |
| Azul   | GPIO 5     |

## Archivos principales

- `led_PWM_Basic_main.c`: Ejemplo básico para inicializar PWM y encender el LED RGB.
- `led_library.h` / `led.library.c`: Librería para abstraer la configuración y control de LEDs RGB.

## Ejemplo de uso

El archivo `led_PWM_Basic_main.c` inicializa el temporizador y los canales PWM, y enciende el LED RGB en blanco al 100% (máximo brillo).

```c
// Inicializa PWM para los tres colores
init_rgb_pwm();

// Enciende el LED en blanco (todos los colores al máximo)
ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNEL_RED, 0);
ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNEL_RED);
ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNEL_GREEN, 0);
ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNEL_GREEN);
ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNEL_BLUE, 0);
ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNEL_BLUE);
```

## Requisitos

- ESP-IDF instalado y configurado.
- ESP32.
- LED RGB.
- Resistencias (220Ω–330Ω).

## Personalización

Puedes cambiar los valores de los pines en los defines del archivo principal para adaptarlo a tu hardware.
