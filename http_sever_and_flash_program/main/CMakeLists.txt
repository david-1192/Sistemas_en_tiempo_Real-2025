idf_component_register(SRCS "main.c" "wifi_app.c" "http_server.c" "globales.c" 
                    "sensores/ntc.c" "sensores/pir_sensor.c" "sensores/pwm_led.c"
                    PRIV_REQUIRES esp_wifi nvs_flash driver esp_http_server esp_timer app_update json mqtt

                    INCLUDE_DIRS "." "sensores"
                    EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js
                    )