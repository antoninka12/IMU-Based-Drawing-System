#include "ble.hpp"
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

static uint8_t own_addr_type;

static int gap_event_handler(struct ble_gap_event *event, void *arg);

static int imu_data_access_cb(uint16_t conn_handle,
                              uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt,
                              void *arg);

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
            .access_cb=imu_data_access_cb,
            .flags=BLE_GATT_CHR_F_NOTIFY,
            .val_handle=&imu_val_handle,
            },{
            0,
            }
        },
    },
    {0}

    };


    static int imu_data_access_cb(uint16_t conn_handle,
                              uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt,
                              void *arg)
{
    return 0;
}

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


/*checking if receiver enabled data reception*/
static void gatt_svr_subscribe_cb(struct ble_gap_event *event){
    if(event->subscribe.attr_handle ==imu_val_handle){
        imu_conn_handle=event->subscribe.conn_handle;
        imu_notify_enabled=event->subscribe.cur_notify;

        if(!imu_notify_enabled){
            ESP_LOGE(TAG, "IMU devices notifications are disabled");
        }
        else{
                  ESP_LOGE(TAG, "IMU devices notifications are enabled");
        }
   }
}

/*Uruchamia reklamowanie BLE, czyli sprawia, że urządzenie jest widoczne jako Drawing_Device.

Wzoruj się na:
gap.c → funkcja:

static void start_advertising(void)*/

/*BLE advertasing*/
static void start_advertising(void){
    int support_variable;

    const char *dev_name=ble_svc_gap_device_name();

    struct ble_hs_adv_fields az_adv_fields = {0};
    az_adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    az_adv_fields.name = (uint8_t *)dev_name;
    az_adv_fields.name_len = strlen(dev_name);
    az_adv_fields.name_is_complete = 1;

    support_variable=ble_gap_adv_set_fields(&az_adv_fields);

    if(support_variable!=0){
        ESP_LOGE(TAG, "Failed to advertise, error: %d", support_variable);
        return;
    }

    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(500);
    adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(510);

    support_variable = ble_gap_adv_start(
    own_addr_type,
    NULL,
    BLE_HS_FOREVER,
    &adv_params,
    gap_event_handler,
    NULL
);

    if(support_variable!=0){
        ESP_LOGE(TAG, "Failed to advertise, error: %d", support_variable);
        return;
    }

    ESP_LOGE(TAG, "Advertising");
}

/*Wzoruj się na:
gap.c → funkcja:

static int gap_event_handler(struct ble_gap_event *event, void *arg)*/


/*main.c z przykładu → funkcja typu: */

static int gap_event_handler(struct ble_gap_event *event, void *arg)
{
    int rc = 0;

    switch (event->type) {

    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            imu_connected = true;
            imu_conn_handle = event->connect.conn_handle;

            ESP_LOGI(TAG,
                     "BLE connected; conn_handle=%d",
                     imu_conn_handle);
        } else {
            imu_connected = false;
            imu_notify_enabled = false;
            imu_conn_handle = BLE_HS_CONN_HANDLE_NONE;

            ESP_LOGE(TAG,
                     "BLE connection failed; status=%d",
                     event->connect.status);

            start_advertising();
        }
        return rc;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG,
                 "BLE disconnected; reason=%d",
                 event->disconnect.reason);

        imu_connected = false;
        imu_notify_enabled = false;
        imu_conn_handle = BLE_HS_CONN_HANDLE_NONE;

        start_advertising();
        return rc;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG,
                 "Subscribe event; conn_handle=%d attr_handle=%d cur_notify=%d",
                 event->subscribe.conn_handle,
                 event->subscribe.attr_handle,
                 event->subscribe.cur_notify);

        gatt_svr_subscribe_cb(event);
        return rc;

    case BLE_GAP_EVENT_NOTIFY_TX:
        if (event->notify_tx.status != 0 &&
            event->notify_tx.status != BLE_HS_EDONE) {
            ESP_LOGW(TAG,
                     "Notify error; conn_handle=%d attr_handle=%d status=%d",
                     event->notify_tx.conn_handle,
                     event->notify_tx.attr_handle,
                     event->notify_tx.status);
        }
        return rc;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG,
                 "MTU updated; conn_handle=%d mtu=%d",
                 event->mtu.conn_handle,
                 event->mtu.value);
        return rc;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG,
                 "Advertising complete; reason=%d",
                 event->adv_complete.reason);

        start_advertising();
        return rc;

    default:
        return rc;
    }
}

/*Wzoruj się na:
main.c z przykładu → funkcja:

static void host_task(void *param)

albo:

void bleprph_host_task(void *param)*/
static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE host task started");

   
    nimble_port_run();

    nimble_port_freertos_deinit();

    ESP_LOGI(TAG, "BLE host task stopped");
}

/*Wzoruj się na:
main.c z przykładu → app_main(), dokładnie na kolejności:

nvs_flash_init();
nimble_port_init();
ble_svc_gap_init();
ble_svc_gap_device_name_set(...);
gatt_svc_init();
nimble_port_freertos_init(...);*/
void ble_initialization(void)
{
    int support_variable;

    
    support_variable = nvs_flash_init();
    if (support_variable != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS, error code: %d", support_variable);
        return;
    }

   
    support_variable = nimble_port_init();
    if (support_variable != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NimBLE port, error code: %d", support_variable);
        return;
    }

   
    ble_svc_gap_init();

    support_variable = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (support_variable != 0) {
        ESP_LOGE(TAG, "Failed to set BLE device name, error code: %d", support_variable);
        return;
    }

    support_variable = gatt_service_initialization();
    if (support_variable != 0) {
        ESP_LOGE(TAG, "Failed to initialize GATT service, error code: %d", support_variable);
        return;
    }

    ble_hs_cfg.sync_cb = []() {
        int rc;

        rc = ble_hs_id_infer_auto(0, &own_addr_type);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to infer BLE address type, error code: %d", rc);
            return;
        }

        start_advertising();
    };

    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE initialization finished");
}

/*Wzoruj się częściowo na:
gatt_svc.c → funkcja:

void send_heart_rate_indication(void)

ale zamiast:

ble_gatts_indicate(...)*/
void ble_sending_data(const char *data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "Data pointer is NULL");
        return;
    }

    if (!imu_connected) {
        ESP_LOGW(TAG, "Cannot send data: BLE client is not connected");
        return;
    }

    if (!imu_notify_enabled) {
        ESP_LOGW(TAG, "Cannot send data: notifications are not enabled");
        return;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, strlen(data));

    if (om == NULL) {
        ESP_LOGE(TAG, "Failed to allocate BLE mbuf");
        return;
    }

    int rc = ble_gatts_notify_custom(
        imu_conn_handle,
        imu_val_handle,
        om
    );

    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to send notification, error code: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "BLE data sent: %s", data);
}


bool ble_is_connected(void)
{
    return imu_connected;
}