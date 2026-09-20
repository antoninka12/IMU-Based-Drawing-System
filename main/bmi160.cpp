#include "bmi160.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

namespace AZ_bmi160 {

namespace {

constexpr char TAG[] = "BMI160";

constexpr uint8_t REG_CHIP_ID  = 0x00;
constexpr uint8_t REG_GYR_DATA = 0x0C;
constexpr uint8_t REG_CMD      = 0x7E;

constexpr uint8_t CHIP_ID_VALUE = 0xD1;

constexpr uint8_t CMD_ACC_NORMAL = 0x11;
constexpr uint8_t CMD_GYR_NORMAL = 0x15;

constexpr uint8_t REG_ACC_RANGE = 0x41;
constexpr uint8_t REG_GYR_RANGE = 0x43;

constexpr uint8_t REG_ACC_CONF = 0x40;
constexpr uint8_t REG_GYR_CONF = 0x42;

constexpr uint8_t ODR_MASK = 0x0F;
constexpr uint8_t ODR_50HZ = 0x07;


constexpr float IMU_AC_OFFSET_X =  1107.87f;
constexpr float IMU_AC_OFFSET_Y =  -322.67f;
constexpr float IMU_AC_OFFSET_Z =  -474.60f;

constexpr float IMU_AC_SCALE_X = 16233.14f;
constexpr float IMU_AC_SCALE_Y = 16348.07f;
constexpr float IMU_AC_SCALE_Z = 16448.09f;


constexpr float IMU_GYR_OFFSET_X =  7.36f;
constexpr float IMU_GYR_OFFSET_Y =  6.36f;
constexpr float IMU_GYR_OFFSET_Z = -0.78f;

constexpr float IMU_GYR_SCALE = 16.4f;


constexpr float GRAVITY = 9.80665f;

// BMI160 wysyła dane little-endian:
// najpierw LSB, potem MSB.
int16_t toInt16(uint8_t lsb, uint8_t msb)
{
    return static_cast<int16_t>(
        static_cast<uint16_t>(lsb) |
        (static_cast<uint16_t>(msb) << 8)
    );
}

} // namespace


esp_err_t Bmi160::init(const AZ_i2c::Bus& bus, uint8_t addr) 
{ 
    // Dodanie BMI160 do magistrali I2C 
    esp_err_t err = dev_.init(bus, addr); 
 
    if (err != ESP_OK) { 
        ESP_LOGE(TAG, "I2C init failed"); 
        return err; 
    } 
 
 
    // Sprawdzenie czy rzeczywiście mamy BMI160 
    uint8_t chip_id = 0; 
 
    err = dev_.readReg(REG_CHIP_ID, chip_id); 
 
    if (err != ESP_OK) { 
        ESP_LOGE(TAG, "Cannot read CHIP_ID"); 
        return err; 
    } 
 
    if (chip_id != CHIP_ID_VALUE) { 
        ESP_LOGE(TAG, 
                 "Wrong CHIP_ID: 0x%02X, expected 0x%02X", 
                 chip_id, 
                 CHIP_ID_VALUE); 
 
        return ESP_ERR_NOT_FOUND; 
    } 
 
    ESP_LOGI(TAG, "BMI160 found, CHIP_ID = 0x%02X", chip_id); 
 
 
    // Włączenie akcelerometru 
    err = dev_.writeReg(REG_CMD, CMD_ACC_NORMAL); 
 
    if (err != ESP_OK) { 
        return err; 
    } 
 
    vTaskDelay(pdMS_TO_TICKS(5)); 
 
 
    // Włączenie żyroskopu 
    err = dev_.writeReg(REG_CMD, CMD_GYR_NORMAL); 
 
    if (err != ESP_OK) { 
        return err; 
    } 
 
    vTaskDelay(pdMS_TO_TICKS(80)); 
 
 
    // Ustawienie ODR akcelerometru i żyroskopu na 50 Hz
    uint8_t acc_conf = 0;
    uint8_t gyr_conf = 0;

    err = dev_.readReg(REG_ACC_CONF, acc_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot read ACC_CONF");
        return err;
    }

    err = dev_.readReg(REG_GYR_CONF, gyr_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot read GYR_CONF");
        return err;
    }

    acc_conf = (acc_conf & ~ODR_MASK) | ODR_50HZ;
    gyr_conf = (gyr_conf & ~ODR_MASK) | ODR_50HZ;

    err = dev_.writeReg(REG_ACC_CONF, acc_conf);
    if (err != ESP_OK) {
        return err;
    }

    err = dev_.writeReg(REG_GYR_CONF, gyr_conf);
    if (err != ESP_OK) {
        return err;
    }

    err = dev_.readReg(REG_ACC_CONF, acc_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot read ACC_CONF");
        return err;
    }

    err = dev_.readReg(REG_GYR_CONF, gyr_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot read GYR_CONF");
        return err;
    }

    ESP_LOGI(TAG, "ACC_CONF register = 0x%02X", acc_conf);
    ESP_LOGI(TAG, "GYR_CONF register = 0x%02X", gyr_conf);


    uint8_t acc_range = 0; 
    uint8_t gyr_range = 0; 
 
    err = dev_.readReg(REG_ACC_RANGE, acc_range); 
    if (err != ESP_OK) { 
        ESP_LOGE(TAG, "Cannot read ACC_RANGE"); 
        return err; 
    } 
 
    err = dev_.readReg(REG_GYR_RANGE, gyr_range); 
    if (err != ESP_OK) { 
        ESP_LOGE(TAG, "Cannot read GYR_RANGE"); 
        return err; 
    } 
 
    ESP_LOGI(TAG, "ACC_RANGE register = 0x%02X", acc_range); 
    ESP_LOGI(TAG, "GYR_RANGE register = 0x%02X", gyr_range); 
 
 
    ESP_LOGI(TAG, "BMI160 ready"); 
 
    return ESP_OK; 
}


esp_err_t Bmi160::readRaw(RawSample& sample) const
{
    uint8_t data[12];

    // 0x0C - 0x11 -> gyro
    // 0x12 - 0x17 -> accel
    esp_err_t err = dev_.readRegs(
        REG_GYR_DATA,
        data,
        sizeof(data)
    );

    if (err != ESP_OK) {
        return err;
    }


    sample.gx = toInt16(data[0],  data[1]);
    sample.gy = toInt16(data[2],  data[3]);
    sample.gz = toInt16(data[4],  data[5]);

    sample.ax = toInt16(data[6],  data[7]);
    sample.ay = toInt16(data[8],  data[9]);
    sample.az = toInt16(data[10], data[11]);


    return ESP_OK;
}

esp_err_t Bmi160::readCalibrated(CalibratedSample& sample) const
{
    RawSample raw{};

    esp_err_t err = readRaw(raw);

    if (err != ESP_OK) {
        return err;
    }

    sample.ax =
    ((raw.ax - IMU_AC_OFFSET_X)
         / IMU_AC_SCALE_X)* GRAVITY;

    sample.ay =
        ((raw.ay - IMU_AC_OFFSET_Y)
        / IMU_AC_SCALE_Y)* GRAVITY;

    sample.az =
        ((raw.az - IMU_AC_OFFSET_Z)
        / IMU_AC_SCALE_Z)* GRAVITY;

    sample.gx =
        (raw.gx - IMU_GYR_OFFSET_X)
        / IMU_GYR_SCALE;

    sample.gy =
        (raw.gy - IMU_GYR_OFFSET_Y)
        / IMU_GYR_SCALE;

    sample.gz =
        (raw.gz - IMU_GYR_OFFSET_Z)
        / IMU_GYR_SCALE;
    return ESP_OK;
}


} // namespace AZ_bmi160