#include <stdint.h>
#include <string.h>
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h" 
#include "nvs_flash.h"
#include "led_strip.h"
#include "version.h"

struct version_info current_version = CURRENT_VERSION;

#define BLINK_GPIO 48 

#define CDN_SERVER_VERSION_URL  "https://d2bpgfntnplhed.cloudfront.net/version.bin"
#define CDN_SERVER_FIRMWARE_URL "https://d2bpgfntnplhed.cloudfront.net/firmware.bin"

static const char *TAG = "CLOUD_OTA";

extern const uint8_t ota_server_cert_pem_start[] asm("_binary_ota_server_cert_pem_start");
extern const uint8_t ota_server_cert_pem_end[]   asm("_binary_ota_server_cert_pem_end");

void wifi_init_sta(void);

void execute_cloud_ota(void) {
    ESP_LOGI(TAG, "Starting Secure HTTPS Cloud OTA Update...");

    esp_http_client_config_t config = {
        .url = CDN_SERVER_FIRMWARE_URL,
        .cert_pem = (char *)ota_server_cert_pem_start,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_https_ota_handle_t ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed: %s", esp_err_to_name(err));
        return;
    }

    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
    }

    if (esp_https_ota_is_complete_data_received(ota_handle) != true) {
        ESP_LOGE(TAG, "Complete firmware data not received");
    } else {
        err = esp_https_ota_finish(ota_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "OTA Upgrade Successful! Rebooting into new system...");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA Upgrade failed %d", err);
        }
    }
}

void check_cloud_updates_task(void *pvParameter) {
    vTaskDelay(pdMS_TO_TICKS(5000)); 

    while (1) {
        ESP_LOGI(TAG, "Checking CloudFront CDN for updates (Periodic Check)...");

        struct version_info server_version = {0, 0, 0};

        esp_http_client_config_t config = {
            .url = CDN_SERVER_VERSION_URL,
            .cert_pem = (char *)ota_server_cert_pem_start, 
            .timeout_ms = 5000,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        
        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK) {
            esp_http_client_fetch_headers(client);
            int read_len = esp_http_client_read(client, (void *)&server_version, sizeof(server_version));
            if (read_len == 3) {
                ESP_LOGI(TAG, "Current Version: %d.%d.%d | Cloud Version: %d.%d.%d", 
                            current_version.major, current_version.minor, current_version.patch, 
                            server_version.major, server_version.minor, server_version.patch);

                if (memcmp(&current_version, &server_version, sizeof(server_version)) != 0) {
                    ESP_LOGI(TAG, "New cloud version found! Initiating HTTPS OTA...");
                    esp_http_client_cleanup(client);
                    execute_cloud_ota();
                } else {
                    ESP_LOGI(TAG, "System is up-to-date.");
                }
            } else {
                ESP_LOGE(TAG, "Failed to read version info from CloudFront CDN. Read length: %d", read_len);
            }
        } else {
            ESP_LOGE(TAG, "Failed to connect to CloudFront CDN. Error: %s", esp_err_to_name(err));
        }
        
        esp_http_client_cleanup(client);

        ESP_LOGI(TAG, "Waiting 30 seconds before next check...");
        vTaskDelay(pdMS_TO_TICKS(30000)); 
    }    
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
    
    xTaskCreate(&check_cloud_updates_task, "check_cloud_updates", 1024 * 8, NULL, 5, NULL);

    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Firmware Running Version: %d.%d.%d", current_version.major, current_version.minor, current_version.patch);
    ESP_LOGI(TAG, "==========================================");

    led_strip_handle_t led_strip;
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, 
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, 
    };
    
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip); 

    int color = 0;
    while (1) {
        color = esp_random();
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        led_strip_set_pixel(led_strip, 0, r, g, b);
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(400));
    }
}
