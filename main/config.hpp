#pragma once
#include "driver/gpio.h"
#include "driver/i2c_master.h"

namespace AZ_config {

/*i2c magistral*/
constexpr i2c_port_num_t I2C_PORT = I2C_NUM_0;
constexpr gpio_num_t     I2C_SDA  = GPIO_NUM_21;
constexpr gpio_num_t     I2C_SCL  = GPIO_NUM_22;
constexpr bool           I2C_INTERNAL_PULLUP = true;

/*BMI160*/
constexpr uint8_t    IMU_ADDR = 0x68;   // SDO->GND; 0x69 gdy SDO->VDD
constexpr gpio_num_t IMU_INT1 = GPIO_NUM_4;

} // namespace AZ_config 
