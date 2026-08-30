#include "i2c.hpp"
#include "bmi160.hpp"
#include "config.hpp"

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char* TAG = "APP";


extern "C" void app_main(void)
{
    // =========================================================
    // 1. Inicjalizacja magistrali I2C
    // =========================================================

    static AZ_i2c::Bus bus;

    esp_err_t err = bus.init(
        AZ_config::I2C_PORT,
        AZ_config::I2C_SDA,
        AZ_config::I2C_SCL,
        AZ_config::I2C_INTERNAL_PULLUP
    );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "I2C init failed: %s",
            esp_err_to_name(err)
        );

        return;
    }

    ESP_LOGI(TAG, "I2C initialized");


    // =========================================================
    // 2. Skan magistrali
    // =========================================================

    ESP_LOGI(TAG, "Scanning I2C bus...");

    bus.scan();


    // =========================================================
    // 3. Sprawdzenie adresu BMI160
    // =========================================================

    err = bus.probe(AZ_config::IMU_ADDR);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "BMI160 not found at address 0x%02X",
            AZ_config::IMU_ADDR
        );

        return;
    }

    ESP_LOGI(
        TAG,
        "Device found at address 0x%02X",
        AZ_config::IMU_ADDR
    );


    // =========================================================
    // 4. Inicjalizacja BMI160
    // =========================================================

    static AZ_bmi160::Bmi160 imu;

    err = imu.init(
        bus,
        AZ_config::IMU_ADDR
    );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "BMI160 init failed: %s",
            esp_err_to_name(err)
        );

        return;
    }

    ESP_LOGI(TAG, "BMI160 initialized successfully");


    // =========================================================
    // 5. Odczyt danych
    // =========================================================

    while (true)
    {
        AZ_bmi160::RawSample sample{};

        err = imu.readRaw(sample);

        if (err == ESP_OK)
        {
            ESP_LOGI(
                TAG,
                "ACC [raw]: X=%d Y=%d Z=%d | "
                "GYR [raw]: X=%d Y=%d Z=%d",
                static_cast<int>(sample.ax),
                static_cast<int>(sample.ay),
                static_cast<int>(sample.az),
                static_cast<int>(sample.gx),
                static_cast<int>(sample.gy),
                static_cast<int>(sample.gz)
            );

            // =============================================
            // Tutaj później wysyłasz sample przez BLE
            //
            // np.
            //
            // ble.send(...);
            //
            // =============================================
        }
        else
        {
            ESP_LOGE(
                TAG,
                "BMI160 read failed: %s",
                esp_err_to_name(err)
            );
        }


        // Na test 10 Hz, żeby logi były czytelne.
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}