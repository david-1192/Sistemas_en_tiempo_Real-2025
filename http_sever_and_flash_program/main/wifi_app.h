/**
 * @file wifi_app.h
 * @brief Interfaz de aplicación WiFi y gestión de registros programados
 * @author David PC
 * @date Octubre 2021 (actualizado: Diciembre 2025)
 * 
 * @details
 * Este módulo proporciona funcionalidades para:
 * - Conexión y gestión de redes WiFi (modo AP y STA)
 * - Almacenamiento y carga de credenciales WiFi en NVS
 * - Sistema de registros programados (scheduling) con soporte para:
 *   * Rangos horarios (hora de inicio y fin)
 *   * Selección de días de la semana
 *   * Persistencia en NVS Flash
 * - Sincronización de tiempo mediante protocolo SNTP
 * - Cola de mensajes para comunicación entre tareas
 * - Funciones de callback para eventos WiFi
 * 
 * @section config Configuración por defecto
 * - SSID AP: ESP32_AP_DAVID
 * - IP AP: 192.168.43.50
 * - Máximo de conexiones AP: 5
 * - Máximo de registros: 10
 * 
 * @section storage Almacenamiento
 * Utiliza Non-Volatile Storage (NVS) para:
 * - Credenciales WiFi (namespace "storage")
 * - Registros programados (namespace "prog_regs_v1")
 * 
 * @see http_server.h para endpoints HTTP relacionados
 */

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_netif.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/** @defgroup AP_CONFIG Configuración del Punto de Acceso WiFi */
/** @{ */
#define WIFI_AP_SSID				"ESP32_AP_DAVID"    ///< SSID del punto de acceso
#define WIFI_AP_PASSWORD			"password"           ///< Contraseña del punto de acceso
#define WIFI_AP_CHANNEL				1                    ///< Canal WiFi (1-13)
#define WIFI_AP_SSID_HIDDEN			0                    ///< 0=visible, 1=oculto
#define WIFI_AP_MAX_CONNECTIONS		5                    ///< Máximo de clientes simultáneos
#define WIFI_AP_BEACON_INTERVAL		100                  ///< Intervalo beacon en ms
#define WIFI_AP_IP					"192.168.43.50"      ///< Dirección IP del AP
#define WIFI_AP_GATEWAY				"192.168.43.50"      ///< Gateway del AP
#define WIFI_AP_NETMASK				"255.255.255.0"      ///< Máscara de red
#define WIFI_AP_BANDWIDTH			WIFI_BW_HT20         ///< Ancho de banda: 20 MHz
#define WIFI_STA_POWER_SAVE			WIFI_PS_NONE         ///< Sin ahorro de energía en STA
/** @} */

/** @defgroup WIFI_LIMITS Límites y restricciones */
/** @{ */
#define MAX_SSID_LENGTH				32                   ///< Longitud máxima del SSID
#define MAX_PASSWORD_LENGTH			64                   ///< Longitud máxima de contraseña
#define MAX_CONNECTION_RETRIES		5                    ///< Reintentos de conexión
#define NUM_REGISTERS_AV 			10                   ///< Número total de registros disponibles
/** @} */

/** @defgroup WIFI_NETIF Interfaces de red WiFi */
/** @{ */
extern esp_netif_t* esp_netif_sta;  ///< Interfaz de estación (cliente) WiFi
extern esp_netif_t* esp_netif_ap;   ///< Interfaz de punto de acceso WiFi
/** @} */

/**
 * @enum wifi_app_message
 * @brief Identificadores de mensajes para la cola de eventos WiFi
 * 
 * @details
 * Estos mensajes se envían a través de una cola FreeRTOS
 * para comunicación entre tareas sin bloqueos
 */
typedef enum wifi_app_message
{
    WIFI_APP_MSG_START_HTTP_SERVER = 0,      ///< Inicia el servidor HTTP
    WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER, ///< Intenta conectar (solicitud desde HTTP)
    WIFI_APP_MSG_STA_CONNECTED_GOT_IP,        ///< Conectado a AP y obtuve dirección IP
    WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT, ///< Usuario solicita desconexión
    WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS,      ///< Cargar credenciales guardadas
    WIFI_APP_MSG_STA_DISCONNECTED,            ///< Desconectado de AP
    WIFI_APP_CONNECT_TO_STA,                  ///< Conectar al modo estación
} wifi_app_message_e;

/**
 * @struct wifi_app_queue_message
 * @brief Estructura de mensaje para la cola de eventos
 * 
 * @details
 * Contenedor simple para pasar mensajes entre tareas.
 * Se utiliza con xQueueSend() y xQueueReceive() de FreeRTOS.
 */
typedef struct wifi_app_queue_message
{
    wifi_app_message_e msgID;  ///< ID del mensaje
} wifi_app_queue_message_t;

/**
 * @struct program_register_t
 * @brief Registro de programa horario para ejecución automática
 * 
 * @details
 * Define un horario de trabajo con:
 * - Hora y minuto de inicio
 * - Hora y minuto de finalización
 * - Máscara de bits para seleccionar días de la semana
 * 
 * @note
 * Los rangos horarios pueden cruzar medianoche
 * (por ejemplo: 22:00 a 06:00 es válido)
 * 
 * @example
 * // Programa de lunes a viernes, 08:30 a 17:45
 * program_register_t schedule = {
 *     .start_hour = 8,
 *     .start_min = 30,
 *     .end_hour = 17,
 *     .end_min = 45,
 *     .weekdays = 0b0011111  // bit0=Mon, bit4=Fri, bit6=Sun
 * };
 */
typedef struct program_register_t
{
    uint8_t start_hour;   ///< Hora de inicio (0-23)
    uint8_t start_min;    ///< Minuto de inicio (0-59)
    uint8_t end_hour;     ///< Hora de fin (0-23)
    uint8_t end_min;      ///< Minuto de fin (0-59)
    uint8_t weekdays;     ///< Bitmask: bit0=Lunes ... bit6=Domingo
} program_register_t;

/** @defgroup NVS_CONFIG Configuración de almacenamiento NVS */
/** @{ */
/**
 * @def PROGRAM_NVS_NAMESPACE
 * @brief Espacio de nombres NVS para almacenar registros programados
 * 
 * @details
 * Cada registro se guarda como clave "reg0" a "reg9" dentro de este namespace.
 * La versión en el nombre asegura compatibilidad hacia adelante.
 */
#define PROGRAM_NVS_NAMESPACE "prog_regs_v1"
/** @} */

/** @defgroup PROGRAM_API API de Gestión de Registros Programados */
/** @{ */

/**
 * @brief Establece el registro programado activo/seleccionado
 * 
 * @param reg Índice del registro a activar:
 *            - Valores válidos: 0 a (NUM_REGISTERS_AV-1)
 *            - Valor especial: 0xFF para deseleccionar
 * 
 * @details
 * - Persiste la selección en NVS
 * - Solo se ejecutarán las acciones del registro seleccionado
 * - Los cambios se aplican inmediatamente
 * 
 * @note
 * El registro seleccionado debe estar guardado previamente
 * con save_program_register() para tener datos válidos.
 * 
 * @see wifi_app_get_selected_register()
 * @see save_program_register()
 */
void wifi_app_set_selected_register(uint8_t reg);

/**
 * @brief Obtiene el registro programado activo/seleccionado
 * 
 * @return Índice del registro activo:
 *         - Valores 0 a (NUM_REGISTERS_AV-1): índice del registro
 *         - 0xFF: ningún registro está activo
 * 
 * @details
 * Retorna el registro almacenado en la variable estática interna.
 * Se carga al inicializar desde NVS.
 * 
 * @see wifi_app_set_selected_register()
 * @see initialize_registers()
 */
uint8_t wifi_app_get_selected_register(void);

/**
 * @brief Verifica si el registro activo debe ejecutarse en este momento
 * 
 * @return true si:
 *         - Existe un registro seleccionado
 *         - El tiempo está sincronizado (SNTP)
 *         - Hoy es un día incluido en el registro
 *         - La hora actual está dentro del rango horario
 *         false en cualquier otro caso
 * 
 * @details
 * Realiza validación secuencial:
 * 1. Comprueba sincronización de tiempo
 * 2. Valida que exista registro seleccionado (no 0xFF)
 * 3. Extrae el día actual y lo compara con bitmask weekdays
 * 4. Calcula minutos desde medianoche para comparación
 * 5. Soporta rangos que cruzan medianoche
 * 
 * @example
 * // Si es martes 14:30 y el registro es lunes-viernes 09:00-18:00
 * if (wifi_app_is_selected_register_active()) {
 *     // El registro está activo ahora
 *     activate_motor();
 * }
 * 
 * @note
 * Requiere que get_state_time_was_synchronized() retorne true.
 * Las comparaciones de hora se hacen en zona horaria local.
 * 
 * @see get_state_time_was_synchronized()
 * @see wifi_app_get_selected_register()
 */
bool wifi_app_is_selected_register_active(void);

/**
 * @brief Guarda un registro programado en NVS (almacenamiento persistente)
 * 
 * @param idx Índice del registro (0 a NUM_REGISTERS_AV-1)
 * @param reg Puntero a estructura program_register_t con los datos
 * 
 * @return ESP_OK si se guardó exitosamente
 * @return ESP_ERR_INVALID_ARG si idx o reg son inválidos
 * @return Otro código de error si falla NVS
 * 
 * @details
 * Operación segura en NVS:
 * 1. Abre namespace PROGRAM_NVS_NAMESPACE en modo READWRITE
 * 2. Guarda como blob binario con clave "reg0", "reg1", etc.
 * 3. Realiza commit() para persistencia
 * 4. Cierra el manejador NVS
 * 5. Actualiza la copia en memoria s_program_regs[idx]
 * 
 * @note
 * - El tamaño guardado es exactamente sizeof(program_register_t)
 * - No modifica s_selected_register
 * - Es thread-safe a nivel de NVS
 * 
 * @see load_program_register()
 * @see initialize_registers()
 */
esp_err_t save_program_register(uint8_t idx, const program_register_t *reg);

/**
 * @brief Carga un registro programado desde NVS
 * 
 * @param idx Índice del registro a cargar (0 a NUM_REGISTERS_AV-1)
 * @param[out] reg Puntero a buffer donde guardar el registro
 * 
 * @return ESP_OK si se cargó exitosamente
 * @return ESP_ERR_INVALID_ARG si idx o reg son inválidos
 * @return ESP_ERR_NVS_NOT_FOUND si el registro no existe en NVS
 * @return Otro código de error si falla NVS
 * 
 * @details
 * Operación segura en NVS:
 * 1. Abre namespace PROGRAM_NVS_NAMESPACE en modo READONLY
 * 2. Lee blob con clave "reg0", "reg1", etc.
 * 3. Valida que el tamaño sea exactamente sizeof(program_register_t)
 * 4. Cierra el manejador NVS
 * 5. Actualiza s_program_regs[idx] en memoria
 * 
 * @note
 * - No modifica reg si el registro no existe
 * - Es thread-safe a nivel de NVS
 * - Si el registro no existía, retorna ESP_ERR_NVS_NOT_FOUND
 * 
 * @see save_program_register()
 * @see initialize_registers()
 */
esp_err_t load_program_register(uint8_t idx, program_register_t *reg);

/**
 * @brief Inicializa y carga todos los registros programados desde NVS
 * 
 * @details
 * Secuencia de inicialización:
 * 1. Llama nvs_flash_init() para inicializar NVS
 * 2. Abre namespace PROGRAM_NVS_NAMESPACE
 * 3. Carga el registro seleccionado previo (clave "sel")
 * 4. Itera sobre todos los registros (0 a NUM_REGISTERS_AV-1)
 * 5. Carga cada registro o inicializa con ceros si no existe
 * 6. Actualiza s_selected_register y array s_program_regs
 * 
 * @return Ninguno (void)
 * 
 * @note
 * - Debe llamarse una sola vez al inicio de la aplicación
 * - Típicamente se llama desde wifi_app_task()
 * - Es seguro llamarla múltiples veces (idempotente)
 * - Los registros no existentes se inicializan con ceros
 * 
 * @see wifi_app_set_selected_register()
 * @see save_program_register()
 * @see load_program_register()
 */
void initialize_registers(void);

/** @} */

/** @defgroup WIFI_CORE_API API Core de WiFi */
/** @{ */

/**
 * @brief Inicia la aplicación WiFi y todas sus tareas asociadas
 * 
 * @details
 * Realiza inicializaciones:
 * 1. Configura logging del módulo WiFi
 * 2. Asigna memoria para configuración WiFi
 * 3. Crea cola de mensajes para eventos
 * 4. Crea semáforo binario para sincronización
 * 5. Crea tarea principal wifi_app_task
 * 6. Crea tarea de monitoreo check_sta_connection_state
 * 7. Crea tarea de scheduling task_compare_hour_to_execute_action
 * 
 * @note
 * - Debe llamarse una sola vez desde app_main()
 * - No es bloqueante
 * - Utiliza dinamicamente FreeRTOS para crear tareas
 * 
 * @see wifi_app_task()
 */
void wifi_app_start(void);

/**
 * @brief Conecta el ESP32 a una red WiFi configurada como STA
 * 
 * @details
 * Secuencia de conexión:
 * 1. Toma semáforo mySemaphore para exclusión mutua
 * 2. Carga SSID y contraseña desde NVS
 * 3. Obtiene estructura wifi_config actual
 * 4. Configura campos SSID y password en wifi_config
 * 5. Envía configuración al driver WiFi
 * 6. Inicia conexión (mediante wifi_app_connect_sta)
 * 7. Libera semáforo
 * 
 * @note
 * - Es thread-safe mediante semáforo
 * - Las credenciales deben existir (save_wifi_credentials)
 * - No reinicia el driver WiFi, solo cambia configuración
 * 
 * @see save_wifi_credentials()
 * @see load_wifi_credentials()
 * @see wifi_app_connect_sta()
 */
void connect_to_wifi(void);

/**
 * @brief Inicializa el estado de sincronización de tiempo
 * 
 * @details
 * Establece time_was_synchronized = false
 * Se llama típicamente al inicio antes de obtener hora SNTP
 * 
 * @see obtain_time()
 * @see get_state_time_was_synchronized()
 */
void init_obtain_time(void);

/**
 * @brief Obtiene el estado actual de sincronización de tiempo SNTP
 * 
 * @return true si el tiempo está sincronizado con servidor NTP
 * @return false si aún no se ha obtenido la hora correcta
 * 
 * @details
 * Variable booleana que indica si la hora del sistema es válida.
 * Se establece a true por obtain_time() cuando logra sincronizar.
 * Usada por otros módulos para validar hora antes de usar timers.
 * 
 * @see obtain_time()
 * @see init_obtain_time()
 */
bool get_state_time_was_synchronized(void);

/**
 * @brief Guarda las credenciales WiFi en almacenamiento NVS
 * 
 * @param ssid SSID de la red WiFi (máximo 32 caracteres)
 * @param password Contraseña de la red (máximo 64 caracteres)
 * 
 * @details
 * Almacenamiento persistente:
 * 1. Abre namespace "storage" en modo READWRITE
 * 2. Guarda SSID como string con clave "wifi_ssid"
 * 3. Guarda password como string con clave "wifi_password"
 * 4. Realiza commit para persistencia
 * 5. Cierra manejador NVS
 * 
 * @note
 * - Las credenciales permanecen almacenadas tras reinicio
 * - Usa ESP_ERROR_CHECK, aborta si falla
 * - Llamado típicamente desde página web HTTP
 * 
 * @see load_wifi_credentials()
 * @see nvs_credentials_exist()
 */
void save_wifi_credentials(const char *ssid, const char *password);

/**
 * @brief Carga las credenciales WiFi guardadas desde NVS
 * 
 * @param[out] ssid Buffer para SSID (mínimo 32 bytes)
 * @param[out] password Buffer para contraseña (mínimo 64 bytes)
 * 
 * @details
 * Recuperación de almacenamiento:
 * 1. Abre namespace "storage" en modo READONLY
 * 2. Obtiene tamaño requerido para SSID
 * 3. Asigna memoria y carga SSID
 * 4. Obtiene tamaño requerido para password
 * 5. Asigna memoria y carga password
 * 6. Copia datos a buffers de salida
 * 7. Libera memoria temporal
 * 8. Cierra manejador NVS
 * 
 * @note
 * - Las credenciales deben existir (se asumen existentes)
 * - Usa ESP_ERROR_CHECK, aborta si no existen
 * - Los buffers deben ser suficientemente grandes
 * 
 * @see save_wifi_credentials()
 * @see nvs_credentials_exist()
 */
void load_wifi_credentials(char *ssid, char *password);

/** @} */

/** @defgroup TASK_FUNCTIONS Funciones de tareas FreeRTOS */
/** @{ */

/**
 * @brief Obtiene hora del servidor NTP y sincroniza sistema
 * 
 * @details
 * Sincronización de tiempo:
 * 1. Establece zona horaria a EST5EDT
 * 2. Configura servidor SNTP (pool.ntp.org)
 * 3. Espera hasta 10 intentos a obtener hora válida
 * 4. Valida que año > 2016
 * 5. Actualiza time_was_synchronized
 * 
 * @note
 * - Función interna, marcada como static
 * - Bloqueante: puede tomar hasta 20 segundos
 * - Se llama desde check_sta_connection_state
 * - Requiere conexión WiFi activa
 * 
 * @see get_state_time_was_synchronized()
 */
static void obtain_time(void);

/**
 * @brief Gestiona la conexión a punto de acceso WiFi
 * 
 * @param[in] pvParameters Parámetros de FreeRTOS (no utilizado)
 * 
 * @details
 * Monitoreo continuo cada 20 segundos:
 * 1. Obtiene información del AP actual
 * 2. Si está conectado:
 *    - Registra conexión exitosa
 *    - Obtiene hora si no está sincronizada
 * 3. Si no está conectado:
 *    - Verifica si existen credenciales guardadas
 *    - Reintentas conexión si las credenciales existen
 *    - Registra fallo de conexión
 * 
 * @note
 * - Tarea de FreeRTOS, bucle infinito
 * - Crea automáticamente al recibir WIFI_APP_CONNECT_TO_STA
 * - No se termina, solo cuando device reinicia
 * 
 * @see connect_to_wifi()
 * @see nvs_credentials_exist()
 */
void check_sta_connection_state(void *pvParameters);

/**
 * @brief Compara hora actual con horario de un registro programado
 * 
 * @param[in] timeinfo Hora actual del sistema (struct tm)
 * @param[in] aux_reg Registro programado a comparar
 * 
 * @return true si la hora/día actual coinciden con el horario del registro
 * @return false si no coinciden
 * 
 * @details
 * Validaciones realizadas:
 * 1. Convierte tm_wday (0=Dom...6=Sab) a bitmask (0=Lun...6=Dom)
 * 2. Valida que el bit del día esté activo en weekdays
 * 3. Verifica que hora actual = start_hour
 * 4. Verifica que minuto actual = start_min
 * 5. Si coinciden:
 *    - Llama a toogle_led() para activar motor
 *    - Espera 40 segundos
 *    - Retorna true
 * 
 * @note
 * - Solo detecta el momento exacto de inicio (no rango completo)
 * - Usa logging para debugging
 * - Soporta mapeo de días de semana
 * 
 * @see task_compare_hour_to_execute_action()
 */
bool compare_hour_day_structs(struct tm timeinfo, program_register_t aux_reg);

/**
 * @brief Tarea que monitorea y ejecuta registros programados
 * 
 * @param[in] pvParameters Parámetros de FreeRTOS (no utilizado)
 * 
 * @details
 * Comportamiento:
 * 1. Espera sincronización de tiempo (get_state_time_was_synchronized)
 * 2. Bucle infinito cada 30 segundos:
 *    a. Obtiene hora actual del sistema
 *    b. Itera sobre todos los registros (0 a NUM_REGISTERS_AV-1)
 *    c. Compara hora/día con cada registro
 *    d. Ejecuta acciones si hay coincidencia
 * 3. Registra logs con información de comparación
 * 
 * @note
 * - Tarea de prioridad baja (5)
 * - Creada en núcleo 1 del ESP32
 * - No requiere credenciales ni conexión WiFi
 * - Funciona incluso en modo AP local
 * 
 * @see compare_hour_day_structs()
 * @see get_state_time_was_synchronized()
 */
void task_compare_hour_to_execute_action(void *pvParameters);

/**
 * @brief Actualiza datos de registro desde NVS a memoria
 * 
 * @param[in] reg_to_update Número de registro a actualizar (1-10)
 * 
 * @details
 * Proceso de actualización:
 * 1. Lee datos del registro en NVS (formato string)
 * 2. Parsea campos individuales:
 *    - Bytes 0-1: hora inicio
 *    - Bytes 2-3: minuto inicio
 *    - Bytes 4-5: hora fin
 *    - Bytes 6-7: minuto fin
 *    - Bytes 8-14: bits de días (uno por carácter)
 * 3. Convierte strings a valores numéricos
 * 4. Construye bitmask de weekdays
 * 5. Actualiza estructura s_program_regs[reg_to_update-1]
 * 6. Registra valores en log
 * 
 * @note
 * - Índice es 1-based (1-10), se convierte a 0-based internamente
 * - Formato string: "HHMMhhmmDDDDDDD"
 * - Mapea bits de días: Mon=bit0, Tue=bit1, ..., Sun=bit6
 * 
 * @see read_reg_data()
 * @see save_reg_data()
 */
void update_register(int reg_to_update);

/** @} */

/** @defgroup DATA_API API de manejo de datos NVS (legacy) */
/** @{ */

/**
 * @brief Guarda datos de registro en formato string (legacy)
 * 
 * @param[in] register Número de registro (1-10)
 * @param[in] str Datos en formato string
 * 
 * @details
 * Almacenamiento legacy:
 * 1. Abre namespace "storage"
 * 2. Guarda string con clave "reg01"..."reg10"
 * 3. Commit para persistencia
 * 
 * @note
 * - Función legacy, mantenida por compatibilidad
 * - Formato: "HHMMhhmmDDDDDDD"
 * - Preferir save_program_register() para nuevas implementaciones
 * 
 * @see read_reg_data()
 * @see update_register()
 */
void save_reg_data(uint8_t register, char *str);

/** @} */

/** @defgroup QUEUE_API API de cola de mensajes */
/** @{ */

/**
 * @brief Envía un mensaje a la cola de eventos de la aplicación WiFi
 * 
 * @param[in] msgID ID del mensaje a enviar
 * 
 * @return pdTRUE si se envió correctamente
 * @return pdFALSE si la cola está llena
 * 
 * @details
 * Comunicación entre tareas:
 * 1. Empaqueta msgID en estructura wifi_app_queue_message_t
 * 2. Envía a cola wifi_app_queue_handle
 * 3. Espera indefinidamente si cola está llena
 * 4. Retorna cuando se envía el mensaje
 * 
 * @note
 * - Es thread-safe
 * - Tamaño de cola: 3 mensajes
 * - Usar desde interrupt handlers con cuidado
 * 
 * @see wifi_app_message_e
 * @see wifi_app_task()
 */
BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

/** @} */

/** @defgroup CONFIG_API API de configuración WiFi */
/** @{ */

/**
 * @brief Obtiene la estructura de configuración WiFi actual
 * 
 * @return Puntero a estructura wifi_config_t
 * 
 * @details
 * Retorna el puntero asignado en wifi_app_start().
 * Se modifica típicamente antes de llamar esp_wifi_set_config().
 * 
 * @note
 * - No es NULL-safe
 * - Estructura compartida entre STA y AP
 * - Modificaciones se aplican con esp_wifi_set_config()
 * 
 * @see connect_to_wifi()
 * @see wifi_app_start()
 */
wifi_config_t* wifi_app_get_wifi_config(void);

/** @} */

/** @defgroup CALLBACK_API API de Callbacks de eventos */
/** @{ */

/**
 * @brief Registra callback de evento WiFi
 * 
 * @param[in] cb Puntero a función callback
 * 
 * @details
 * Se llamará cuando ocurran eventos importantes de WiFi.
 * Típicamente usado para notificar a otros módulos.
 * 
 * @note Función disponible, sin uso actual en el proyecto
 */
void wifi_app_set_callback(void (*cb)(void));

/** @} */

#endif // MAIN_WIFI_APP_H_




























