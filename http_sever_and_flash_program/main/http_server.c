/*
 * http_server.c
 *
 *  Created on: Oct 20, 2021
 *      Author: kjagu
 */

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "sys/param.h"
#include <stdlib.h>

#include "http_server.h"
#include "tasks_common.h"
#include "wifi_app.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "globales.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_sntp.h"

// Tag used for ESP serial console messages
static const char TAG[] = "http_server";
static const char *TAG_HTTP_PROG = "http_prog";

// Wifi connect status
static int g_wifi_connect_status = NONE;

// Firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;

// HTTP server task handle
static httpd_handle_t http_server_handle = NULL;

// HTTP server monitor task handle
static TaskHandle_t task_http_server_monitor = NULL;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle;

/**
 * ESP32 timer configuration passed to esp_timer_create.
 */
const esp_timer_create_args_t fw_update_reset_args = {
	.callback = &http_server_fw_update_reset_callback,
	.arg = NULL,
	.dispatch_method = ESP_TIMER_TASK,
	.name = "fw_update_reset"};
esp_timer_handle_t fw_update_reset;

// Embedded files: JQuery, index.html, app.css, app.js and favicon.ico files
extern const uint8_t jquery_3_3_1_min_js_start[] asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[] asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t app_css_start[] asm("_binary_app_css_start");
extern const uint8_t app_css_end[] asm("_binary_app_css_end");
extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

uint8_t s_led_state = 0;

void toogle_led(void)
{

	s_led_state = !s_led_state;
	gpio_set_level(BLINK_GPIO, s_led_state);
}

static esp_err_t http_server_mode_handler(httpd_req_t *req);
static esp_err_t http_server_manual_pwm_handler(httpd_req_t *req);
static esp_err_t http_server_get_program_registers_handler(httpd_req_t *req);
static esp_err_t http_server_post_program_register_handler(httpd_req_t *req);
static esp_err_t http_server_select_register_handler(httpd_req_t *req);

// Forward declaration for read_reg_data
extern esp_err_t read_reg_data(char *data, int reg_num);

/**
 * @brief Handler for /dhtSensor.json endpoint
 */
static esp_err_t http_server_get_dht_sensor_readings_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/dhtSensor.json requested");

	char dhtSensorJSON[100];

	sprintf(dhtSensorJSON, "{\"temp\":\"%.1f\",\"humidity\":\"%.1f\"}", 30.1, 40.5);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, dhtSensorJSON, strlen(dhtSensorJSON));

	return ESP_OK;
}

/**
 * @brief Handler for /toogle_led.json endpoint
 */
static esp_err_t http_server_togle_led_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/toogle_led.json requested");

	toogle_led();

	// Cerrar la conexion
	httpd_resp_set_hdr(req, "Connection", "close");
	httpd_resp_send(req, NULL, 0);

	return ESP_OK;
}

/**
 * @brief Handler for /readreg.json endpoint
 */
static esp_err_t http_server_read_register_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/readreg.json requested");

	char read_regs[255];
	char register_information_read_1[16];
	register_information_read_1[15] = 0x00;
	if (read_reg_data(&register_information_read_1[0], 1) != ESP_OK)
	{
		memset(&register_information_read_1[0], '9', 15);
	}

	char register_information_read_2[16];
	register_information_read_2[15] = 0x00;
	if (read_reg_data(&register_information_read_2[0], 2) != ESP_OK)
	{
		memset(&register_information_read_2[0], '9', 15);
	}

	char register_information_read_3[16];
	register_information_read_3[15] = 0x00;
	if (read_reg_data(&register_information_read_3[0], 3) != ESP_OK)
	{
		memset(&register_information_read_3[0], '9', 15);
	}

	char register_information_read_4[16];
	register_information_read_4[15] = 0x00;
	if (read_reg_data(&register_information_read_4[0], 4) != ESP_OK)
	{
		memset(&register_information_read_4[0], '9', 15);
	}

	char register_information_read_5[16];
	register_information_read_5[15] = 0x00;
	if (read_reg_data(&register_information_read_5[0], 5) != ESP_OK)
	{
		memset(&register_information_read_5[0], '9', 15);
	}

	sprintf(read_regs,
			"{\"reg1\":\"%s\",\"reg2\":\"%s\",\"reg3\":\"%s\",\"reg4\":\"%s\",\"reg5\":\"%s\"}",
			(char *)register_information_read_1, (char *)register_information_read_2, (char *)register_information_read_3, (char *)register_information_read_4,
			(char *)register_information_read_5);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, read_regs, strlen(read_regs));

	return ESP_OK;
}

static void log_error_if_nonzero(const char *message, int error_code)
{
	if (error_code != 0)
	{
		ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
	}
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	// ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;
	switch ((esp_mqtt_event_id_t)event_id)
	{
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		// msg_id = esp_mqtt_client_publish(client, "test", "hola", 0, 1, 0);
		// ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_subscribe(client, "test", 1);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		// msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
		// ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		// msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
		// ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		// msg_id = esp_mqtt_client_publish(client, "test", "hola", 0, 1, 0);
		// ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
		printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
		printf("DATA=%.*s\r\n", event->data_len, event->data);
		if (strncmp(event->data, "toggle", event->data_len) == 0)
		{
			printf("toogle LED received");
			toogle_led();
		}

		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
		{
			log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
			log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
			log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
			ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
		}
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}
}

static void mqtt_app_start(void)
{
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = "mqtt://victor:banano@ec2-35-93-50-123.us-west-2.compute.amazonaws.com",
		.session.keepalive = 15,
		.network.reconnect_timeout_ms = 50,
		//.network.refresh_connection_after_ms,
		.task.priority = 5,
		.task.stack_size = 4096,
	};

#if CONFIG_BROKER_URL_FROM_STDIN
	char line[128];

	if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0)
	{
		int count = 0;
		printf("Please enter url of mqtt broker\n");
		while (count < 128)
		{
			int c = fgetc(stdin);
			if (c == '\n')
			{
				line[count] = '\0';
				break;
			}
			else if (c > 0 && c < 127)
			{
				line[count] = c;
				++count;
			}
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
		mqtt_cfg.broker.address.uri = line;
		printf("Broker url: %s\n", line);
	}
	else
	{
		ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
		abort();
	}
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	/* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
	esp_mqtt_client_start(client);
}

/**
 * Checks the g_fw_update_status and creates the fw_update_reset timer if g_fw_update_status is true.
 */
static void http_server_fw_update_reset_timer(void)
{
	if (g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
	{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW updated successful starting FW update reset timer");

		// Give the web page a chance to receive an acknowledge back and initialize the timer
		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
	}
	else
	{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}

/**
 * HTTP server monitor task used to track events of the HTTP server
 * @param pvParameters parameter which can be passed to the task.
 */
static void http_server_monitor(void *parameter)
{
	http_server_queue_message_t msg;

	for (;;)
	{
		if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
		{
			switch (msg.msgID)
			{
			case HTTP_MSG_WIFI_CONNECT_INIT:
				ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");

				g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECTING;

				break;

			case HTTP_MSG_WIFI_CONNECT_SUCCESS:
				ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");

				g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_SUCCESS;
				// mqtt_app_start(); dissable MQTT
				break;

			case HTTP_MSG_WIFI_CONNECT_FAIL:
				ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");

				g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_FAILED;

				break;

			case HTTP_MSG_OTA_UPDATE_SUCCESSFUL:
				ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
				g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
				http_server_fw_update_reset_timer();

				break;

			case HTTP_MSG_OTA_UPDATE_FAILED:
				ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAILED");
				g_fw_update_status = OTA_UPDATE_FAILED;

				break;

			default:
				break;
			}
		}
	}
}

/**
 * Jquery get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "Jquery requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);

	return ESP_OK;
}

/**
 * Sends the index.html page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "index.html requested");

	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}

/**
 * app.css get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "app.css requested");

	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);

	return ESP_OK;
}

/**
 * app.js get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "app.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);

	return ESP_OK;
}

/**
 * Sends the .ico (icon) file when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "favicon.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}

/**
 * Receives the .bin file fia the web page and handles the firmware update
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK, otherwise ESP_FAIL if timeout occurs and the update cannot be started.
 */
esp_err_t http_server_OTA_update_handler(httpd_req_t *req)
{
	esp_ota_handle_t ota_handle;

	char ota_buff[1024];
	int content_length = req->content_len;
	int content_received = 0;
	int recv_len;
	bool is_req_body_started = false;
	bool flash_successful = false;

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	do
	{
		// Read the data for the request
		if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
		{
			// Check if timeout occurred
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
			{
				ESP_LOGI(TAG, "http_server_OTA_update_handler: Socket Timeout");
				continue; ///> Retry receiving if timeout occurred
			}
			ESP_LOGI(TAG, "http_server_OTA_update_handler: OTA other Error %d", recv_len);
			return ESP_FAIL;
		}
		printf("http_server_OTA_update_handler: OTA RX: %d of %d\r", content_received, content_length);

		// Is this the first data we are receiving
		// If so, it will have the information in the header that we need.
		if (!is_req_body_started)
		{
			is_req_body_started = true;

			// Get the location of the .bin file content (remove the web form data)
			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
			int body_part_len = recv_len - (body_start_p - ota_buff);

			printf("http_server_OTA_update_handler: OTA file size: %d\r\n", content_length);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
			if (err != ESP_OK)
			{
				printf("http_server_OTA_update_handler: Error with OTA begin, cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				printf("http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%lx\r\n", update_partition->subtype, update_partition->address);
			}

			// Write this first part of the data
			esp_ota_write(ota_handle, body_start_p, body_part_len);
			content_received += body_part_len;
		}
		else
		{
			// Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_len);
			content_received += recv_len;
		}

	} while (recv_len > 0 && content_received < content_length);

	if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
			ESP_LOGI(TAG, "http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%lx", boot_partition->subtype, boot_partition->address);
			flash_successful = true;
		}
		else
		{
			ESP_LOGI(TAG, "http_server_OTA_update_handler: FLASHED ERROR!!!");
		}
	}
	else
	{
		ESP_LOGI(TAG, "http_server_OTA_update_handler: esp_ota_end ERROR!!!");
	}

	// We won't update the global variables throughout the file, so send the message about the status
	if (flash_successful)
	{
		http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFUL);
	}
	else
	{
		http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED);
	}

	return ESP_OK;
}

/**
 * OTA status handler responds with the firmware update status after the OTA update is started
 * and responds with the compile time/date when the page is first requested
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
esp_err_t http_server_OTA_status_handler(httpd_req_t *req)
{
	char otaJSON[100];

	ESP_LOGI(TAG, "OTAstatus requested");

	sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", g_fw_update_status, __TIME__, __DATE__);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, otaJSON, strlen(otaJSON));

	return ESP_OK;
}

/**
 * regchange.json handler is invoked after the "enviar registro" button is pressed
 * and handles receiving the data entered by the user
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */

static esp_err_t http_server_register_change_handler(httpd_req_t *req)
{
	size_t header_len;
	char *header_value;
	char *hour_str = NULL;
	char *reg_str = NULL;
	char *min_str = NULL;
	char *days = NULL;
	int content_length;

	ESP_LOGI(TAG, "/regchange.json requested");

	// Get the "Content-Length" header to determine the length of the request body
	header_len = httpd_req_get_hdr_value_len(req, "Content-Length");
	if (header_len <= 0)
	{
		// Content-Length header not found or invalid
		// httpd_resp_send_err(req, HTTP_STATUS_411_LENGTH_REQUIRED, "Content-Length header is missing or invalid");
		ESP_LOGI(TAG, "Content-Length header is missing or invalid");
		return ESP_FAIL;
	}

	// Allocate memory to store the header value
	header_value = (char *)malloc(header_len + 1);
	if (httpd_req_get_hdr_value_str(req, "Content-Length", header_value, header_len + 1) != ESP_OK)
	{
		// Failed to get Content-Length header value
		free(header_value);
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Failed to get Content-Length header value");
		ESP_LOGI(TAG, "Failed to get Content-Length header value");
		return ESP_FAIL;
	}

	// Convert the Content-Length header value to an integer
	content_length = atoi(header_value);
	free(header_value);

	if (content_length <= 0)
	{
		// Content length is not a valid positive integer
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid Content-Length value");
		ESP_LOGI(TAG, "Invalid Content-Length value");
		return ESP_FAIL;
	}

	// Allocate memory for the data buffer based on the content length
	char *data_buffer = (char *)malloc(content_length + 1);

	// Read the request body into the data buffer
	if (httpd_req_recv(req, data_buffer, content_length) <= 0)
	{
		// Handle error while receiving data
		free(data_buffer);
		// httpd_resp_send_err(req, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Failed to receive request body");
		ESP_LOGI(TAG, "Failed to receive request body");
		return ESP_FAIL;
	}

	// Null-terminate the data buffer to treat it as a string
	data_buffer[content_length] = '\0';

	// Parse the received JSON data
	cJSON *root = cJSON_Parse(data_buffer);
	free(data_buffer);

	if (root == NULL)
	{
		// JSON parsing error
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid JSON data");
		ESP_LOGI(TAG, "Invalid JSON data");
		return ESP_FAIL;
	}

	cJSON *reg_number_json = cJSON_GetObjectItem(root, "selectedNumber");
	cJSON *hour_json = cJSON_GetObjectItem(root, "hours");
	cJSON *min_json = cJSON_GetObjectItem(root, "minutes");
	cJSON *selectedDays_json = cJSON_GetObjectItem(root, "selectedDays");

	if (reg_number_json == NULL || hour_json == NULL || min_json == NULL || selectedDays_json == NULL) {
	    cJSON_Delete(root);
	    ESP_LOGI(TAG, "Missing JSON fields");
	    return ESP_FAIL;
	}

	/* selectedNumber: accept string or number */
	if (cJSON_IsString(reg_number_json)) {
	    reg_str = strdup(reg_number_json->valuestring);
	} else if (cJSON_IsNumber(reg_number_json)) {
	    reg_str = malloc(12);
	    snprintf(reg_str, 12, "%d", reg_number_json->valueint);
	} else {
	    cJSON_Delete(root);
	    ESP_LOGI(TAG, "Invalid type for selectedNumber");
	    return ESP_FAIL;
	}

	/* hours and minutes: accept string or number */
	if (cJSON_IsString(hour_json)) {
	    hour_str = strdup(hour_json->valuestring);
	} else if (cJSON_IsNumber(hour_json)) {
	    hour_str = malloc(4);
	    snprintf(hour_str, 4, "%d", hour_json->valueint);
	} else {
	    free(reg_str);
	    cJSON_Delete(root);
	    ESP_LOGI(TAG, "Invalid hours field");
	    return ESP_FAIL;
	}

	if (cJSON_IsString(min_json)) {
	    min_str = strdup(min_json->valuestring);
	} else if (cJSON_IsNumber(min_json)) {
	    min_str = malloc(4);
	    snprintf(min_str, 4, "%d", min_json->valueint);
	} else {
	    free(reg_str);
	    free(hour_str);
	    cJSON_Delete(root);
	    ESP_LOGI(TAG, "Invalid minutes field");
	    return ESP_FAIL;
	}

	/* If selectedDays is not an array but a single value, wrap it into a temporary array */
	cJSON *tmp_selectedDays = NULL;
	bool created_tmp_array = false;
	if (!cJSON_IsArray(selectedDays_json)) {
	    tmp_selectedDays = cJSON_CreateArray();
	    if (cJSON_IsString(selectedDays_json)) {
	        cJSON_AddItemToArray(tmp_selectedDays, cJSON_CreateString(selectedDays_json->valuestring));
	    } else if (cJSON_IsNumber(selectedDays_json)) {
	        char buf[4];
	        snprintf(buf, sizeof(buf), "%d", selectedDays_json->valueint);
	        cJSON_AddItemToArray(tmp_selectedDays, cJSON_CreateString(buf));
	    } else {
	        free(reg_str);
	        free(hour_str);
	        free(min_str);
	        cJSON_Delete(root);
	        ESP_LOGI(TAG, "Invalid selectedDays field");
	        return ESP_FAIL;
	    }
	    selectedDays_json = tmp_selectedDays;
	    created_tmp_array = true;
	}

	ESP_LOGI(TAG, "Received reg: %s", reg_str);
	ESP_LOGI(TAG, "Received hour: %s", hour_str);
	ESP_LOGI(TAG, "Received min: %s", min_str);

	char str_to_save[16];
	memset(str_to_save, 0x00, 16);

	// Optional end time fields
	cJSON *end_hour_json = cJSON_GetObjectItem(root, "endHours");
	cJSON *end_min_json = cJSON_GetObjectItem(root, "endMinutes");

	int sh = atoi(hour_str);
	int sm = atoi(min_str);
	int eh = sh;
	int em = sm;
	if (end_hour_json && (cJSON_IsNumber(end_hour_json) || cJSON_IsString(end_hour_json))) {
		eh = cJSON_IsNumber(end_hour_json) ? end_hour_json->valueint : atoi(end_hour_json->valuestring);
	}
	if (end_min_json && (cJSON_IsNumber(end_min_json) || cJSON_IsString(end_min_json))) {
		em = cJSON_IsNumber(end_min_json) ? end_min_json->valueint : atoi(end_min_json->valuestring);
	}

	// Build string: startHH startMM endHH endMM days(7)
	// Ensure two-digit zero-padded fields
	char tmp[4];
	snprintf(str_to_save, sizeof(str_to_save), "%02d%02d%02d%02d", sh, sm, eh, em);
	if (cJSON_IsArray(selectedDays_json)) {
		cJSON *day_item;
		cJSON_ArrayForEach(day_item, selectedDays_json) {
			if (cJSON_IsString(day_item)) {
				strncat(str_to_save, day_item->valuestring, 1);
			}
		}
	}
	else
	{
		printf("SelectedDays is not an array\n");
	}
	printf("%s\n", str_to_save);
	save_reg_data(atoi(reg_str), &str_to_save[0]);
	update_register(atoi(reg_str));
	// Process the selected days array
	// cJSON* day_item;
	// cJSON_ArrayForEach(day_item, selectedDays_json) {
	// if (cJSON_IsString(day_item)) {
	//   const char* day_str = day_item->valuestring;
	// ESP_LOGI(TAG, "Selected Day: %s", day_str);

	// Perform any additional actions based on the selected day
	// ...

	// Release memory when no longer needed
	//}

	//}
	// free(day_item);

	// free(selectedDays_json);

	// Send a success response to the client
	// Cerrar la conexion
	free(reg_str);
	free(hour_str);
	free(min_str);
	if (created_tmp_array) {
	    cJSON_Delete(tmp_selectedDays);
	}
	cJSON_Delete(root);
	httpd_resp_set_hdr(req, "Connection", "close");
	httpd_resp_send(req, NULL, 0);

	return ESP_OK;
}

/**
 * erasereg.json handler is invoked after the "enviar registro" button is pressed
 * and handles receiving the data entered by the user
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */

static esp_err_t http_server_register_erase_handler(httpd_req_t *req)
{
	size_t header_len;
	char *header_value;
	char *hour_str = NULL;
	char *reg_str = NULL;
	char *min_str = NULL;
	char *days = NULL;
	int content_length;

	ESP_LOGI(TAG, "/regerase.json requested");

	// Get the "Content-Length" header to determine the length of the request body
	header_len = httpd_req_get_hdr_value_len(req, "Content-Length");
	if (header_len <= 0)
	{
		// Content-Length header not found or invalid
		// httpd_resp_send_err(req, HTTP_STATUS_411_LENGTH_REQUIRED, "Content-Length header is missing or invalid");
		ESP_LOGI(TAG, "Content-Length header is missing or invalid");
		return ESP_FAIL;
	}

	// Allocate memory to store the header value
	header_value = (char *)malloc(header_len + 1);
	if (httpd_req_get_hdr_value_str(req, "Content-Length", header_value, header_len + 1) != ESP_OK)
	{
		// Failed to get Content-Length header value
		free(header_value);
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Failed to get Content-Length header value");
		ESP_LOGI(TAG, "Failed to get Content-Length header value");
		return ESP_FAIL;
	}

	// Convert the Content-Length header value to an integer
	content_length = atoi(header_value);
	free(header_value);

	if (content_length <= 0)
	{
		// Content length is not a valid positive integer
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid Content-Length value");
		ESP_LOGI(TAG, "Invalid Content-Length value");
		return ESP_FAIL;
	}

	// Allocate memory for the data buffer based on the content length
	char *data_buffer = (char *)malloc(content_length + 1);

	// Read the request body into the data buffer
	if (httpd_req_recv(req, data_buffer, content_length) <= 0)
	{
		// Handle error while receiving data
		free(data_buffer);
		// httpd_resp_send_err(req, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Failed to receive request body");
		ESP_LOGI(TAG, "Failed to receive request body");
		return ESP_FAIL;
	}

	// Null-terminate the data buffer to treat it as a string
	data_buffer[content_length] = '\0';

	// Parse the received JSON data
	cJSON *root = cJSON_Parse(data_buffer);
	free(data_buffer);

	if (root == NULL)
	{
		// JSON parsing error
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid JSON data");
		ESP_LOGI(TAG, "Invalid JSON data");
		return ESP_FAIL;
	}

	cJSON *reg_number_json = cJSON_GetObjectItem(root, "selectedNumber");

	if (reg_number_json == NULL || !(cJSON_IsString(reg_number_json) || cJSON_IsNumber(reg_number_json)))
	{
		cJSON_Delete(root);
		// Missing or invalid JSON fields
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Missing or invalid JSON data fields");
		ESP_LOGI(TAG, "Missing or invalid JSON data fields");
		return ESP_FAIL;
	}

	if (cJSON_IsString(reg_number_json))
	{
		reg_str = strdup(reg_number_json->valuestring);
	}
	else /* number */
	{
		reg_str = (char *)malloc(12);
		snprintf(reg_str, 12, "%d", reg_number_json->valueint);
	}

	ESP_LOGI(TAG, "received reg: %s", reg_str);

	char str_to_save[16];
	memset(str_to_save, 0x00, 16);
	// default erased = 99:99 - end 99:99 and all days 0 -> format: HHMMEEFFddddddd
	snprintf(str_to_save, sizeof(str_to_save), "%02d%02d%02d%02d%s", 99, 99, 99, 99, "0000000");
	ESP_LOGI(TAG, "erase -> %s", str_to_save);
	save_reg_data(atoi(reg_str), &str_to_save[0]);
	update_register(atoi(reg_str));
	// Process the selected days array
	// cJSON* day_item;
	// cJSON_ArrayForEach(day_item, selectedDays_json) {
	// if (cJSON_IsString(day_item)) {
	//   const char* day_str = day_item->valuestring;
	// ESP_LOGI(TAG, "Selected Day: %s", day_str);

	// Perform any additional actions based on the selected day
	// ...

	// Release memory when no longer needed
	//}

	//}
	// free(day_item);

	// free(selectedDays_json);

	// Send a success response to the client
	// Cerrar la conexion
	free(reg_str);
	cJSON_Delete(root);
	httpd_resp_set_hdr(req, "Connection", "close");
	httpd_resp_send(req, NULL, 0);

	return ESP_OK;
}

/**
 * wifiConnect.json handler is invoked after the connect button is pressed
 * and handles receiving the SSID and password entered by the user
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */

static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *req)
{
	size_t header_len;
	char *header_value;
	char *ssid_str = NULL;
	char *pass_str = NULL;
	int content_length;

	ESP_LOGI(TAG, "/wifiConnect.json requested");

	// Get the "Content-Length" header to determine the length of the request body
	header_len = httpd_req_get_hdr_value_len(req, "Content-Length");
	if (header_len <= 0)
	{
		// Content-Length header not found or invalid
		// httpd_resp_send_err(req, HTTP_STATUS_411_LENGTH_REQUIRED, "Content-Length header is missing or invalid");
		ESP_LOGI(TAG, "Content-Length header is missing or invalid");
		return ESP_FAIL;
	}

	// Allocate memory to store the header value
	header_value = (char *)malloc(header_len + 1);
	if (httpd_req_get_hdr_value_str(req, "Content-Length", header_value, header_len + 1) != ESP_OK)
	{
		// Failed to get Content-Length header value
		free(header_value);
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Failed to get Content-Length header value");
		ESP_LOGI(TAG, "Failed to get Content-Length header value");
		return ESP_FAIL;
	}

	// Convert the Content-Length header value to an integer
	content_length = atoi(header_value);
	free(header_value);

	if (content_length <= 0)
	{
		// Content length is not a valid positive integer
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid Content-Length value");
		ESP_LOGI(TAG, "Invalid Content-Length value");
		return ESP_FAIL;
	}

	// Allocate memory for the data buffer based on the content length
	char *data_buffer = (char *)malloc(content_length + 1);

	// Read the request body into the data buffer
	if (httpd_req_recv(req, data_buffer, content_length) <= 0)
	{
		// Handle error while receiving data
		free(data_buffer);
		// httpd_resp_send_err(req, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Failed to receive request body");
		ESP_LOGI(TAG, "Failed to receive request body");
		return ESP_FAIL;
	}

	// Null-terminate the data buffer to treat it as a string
	data_buffer[content_length] = '\0';

	// Parse the received JSON data
	cJSON *root = cJSON_Parse(data_buffer);
	free(data_buffer);

	if (root == NULL)
	{
		// JSON parsing error
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid JSON data");
		ESP_LOGI(TAG, "Invalid JSON data");
		return ESP_FAIL;
	}

	cJSON *ssid_json = cJSON_GetObjectItem(root, "selectedSSID");
	cJSON *pwd_json = cJSON_GetObjectItem(root, "pwd");

	if (ssid_json == NULL || pwd_json == NULL || !cJSON_IsString(ssid_json) || !cJSON_IsString(pwd_json))
	{
		cJSON_Delete(root);
		// Missing or invalid JSON fields
		// httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Missing or invalid JSON data fields");
		ESP_LOGI(TAG, "Missing or invalid JSON data fields");
		return ESP_FAIL;
	}

	// Extract SSID and password from JSON
	ssid_str = strdup(ssid_json->valuestring);
	pass_str = strdup(pwd_json->valuestring);

	cJSON_Delete(root);

	// Now, you have the SSID and password in ssid_str and pass_str
	ESP_LOGI(TAG, "Received SSID: %s", ssid_str);
	ESP_LOGI(TAG, "Received Password: %s", pass_str);

	// Update the Wifi networks configuration and let the wifi application know
	wifi_config_t *wifi_config = wifi_app_get_wifi_config();
	memset(wifi_config, 0x00, sizeof(wifi_config_t));
	// memset(wifi_config->sta.ssid, 0x00, sizeof(wifi_config->sta.ssid));
	// memset(wifi_config->sta.password, 0x00, sizeof(wifi_config->sta.password));
	memcpy(wifi_config->sta.ssid, ssid_str, strlen(ssid_str));
	memcpy(wifi_config->sta.password, pass_str, strlen(pass_str));
	save_wifi_credentials(ssid_str, pass_str);
	esp_wifi_disconnect();
	// wifi_app_send_message(WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER); // if doesn't work this need to be checked
	connect_to_wifi();

	free(ssid_str);
	free(pass_str);

	return ESP_OK;
}

/**
 * wifiConnectStatus handler updates the connection status for the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_status_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/wifiConnectStatus requested");

	char statusJSON[100];

	sprintf(statusJSON, "{\"wifi_connect_status\":%d}", g_wifi_connect_status);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, statusJSON, strlen(statusJSON));

	return ESP_OK;
}

/**
 * Handler for sensors status endpoint
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_sensors_status_handler(httpd_req_t *req)
{
    char buf[200];
    float temp = current_temperature;
    int pir = pir_state;
    int mode = current_mode;
    uint32_t duty = current_pwm_duty;
    // convertir duty 0..8191 a porcentaje
    float pct = (duty / 8191.0f) * 100.0f;
    snprintf(buf, sizeof(buf),
        "{\"temperature\":%.2f,\"pir\":%d,\"mode\":%d,\"pwm_pct\":%.1f}",
        temp, pir, mode, pct);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, strlen(buf));
    return ESP_OK;
}

/**
 * Sets up the default httpd server configuration.
 * @return http server instance handle if successful, NULL otherwise.
 */
static httpd_handle_t http_server_configure(void)
{
	// Generate the default configuration
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	// Create the message queue
	http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

	// Create HTTP server monitor task
	xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor, HTTP_SERVER_MONITOR_CORE_ID);

	// The core that the HTTP server will run on
	config.core_id = HTTP_SERVER_TASK_CORE_ID;

	// Adjust the default priority to 1 less than the wifi application task
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;

	// Bump up the stack size (default is 4096)
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

	// Increase uri handlers
	config.max_uri_handlers = 20;

	// Increase the timeout limits
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	ESP_LOGI(TAG,
			 "http_server_configure: Starting server on port: '%d' with task priority: '%d'",
			 config.server_port,
			 config.task_priority);

	// Start the httpd server
	if (httpd_start(&http_server_handle, &config) == ESP_OK)
	{
		ESP_LOGI(TAG, "http_server_configure: Registering URI handlers");

		// register query handler
		httpd_uri_t jquery_js = {
			.uri = "/jquery-3.3.1.min.js",
			.method = HTTP_GET,
			.handler = http_server_jquery_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &jquery_js);

		// register index.html handler
		httpd_uri_t index_html = {
			.uri = "/",
			.method = HTTP_GET,
			.handler = http_server_index_html_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &index_html);

		// register app.css handler
		httpd_uri_t app_css = {
			.uri = "/app.css",
			.method = HTTP_GET,
			.handler = http_server_app_css_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &app_css);

		// register app.js handler
		httpd_uri_t app_js = {
			.uri = "/app.js",
			.method = HTTP_GET,
			.handler = http_server_app_js_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &app_js);

		// register favicon.ico handler
		httpd_uri_t favicon_ico = {
			.uri = "/favicon.ico",
			.method = HTTP_GET,
			.handler = http_server_favicon_ico_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &favicon_ico);

		httpd_uri_t toogle_led = {
			.uri = "/toogle_led.json",
			.method = HTTP_POST,
			.handler = http_server_togle_led_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &toogle_led);

		// register wifiConnect.json handler
		httpd_uri_t wifi_connect_json = {
			.uri = "/wifiConnect.json",
			.method = HTTP_POST,
			.handler = http_server_wifi_connect_json_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_json);

		// register OTAupdate handler
		httpd_uri_t OTA_update = {
			.uri = "/OTAupdate",
			.method = HTTP_POST,
			.handler = http_server_OTA_update_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &OTA_update);

		// register OTAstatus handler
		httpd_uri_t OTA_status = {
			.uri = "/OTAstatus",
			.method = HTTP_POST,
			.handler = http_server_OTA_status_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &OTA_status);

		// register OTAstatus handler
		httpd_uri_t register_change = {
			.uri = "/regchange.json",
			.method = HTTP_POST,
			.handler = http_server_register_change_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &register_change);

		// register erase handler
		httpd_uri_t register_erase = {
			.uri = "/regerase.json",
			.method = HTTP_POST,
			.handler = http_server_register_erase_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &register_erase);

		// register dhtSensor.json handler
		httpd_uri_t dht_sensor_json = {
			.uri = "/dhtSensor.json",
			.method = HTTP_GET,
			.handler = http_server_get_dht_sensor_readings_json_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &dht_sensor_json);

		// register wifiConnectStatus.json handler
		httpd_uri_t wifi_connect_status_json = {
			.uri = "/wifiConnectStatus",
			.method = HTTP_POST,
			.handler = http_server_wifi_connect_status_json_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_status_json);

		httpd_uri_t read_range_uri = {
			.uri = "/readreg.json",
			.method = HTTP_GET,
			.handler = http_server_read_register_handler,
			.user_ctx = NULL};
		httpd_register_uri_handler(http_server_handle, &read_range_uri);

		httpd_uri_t sensors_status = {
			.uri = "/sensorsStatus.json",
			.method = HTTP_GET,
			.handler = http_server_sensors_status_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &sensors_status);

		httpd_uri_t mode_uri = {
			.uri = "/mode.json",
			.method = HTTP_POST,
			.handler = http_server_mode_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &mode_uri);

		httpd_uri_t manual_pwm_uri = {
			.uri = "/manual_pwm.json",
			.method = HTTP_POST,
			.handler = http_server_manual_pwm_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &manual_pwm_uri);

		httpd_uri_t uri_prog_get = {
			.uri = "/program_registers.json",
			.method = HTTP_GET,
			.handler = http_server_get_program_registers_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &uri_prog_get);

		httpd_uri_t uri_prog_post = {
			.uri = "/program_register.json",
			.method = HTTP_POST,
			.handler = http_server_post_program_register_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &uri_prog_post);

		httpd_uri_t uri_select = {
			.uri = "/select_register.json",
			.method = HTTP_POST,
			.handler = http_server_select_register_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &uri_select);

		return http_server_handle;
	}

	return NULL;
}

void http_server_start(void)
{
	if (http_server_handle == NULL)
	{
		http_server_handle = http_server_configure();
	}
}

void http_server_stop(void)
{
	if (http_server_handle)
	{
		httpd_stop(http_server_handle);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server");
		http_server_handle = NULL;
	}
	if (task_http_server_monitor)
	{
		vTaskDelete(task_http_server_monitor);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server monitor");
		task_http_server_monitor = NULL;
	}
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
	http_server_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

void http_server_fw_update_reset_callback(void *arg)
{
	ESP_LOGI(TAG, "http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}

static esp_err_t http_server_mode_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/mode.json requested");

    size_t content_len = req->content_len;
    if (content_len == 0) {
        int hdr_len = httpd_req_get_hdr_value_len(req, "Content-Length");
        if (hdr_len > 0) {
            char *hdr = malloc(hdr_len + 1);
            if (hdr) {
                if (httpd_req_get_hdr_value_str(req, "Content-Length", hdr, hdr_len + 1) == ESP_OK) {
                    content_len = (size_t)atoi(hdr);
                }
                free(hdr);
            }
        }
    }

    if (content_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Content-Length");
        return ESP_OK;
    }

    char *buf = malloc(content_len + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_OK;
    }

    size_t received = 0;
    while (received < content_len) {
        int r = httpd_req_recv(req, buf + received, content_len - received);
        if (r == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        } else if (r <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive body");
            return ESP_OK;
        }
        received += r;
    }
    buf[content_len] = '\0';
    ESP_LOGI(TAG, "mode body: %s", buf);

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        ESP_LOGI(TAG, "Invalid JSON");
        free(buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_OK;
    }
    free(buf);

    cJSON *mode_j = cJSON_GetObjectItem(root, "mode");
    cJSON *min_j  = cJSON_GetObjectItem(root, "temp_min");
    cJSON *max_j  = cJSON_GetObjectItem(root, "temp_max");
    cJSON *pwm_j  = cJSON_GetObjectItem(root, "pwm");
	cJSON *prog_j = cJSON_GetObjectItem(root, "program_reg");

    // Procesar modo
    if (mode_j && cJSON_IsNumber(mode_j)) {
        current_mode = mode_j->valueint;
        ESP_LOGI(TAG, "Modo configurado: %d", current_mode);
    }

    // Si es modo MANUAL y viene PWM, guardar el PWM
    if (pwm_j && cJSON_IsNumber(pwm_j)) {
        uint32_t pwm_pct = (uint32_t)pwm_j->valueint;
        if (pwm_pct > 100) pwm_pct = 100;
        
        manual_pwm_duty = pwm_pct;
        save_manual_pwm(pwm_pct);
        ESP_LOGI(TAG, "PWM manual establecido en modo.json: %lu%%", pwm_pct);
    }

	// Si viene selección de registro para PROGRAMADO, guardarla
	if (prog_j && cJSON_IsNumber(prog_j)) {
		int reg = prog_j->valueint;
		if (reg < 0) reg = 0;
		if (reg > NUM_REGISTERS_AV) reg = NUM_REGISTERS_AV;
		wifi_app_set_selected_register((uint8_t)reg);
		ESP_LOGI(TAG, "Selected program register set to %d", reg);
	}

    // Procesar y guardar rango de temperatura (solo para AUTOMATICO)
    if (current_mode == 1) {  // AUTOMATICO
        float new_temp_min = current_temp_min;
        float new_temp_max = current_temp_max;

        if (min_j && (cJSON_IsNumber(min_j) || cJSON_IsString(min_j))) {
            new_temp_min = (float)(cJSON_IsNumber(min_j) ? min_j->valuedouble : atof(min_j->valuestring));
        }

        if (max_j && (cJSON_IsNumber(max_j) || cJSON_IsString(max_j))) {
            new_temp_max = (float)(cJSON_IsNumber(max_j) ? max_j->valuedouble : atof(max_j->valuestring));
        }

        // Guardar en flash si hay cambios
        if (new_temp_min != current_temp_min || new_temp_max != current_temp_max) {
            current_temp_min = new_temp_min;
            current_temp_max = new_temp_max;
            save_temp_range(new_temp_min, new_temp_max);
            ESP_LOGI(TAG, "Rango de temperatura guardado: %.2f - %.2f", new_temp_min, new_temp_max);
        }
    }

    cJSON_Delete(root);

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t http_server_manual_pwm_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/manual_pwm.json requested");

    size_t content_len = req->content_len;
    if (content_len == 0) {
        int hdr_len = httpd_req_get_hdr_value_len(req, "Content-Length");
        if (hdr_len > 0) {
            char *hdr = malloc(hdr_len + 1);
            if (hdr) {
                if (httpd_req_get_hdr_value_str(req, "Content-Length", hdr, hdr_len + 1) == ESP_OK) {
                    content_len = (size_t)atoi(hdr);
                }
                free(hdr);
            }
        }
    }

    if (content_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Content-Length");
        return ESP_OK;
    }

    char *buf = malloc(content_len + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_OK;
    }

    size_t received = 0;
    while (received < content_len) {
        int r = httpd_req_recv(req, buf + received, content_len - received);
        if (r == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        } else if (r <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive body");
            return ESP_OK;
        }
        received += r;
    }
    buf[content_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        free(buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_OK;
    }
    free(buf);

    cJSON *pwm_j = cJSON_GetObjectItem(root, "pwm");
    if (pwm_j && cJSON_IsNumber(pwm_j)) {
        uint32_t pwm_pct = (uint32_t)pwm_j->valueint;
        if (pwm_pct > 100) pwm_pct = 100;
        
        manual_pwm_duty = pwm_pct;
        save_manual_pwm(pwm_pct);
        ESP_LOGI(TAG, "PWM manual establecido: %lu%%", pwm_pct);
    }

    cJSON_Delete(root);

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/* GET /program_registers.json  -> devuelve array de registros + selected */
esp_err_t http_server_get_program_registers_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error");
        return ESP_FAIL;
    }

    cJSON *arr = cJSON_CreateArray();
    if (!arr) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error"); return ESP_FAIL; }

    for (uint8_t i = 0; i < NUM_REGISTERS_AV; ++i) {
        program_register_t r;
        esp_err_t ret = load_program_register(i, &r);
        cJSON *obj = cJSON_CreateObject();
        if (!obj) continue;
        cJSON_AddNumberToObject(obj, "idx", i);
        if (ret == ESP_OK) {
            cJSON_AddNumberToObject(obj, "start_hour", r.start_hour);
            cJSON_AddNumberToObject(obj, "start_min", r.start_min);
            cJSON_AddNumberToObject(obj, "end_hour", r.end_hour);
            cJSON_AddNumberToObject(obj, "end_min", r.end_min);
            cJSON_AddNumberToObject(obj, "weekdays", r.weekdays); // bitmask
        } else {
            cJSON_AddNumberToObject(obj, "start_hour", -1);
        }
        cJSON_AddItemToArray(arr, obj);
    }
    cJSON_AddItemToObject(root, "registers", arr);
    cJSON_AddNumberToObject(root, "selected", wifi_app_get_selected_register());

    char *out = cJSON_PrintUnformatted(root);
    if (out) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, out);
        free(out);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error");
    }
    cJSON_Delete(root);
    return ESP_OK;
}

/* POST /program_register.json  -> body JSON { idx, start_hour, start_min, end_hour, end_min, weekdays } */
esp_err_t http_server_post_program_register_handler(httpd_req_t *req)
{
    size_t sz = req->content_len;
    if (sz == 0 || sz > 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
        return ESP_FAIL;
    }
    char *buf = malloc(sz + 1);
    if (!buf) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error"); return ESP_FAIL; }
    if (httpd_req_recv(req, buf, sz) != (int)sz) { free(buf); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); return ESP_FAIL; }
    buf[sz] = 0;

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); return ESP_FAIL; }

    cJSON *jidx = cJSON_GetObjectItem(root, "idx");
    if (!cJSON_IsNumber(jidx)) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); return ESP_FAIL; }
    int idx = jidx->valueint;
    if (idx < 0 || idx >= NUM_REGISTERS_AV) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); return ESP_FAIL; }

    program_register_t r;
    memset(&r, 0, sizeof(r));
    r.start_hour = (uint8_t) cJSON_GetObjectItem(root, "start_hour")->valueint;
    r.start_min  = (uint8_t) cJSON_GetObjectItem(root, "start_min")->valueint;
    r.end_hour   = (uint8_t) cJSON_GetObjectItem(root, "end_hour")->valueint;
    r.end_min    = (uint8_t) cJSON_GetObjectItem(root, "end_min")->valueint;
    r.weekdays   = (uint8_t) cJSON_GetObjectItem(root, "weekdays")->valueint;

    esp_err_t err = save_program_register((uint8_t)idx, &r);
    cJSON_Delete(root);
    if (err == ESP_OK) {
        httpd_resp_sendstr(req, "{\"ok\":1}");
        return ESP_OK;
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error");
        return ESP_FAIL;
    }
}

/* POST /select_register.json -> body JSON { selected: <number> }  (use 255 to deselect) */
esp_err_t http_server_select_register_handler(httpd_req_t *req)
{
    size_t sz = req->content_len;
    if (sz == 0 || sz > 128) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); return ESP_FAIL; }
    char buf[128];
    if (httpd_req_recv(req, buf, sz) != (int)sz) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); return ESP_FAIL; }
    buf[sz] = 0;
    cJSON *root = cJSON_Parse(buf);
    if (!root) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); return ESP_FAIL; }
    cJSON *jsel = cJSON_GetObjectItem(root, "selected");
    if (!cJSON_IsNumber(jsel)) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); return ESP_FAIL; }
    int sel = jsel->valueint;
    if (sel < 0 || sel > 255) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); return ESP_FAIL; }
    if (sel == 255) wifi_app_set_selected_register(0xFF);
    else wifi_app_set_selected_register((uint8_t)sel);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "{\"ok\":1}");
    return ESP_OK;
}
