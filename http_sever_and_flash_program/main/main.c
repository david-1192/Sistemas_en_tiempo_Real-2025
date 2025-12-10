/**
 * Application entry point.
 */

#include "nvs_flash.h"
//#include "http_server.h"
#include "wifi_app.h"
#include "driver/gpio.h"
#include "globales.h"
#include "sensores/pir_sensor.h"
#include "sensores/ntc.h"
#include "sensores/pwm_led.h"


#define BLINK_GPIO				2

/**
 * @brief Configura el LED GPIO
 * 
 */
static void configure_led(void)
{
	gpio_reset_pin(BLINK_GPIO);
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Start Wifi and peripherals
	init_obtain_time();
	configure_led();
	wifi_app_start();

	// Inicializar módulos de sensores
	pir_sensor_init(GPIO_PIR, GPIO_LED);
	ntc_init();
	pwm_led_init(18);
}

