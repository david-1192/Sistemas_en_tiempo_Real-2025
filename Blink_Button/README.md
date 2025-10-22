# Proyecto LED Blink con Control por Botón
## 📋 Descripción
Este proyecto implementa el control de un LED mediante un botón (BOOT o externo) en una placa ESP32. El LED parpadea continuamente hasta que se presiona el botón, momento en el que se detiene. Al presionar nuevamente el botón, el parpadeo se reanuda.

## 🎯 Funcionalidades
- **Parpadeo controlado**: LED parpadea cada 500ms
- **Control por botón**: Botón BOOT (GPIO0) actúa como interruptor
- **Detección de flanco**: Detecta pulsaciones precisas
- **Anti-rebote**: Evita lecturas múltiples por rebote mecánico
- **Estado persistente**: Mantiene el estado entre pulsaciones

## 🛠️ Hardware Requerido
- Placa ESP32 (DevKit, NodeMCU, etc.)
- LED azul en GPIO2 (LED integrado en la mayoría de placas)
- Botón BOOT (integrado) o botón externo en GPIO0

## 🔧 Funciones Principales

### `init_led()`
- Configura el pin del LED como salida
- Reinicia el estado del pin

### `init_button()`
- Configura el pin del botón como entrada
- Habilita resistencia pull-up interna

### `blink_led()`
- Alterna el estado del LED (ON/OFF)
- Actualiza el estado global del LED

### `check_button()`
- Lee el estado actual del botón
- Detecta flanco descendente (pulsación)
- Implementa anti-rebote de 50ms
- Controla el estado del parpadeo

## 🎮 Comportamiento
1. **Inicio**: LED parpadea automáticamente
2. **Primera pulsación**: LED se detiene (apagado)
3. **Segunda pulsación**: LED reanuda el parpadeo
4. **Pulsaciones sucesivas**: Alterna entre parpadeo y apagado

## 📝 Salida por Consola
```
LED state: 1
LED state: 0
Blink DESACTIVADO # Al presionar el botón
Blink ACTIVADO # Al presionar el botón nuevamente
LED state: 1
LED state: 0