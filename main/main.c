#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h" // ما زلنا نستخدم هذا المكون لأنه يدعم التحديث المدمج، لكن سنمرر له إعدادات HTTP
#include "nvs_flash.h"
#include "led_strip.h"
#include "version.h"

struct version_info current_version = CURRENT_VERSION;

#define BLINK_GPIO 48 // الدبوس الشائع للـ RGB LED في ESP32-S3

// استبدل 192.168.1.50 بـ IP جهاز الكمبيوتر الخاص بك في الشبكة المحلية
#define LOCAL_SERVER_VERSION_URL "http://192.168.1.108:8080/version"
#define LOCAL_SERVER_FIRMWARE_URL "http://192.168.1.108:8080/firmware.bin"

static const char *TAG = "LOCAL_OTA";

void wifi_init_sta(void);

void execute_local_ota(void) {
    ESP_LOGI(TAG, "Starting HTTP OTA Update...");

    esp_http_client_config_t config = {
        .url = LOCAL_SERVER_FIRMWARE_URL,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_https_ota_handle_t ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTP OTA Begin failed");
        return;
    }

    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
    }

    if (esp_https_ota_is_complete_data_received(ota_handle) != true) {
        ESP_LOGE(TAG, "Complete data not received");
    } else {
        err = esp_https_ota_finish(ota_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "OTA Upgrade Successful! Rebooting...");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA Upgrade failed %d", err);
        }
    }
}

void check_local_updates_task(void *pvParameter) {
    vTaskDelay(pdMS_TO_TICKS(5000)); 

    while (1) {
        ESP_LOGI(TAG, "Checking local server for updates (Periodic Check)...");

        struct version_info server_version = {0, 0, 0};
        esp_http_client_config_t config = {
            .url = LOCAL_SERVER_VERSION_URL,
            .timeout_ms = 5000,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        
        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK) {
            esp_http_client_fetch_headers(client);
            int read_len = esp_http_client_read(client, (void *)&server_version, sizeof(server_version));
            if (read_len > 0) {
                ESP_LOGI(TAG, "Current Version: %d.%d.%d | Server Version: %d.%d.%d", current_version.major, current_version.minor, current_version.patch, server_version.major, server_version.minor, server_version.patch);

                if (memcmp(&current_version, &server_version, sizeof(server_version)) != 0) {
                    ESP_LOGI(TAG, "New version found! Initiating OTA Update...");
                    esp_http_client_cleanup(client);
                    execute_local_ota();
                } else {
                    ESP_LOGI(TAG, "System is up-to-date.");
                }
            }
        } else {
            ESP_LOGE(TAG, "Failed to connect to local server. Server might be offline.");
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
    xTaskCreate(&check_local_updates_task, "check_local_updates", 1024 * 8, NULL, 5, NULL);

    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Firmware Update Success! Running Version: %d.%d.%d", current_version.major, current_version.minor, current_version	.patch);
    ESP_LOGI(TAG, "==========================================");

    led_strip_handle_t led_strip;
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // يوجد ليد واحد بالبوردة
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    
    // إنشاء كائن الإضاءة
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip); // إطفاء مبدئي

    while (1) {
        // إضاءة باللون الأحمر مثلاً (R=50, G=0, B=0)
        led_strip_set_pixel(led_strip, 0, 255, 255, 255);
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(5000));

		// إطفاء الليد
		led_strip_clear(led_strip);
		led_strip_refresh(led_strip);
		vTaskDelay(pdMS_TO_TICKS(5000));

    }
}
