#include "ble.hpp"
#include "i2c.hpp"
#include "bmi160.hpp"

#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

#include "low_pass.hpp"

#define TAG "MAIN"

static constexpr gpio_num_t I2C_SDA_PIN = GPIO_NUM_21;
static constexpr gpio_num_t I2C_SCL_PIN = GPIO_NUM_22;

static constexpr uint8_t BMI160_ADDRESS = 0x68;

constexpr float LP_ALPHA = 0.5f;

LowPassFilter filter_gx(LP_ALPHA);
LowPassFilter filter_gy(LP_ALPHA);
LowPassFilter filter_gz(LP_ALPHA);

LowPassFilter filter_ax(LP_ALPHA);
LowPassFilter filter_ay(LP_ALPHA);
LowPassFilter filter_az(LP_ALPHA);

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting application");

    /*
     * 1. I2C bus initialization
     */
    AZ_i2c::Bus i2c_bus;

    esp_err_t err = i2c_bus.init(
        I2C_NUM_0,
        I2C_SDA_PIN,
        I2C_SCL_PIN,
        true
    );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "I2C initialization failed: %s",
            esp_err_to_name(err)
        );
        return;
    }

    /*
     * Optional:
     * check which devices respond on I2C bus
     */
    i2c_bus.scan();

    /*
     * 2. BMI160 initialization
     */
    AZ_bmi160::Bmi160 imu;

    err = imu.init(i2c_bus, BMI160_ADDRESS);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "BMI160 initialization failed: %s",
            esp_err_to_name(err)
        );
        return;
    }

    ESP_LOGI(TAG, "BMI160 initialized");

    /*
     * 3. BLE initialization
     */
    ble_initialization();

    ESP_LOGI(TAG, "BLE initialized");

    uint32_t sequence_number = 0;

    /*
     * 4. Main measurement loop
     */
    while (true) {

        AZ_bmi160::CalibratedSample Calsample{};
        AZ_bmi160::CalibratedSample filtered_sample{};

        err = imu.readCalibrated(Calsample);

        if (err != ESP_OK) {
            ESP_LOGE(
                TAG,
                "BMI160 read failed: %s",
                esp_err_to_name(err)
            );

            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /*
         * Low-pass filtering
         */
        filtered_sample.gx = filter_gx.update(Calsample.gx);
        filtered_sample.gy = filter_gy.update(Calsample.gy);
        filtered_sample.gz = filter_gz.update(Calsample.gz);

        filtered_sample.ax = filter_ax.update(Calsample.ax);
        filtered_sample.ay = filter_ay.update(Calsample.ay);
        filtered_sample.az = filter_az.update(Calsample.az);

        /*
         * Timestamp in microseconds since ESP32 startup
         */
        int64_t timestamp_us = esp_timer_get_time();

        /*
         * CSV:
         *
         * sequence,timestamp,gx,gy,gz,ax,ay,az
         */
        char data[128];

       snprintf(
            data,
            sizeof(data),
            "%lu,%lld,"
            "%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,"
            "%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",

            static_cast<unsigned long>(sequence_number),
            static_cast<long long>(timestamp_us),

            Calsample.gx,
            Calsample.gy,
            Calsample.gz,
            Calsample.ax,
            Calsample.ay,
            Calsample.az,

            filtered_sample.gx,
            filtered_sample.gy,
            filtered_sample.gz,
            filtered_sample.ax,
            filtered_sample.ay,
            filtered_sample.az
        );
        /*
         * Local debug log
         */
        ESP_LOGI(TAG, "%s", data);

        /*
         * Send through BLE only when client is connected
         */
        if (ble_is_connected()) {
            ble_sending_data(data);
        }

        sequence_number++;

        /*
         * 50 Hz sampling
         *
         * 1000 ms / 50 = 20 ms
         */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}