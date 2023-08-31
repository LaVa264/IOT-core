#include <Arduino.h>
#include <nvs_flash.h>

#include "wifi_app.hpp"

void setup() {
    Serial.begin(115200);

    /* 1. Initialize NVS. */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES
        || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
    
    ESP_ERROR_CHECK(ret);
}

void loop() {
    wifi_app_start();
    while (1);
}