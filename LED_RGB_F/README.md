# Sistema de Control de LED RGB con ESP32

Este proyecto implementa un sistema de control de un LED RGB usando un ESP32, sensores analógicos y comunicación UART. El sistema permite cambiar el color y brillo del LED según la temperatura (termistor NTC) y el valor de un potenciómetro, además de modificar los umbrales de color en tiempo real por UART.

## Características

- **Control de LED RGB**: Cambia el color (rojo, verde, azul) según la temperatura medida por el termistor NTC.
- **Brillo ajustable**: El brillo del LED se ajusta con un potenciómetro conectado al ADC.
- **Umbrales configurables**: Los umbrales de temperatura para cada color se pueden modificar por comandos UART.
- **Encendido/apagado**: El sistema puede encenderse o apagarse usando el botón BOOT físico o por comandos UART.
- **FreeRTOS**: Uso de tareas, colas, semáforos y timers para la gestión concurrente de sensores y control.

## Hardware Requerido

- ESP32 DevKit (o compatible)
- LED RGB (ánodo común)
- Termistor NTC 10kΩ
- Potenciómetro 10kΩ
- Botón físico (BOOT, GPIO0)
- Cables de conexión

## Conexiones

- **LED RGB**: GPIO25 (Rojo), GPIO26 (Verde), GPIO27 (Azul)
- **Termistor NTC**: Divisor de voltaje entre 3.3V y GND, salida al GPIO34 (ADC1_CHANNEL_6)
- **Potenciómetro**: Extremos a 3.3V y GND, salida central al GPIO35 (ADC1_CHANNEL_7)
- **Botón BOOT**: GPIO0

## Comandos UART

- `ON` / `OFF`: Enciende o apaga el sistema.
- `azul=<valor>`: Cambia el umbral de temperatura para el color azul.
- `verde=<valor>`: Cambia el umbral para verde.
- `rojo=<valor>`: Cambia el umbral para rojo.
- `?`: Consulta los valores actuales de los umbrales.

## Ejemplo de Uso

1. Conecta el hardware según el esquema.
2. Compila y flashea el proyecto con ESP-IDF:
   ```powershell
   idf.py build
   idf.py -p COMx flash monitor
   ```
3. Usa el botón BOOT o comandos UART para controlar el sistema.
4. Observa el color y brillo del LED según la temperatura y el potenciómetro.

## Estructura del Código

- `main.c`: Inicialización de hardware, colas, semáforos y tareas.
- `led_control.c`: Lógica de control del LED RGB.
- `sensores.c`: Lectura de termistor y potenciómetro.
- `uart_thresholds.c`: Recepción y procesamiento de comandos UART.
- `boton_control.c`: Lógica del botón BOOT.