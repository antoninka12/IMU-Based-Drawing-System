idf_component_register(
    SRCS "main.cpp" "i2c.cpp" "ble.cpp" "bmi160.cpp"
    INCLUDE_DIRS "."
    REQUIRES bt nvs_flash esp_driver_i2c  esp_driver_gpio
)