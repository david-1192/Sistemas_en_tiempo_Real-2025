# LED RGB Control con ESP32 y FreeRTOS

Este proyecto implementa un sistema de control de un LED RGB usando un ESP32, sensores analógicos (termistor y potenciómetro), FreeRTOS y comunicación UART. Permite configurar los umbrales de color, el intervalo de impresión y el encendido/apagado del sistema tanto por UART como por botón físico.

## Características

- **Control de LED RGB**: El color y brillo del LED RGB se ajusta según la temperatura y el valor del potenciómetro.
- **Configuración dinámica**: Los umbrales de temperatura para cada color y el intervalo de impresión pueden configurarse en tiempo real por UART.
- **Rangos solapados**: Es posible definir rangos de temperatura solapados para que varios colores se enciendan simultáneamente.
- **Lectura de sensores**: 
  - Termistor para medir temperatura.
  - Potenciómetro para ajustar el brillo.
- **Comandos UART**:
  - Consultar configuración y voltaje del potenciómetro (`?`).
  - Cambiar umbrales de color: `azul=min,max`, `verde=min,max`, `rojo=min,max`.
  - Cambiar intervalo de impresión: `intervalo=segundos`.
  - Encender/apagar sistema: `ON` / `OFF`.
- **Botón físico**: Permite alternar entre encendido y apagado del sistema.

## Hardware necesario

- ESP32 DevKit
- LED RGB (ánodo común)
- Termistor NTC (ej. 10k)
- Potenciómetro (ej. 10k)
- Botón (conectado a GPIO0)
- Resistencias varias

## Conexiones sugeridas

| Señal         | GPIO ESP32 | Notas                |
|---------------|------------|----------------------|
| LED Rojo      | 25         | PWM (LEDC_CHANNEL_0) |
| LED Verde     | 26         | PWM (LEDC_CHANNEL_1) |
| LED Azul      | 27         | PWM (LEDC_CHANNEL_2) |
| Termistor     | 34 (ADC1_6)| Divisor resistivo    |
| Potenciómetro | 35 (ADC1_7)| Salida central a ADC |
| Botón BOOT    | 0          | Pull-up interno      |

## Estructura del código

- `main.c`: Inicialización de hardware, recursos FreeRTOS y tareas.
- `led_control.c/h`: Lógica de control del LED RGB según temperatura y potenciómetro.
- `sensores.c/h`: Lectura de sensores y envío de datos por colas.
- `uart_thresholds.c/h`: Parser de comandos UART para configuración dinámica.
- `boton_control.c/h`: Lógica del botón físico para ON/OFF.
- `globales.h`: Definiciones globales, pines, colas y estructura de thresholds.

## Comandos UART

- `?`  
  Muestra los rangos actuales de cada color y el voltaje del potenciómetro.

- `azul=min,max`  
  Define el rango de temperatura para el color azul.

- `verde=min,max`  
  Define el rango de temperatura para el color verde.

- `rojo=min,max`  
  Define el rango de temperatura para el color rojo.

- `intervalo=segundos`  
  Cambia el intervalo de impresión de la temperatura (1 a 60 segundos).

- `ON` / `OFF`  
  Enciende o apaga el sistema.

## Ejemplo de uso

```
azul=0,20
verde=18,28
rojo=25,40
intervalo=5
?
```

## Notas

- Los rangos pueden solaparse, permitiendo que varios colores se enciendan al mismo tiempo.
- El sistema es modular y fácilmente extensible para más sensores o salidas.
- El voltaje del potenciómetro se muestra en la consulta `?`.

## Licencia

MIT

---

Desarrollado por [Tu Nombre o Grupo] - 2025