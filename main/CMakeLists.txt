idf_component_register(SRCS "wifi.c" "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES led_strip esp_event app_update esp_http_client esp_https_ota nvs_flash esp_wifi
                    )



