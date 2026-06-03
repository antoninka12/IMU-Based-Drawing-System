#pragma once

#include <stdbool.h>

/*inizialization of BLE*/
void ble_initialization(void);

/*sending data through BLE*/
void ble_sending_data(const char *data);

/*checking if BLE is connected*/
bool ble_is_connected(void);