#pragma once
#include <cstdint>

namespace AZ_bmi160 {

constexpr uint8_t CHIP_ID_VALUE = 0xD1;

namespace reg {

    // Identyfikacja układu
    constexpr uint8_t CHIP_ID  = 0x00;

    // Dane pomiarowe
    constexpr uint8_t DATA_GYR = 0x0C;  // gyro X,Y,Z - 6 bajtów
    constexpr uint8_t DATA_ACC = 0x12;  // accel X,Y,Z - 6 bajtów

    // Rejestr komend
    constexpr uint8_t CMD      = 0x7E;

} // namespace reg


namespace cmd {

    // Reset BMI160
    constexpr uint8_t SOFT_RESET = 0xB6;

    // Włączenie sensorów
    constexpr uint8_t ACC_NORMAL = 0x11;
    constexpr uint8_t GYR_NORMAL = 0x15;

} // namespace cmd

} // namespace AZ_bmi160