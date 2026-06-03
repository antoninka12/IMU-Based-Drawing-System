#include "ble.h"
#include "esp_log.h" /*esp-idf logs*/

/*includes for BLE*/
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#define DEVICE_NAME "Drawing_Device"
#define TAG "BLE_modul"