#pragma once

#include <cstdint>
#include "esp_err.h"
#include "i2c.hpp"

namespace AZ_bmi160 {

struct RawSample {
    int16_t gx;
    int16_t gy;
    int16_t gz;

    int16_t ax;
    int16_t ay;
    int16_t az;
};

class Bmi160 {
public:
    esp_err_t init(const AZ_i2c::Bus& bus, uint8_t addr = 0x68);

    esp_err_t readRaw(RawSample& sample) const;

private:
    AZ_i2c::Device dev_;
};

} // namespace AZ_bmi160