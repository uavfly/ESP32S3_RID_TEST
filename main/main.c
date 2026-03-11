#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nvs_flash.h"

#include "esp_gap_ble_api.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ble.h"
#include "wifi.h"
#include "RID_Payload.h"
#include "ble50.h"
#include "wifi_beacon.h"
#include "wifi_nan.h"

#define TAG "RID_APP"

static RIDContext rid_ctx;
static RIDPayloadBuffer payload_buffer;
static WiFi_NAN_packet nan_packet;
static WiFi_Beacon_packet beacon_packet;
static uint8_t wifi_mac[6];
static uint8_t msg_counter = 0;
static volatile bool adv_started = false;

static esp_ble_gap_ext_adv_t ext_adv = {
    .instance = 0,
    .duration = 0,
    .max_events = 0,
};

// BLE GAP 事件处理回调
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
        if (param->ext_adv_set_params.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "设置广播参数失败: %d", param->ext_adv_set_params.status);
            break;
        }
        if (!ble_5_0_payload_send(ble_get_ext_adv_handle(), &payload_buffer, &msg_counter)) {
            ESP_LOGE(TAG, "初始 BLE 广播数据下发失败");
        }
        break;

    case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        if (param->ext_adv_data_set.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "设置广播数据失败: %d", param->ext_adv_data_set.status);
            break;
        }
        if (!adv_started) {
            ESP_ERROR_CHECK(esp_ble_gap_ext_adv_start(1, &ext_adv));
        }
        break;

    case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
        if (param->ext_adv_start.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "BLE 广播启动失败: %d", param->ext_adv_start.status);
        } else {
            adv_started = true;
            ESP_LOGI(TAG, "BLE 5.0 RID 广播已启动");
        }
        break;

    case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, "BLE 广播已停止");
        break;

    default:
        break;
    }
}

// 设备初始化：NVS、Wi-Fi、BLE 栈、RID 上下文和初始广播数据准备
static void device_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //wifi_raw_tx_init();
    //ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, wifi_mac));

    //wifi_nan_build_template(&nan_packet);
    //wifi_nan_set_source_mac(&nan_packet, wifi_mac);

    ble_stack_init(gap_event_handler);

    RID_DATA_INIT(&rid_ctx.basic, &rid_ctx.pos_vec, &rid_ctx.rd, &rid_ctx.sys, "RID_TEST_123","ESP32S3_RID_TEST");
    ESP_ERROR_CHECK(RIDPacket(&rid_ctx.basic, &rid_ctx.pos_vec, &rid_ctx.rd, &rid_ctx.sys, &payload_buffer) ? ESP_OK : ESP_FAIL);

    memset(&beacon_packet, 0, sizeof(beacon_packet));
    //ESP_ERROR_CHECK(wifi_update_beacon_ie(&beacon_packet, &payload_buffer) ? ESP_OK : ESP_FAIL);

    ESP_LOGI(TAG, "设备初始化完成");
}

void app_main(void)
{
    device_init();

    ESP_ERROR_CHECK(esp_ble_gap_ext_adv_set_params(ble_get_ext_adv_handle(), ble_get_ext_adv_params()));

    // 等待 BLE 广播启动完成
    while (!adv_started) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    uint16_t tick = 0;
    uint8_t sec_div = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(200));
        tick = (tick + 2) % 36000;

        RID_DATA_UPDATE(&rid_ctx.pos_vec, &rid_ctx.sys, 300000000, 1200000000, 80, tick);
        if (!RIDPacket(&rid_ctx.basic, &rid_ctx.pos_vec, &rid_ctx.rd, &rid_ctx.sys, &payload_buffer)) {
            ESP_LOGE(TAG, "RID payload 打包失败");
            continue;
        }

       /*  if (!ble_5_0_payload_send(ble_get_ext_adv_handle(), &payload_buffer, &msg_counter)) {
            ESP_LOGE(TAG, "BLE payload 更新失败");
        } */

        if(!ble_5_0_payload_send_step(ble_get_ext_adv_handle(), &rid_ctx.basic, &rid_ctx.pos_vec, &rid_ctx.rd, &rid_ctx.sys, &msg_counter)) {
            ESP_LOGE(TAG, "BLE 分体 payload 更新失败");
        }


        sec_div++;
        if (sec_div >= 5) {
            sec_div = 0;
            /* if (!wifi_update_beacon_ie(&beacon_packet, &payload_buffer)) {
                ESP_LOGE(TAG, "Beacon IE 更新失败");
            }
 */
            /* for (int i = 0; i < 10; i++) {
                wifi_send_nan_frame(&nan_packet);
                vTaskDelay(pdMS_TO_TICKS(5));
            } */
            ESP_LOGI(TAG, "RID 广播已更新 tick=%u", tick);
        }
    }
}
