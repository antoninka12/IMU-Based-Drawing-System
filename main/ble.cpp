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

/*uuid for service in BLE*/
static const ble_uuid128_t IMU_SERVICE_UUID =
    BLE_UUID128_INIT(0x7e, 0x56, 0x41, 0x87, 0x85, 0x32,
        0x4c, 0x32, 0xae, 0xe1, 0x93, 0xe1, 0x52, 0x3f, 0x3c, 0x32); /*genereted in python uuid for personal purpose*/

/*uuid for charcteristic in BLE*/
static const ble_uuid128_t IMU_DATA_UUID =
    BLE_UUID128_INIT(
        0xf9, 0x9a, 0xe6, 0x92, 0x5d, 0xf4,0x40, 0x11,
        0x94, 0x53, 0x5e, 0xc4, 0xc8, 0xe8, 0x54, 0x77
    );
/*handle to characteristics, set by NimBLE, 
used for which characteristic i want to send data*/
static uint16_t imu_val_handle; 
/*handle to connection, number of current connection*/
static uint16_t imu_conn_handle=0;

/*flag for connection*/
static bool imu_connected=false;
/*flag for notifications*/
static bool imu_notify_enabled=false;

/*create BLE service with set UUID and primary type
with characteristic (with set UUID, flags for notify) and 
save to handle*/
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
        {
        .type=BLE_GATT_SVC_TYPE_PRIMARY, /*main service*/
        .uuid=&IMU_SERVICE_UUID.u,
        .characteristics=(struct ble_gatt_chr_def[]){
            {
            .uuid=&IMU_DATA_UUID.u,
            .access_cb=NULL,
            .flags=BLE_GATT_CHR_F_NOTIFY,
            .val_handle=&imu_val_handle,
            },{
            0,
            }
        },
    },
    {0}

    };

    /*Wzoruj się na:
gatt_svc.c → funkcja int gatt_svc_init(void)*/
static int gatt_service_initialization(void){

    int support_variable;

    /*iniztialization of gatt service*/
    ble_svc_gatt_init();

    /*counting services and characteristics in gatt-svr_svcs table*/
    support_variable=ble_gatts_count_cfg(gatt_svr_svcs); 

    if(support_variable!=0){
        ESP_LOGE(TAG, "Failed in counting services (ble-gatts_count_cfg function), error code: %d", support_variable);
        return support_variable; /*return int of error code*/
    }

    support_variable=ble_gatts_add_svcs(gatt_svr_svcs);

     if(support_variable!=0){
        ESP_LOGE(TAG, "Failed to add gatt service, error code: %d", support_variable);
        return support_variable; /*return int of error code*/
    }

    return 0;
}

/* Wzoruj się na:
gatt_svc.c → funkcja:

void gatt_svr_subscribe_cb(struct ble_gap_event *event)*/
static void gatt_svr_subscribe_cb(struct ble_gap_event *event){
    
}

/*Uruchamia reklamowanie BLE, czyli sprawia, że urządzenie jest widoczne jako Drawing_Device.

Wzoruj się na:
gap.c → funkcja:

static void start_advertising(void)*/
static void start_advertising(void)

/*Wzoruj się na:
gap.c → funkcja:

static int gap_event_handler(struct ble_gap_event *event, void *arg)*/
static int gap_event_handler(struct ble_gap_event *event, void *arg)

/*main.c z przykładu → funkcja typu:

static void on_stack_sync(void), static void bleprph_on_sync(void)*/
static void on_stack_sync(void)

/*Wzoruj się na:
main.c z przykładu → funkcja:

static void host_task(void *param)

albo:

void bleprph_host_task(void *param)*/
static void ble_host_task(void *param)

/*Wzoruj się na:
main.c z przykładu → app_main(), dokładnie na kolejności:

nvs_flash_init();
nimble_port_init();
ble_svc_gap_init();
ble_svc_gap_device_name_set(...);
gatt_svc_init();
nimble_port_freertos_init(...);*/
void ble_initialization(void)

/*Wzoruj się częściowo na:
gatt_svc.c → funkcja:

void send_heart_rate_indication(void)

ale zamiast:

ble_gatts_indicate(...)*/
void ble_sending_data(const char *data)


bool ble_is_connected(void)