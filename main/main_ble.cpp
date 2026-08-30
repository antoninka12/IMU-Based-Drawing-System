#include "ble.hpp"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

#define TAG "MAIN_TEST"

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting BLE test application");

    ble_initialization();

    int sequence_number = 0;

    while (true) {
        char data[64];

        unsigned long timestamp_ms =
            (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        snprintf(
            data,
            sizeof(data),
            "%d,%lu,%d,%d,%d",
            sequence_number,
            timestamp_ms,
            100,
            200,
            300
        );

        if (ble_is_connected()) {
            ble_sending_data(data);
        } else {
            ESP_LOGI(TAG, "Waiting for BLE client connection...");
        }

        sequence_number++;

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}