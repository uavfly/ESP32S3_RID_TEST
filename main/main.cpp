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
#include "RID_Payload.hpp"
#include "ble50.h"
#include "wifi_beacon.h"
#include "wifi_nan.h"
#include "TimeBase.h"

#define TAG "RID_APP"

static RID_Data rid_data;
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
        if (!ble_5_0_payload_send(ble_get_ext_adv_handle(), &rid_data, &msg_counter)) {
            ESP_LOGE(TAG, "初始 BLE 广播数据下发失败");
        }
        break;

    case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        if (param->ext_adv_data_set.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "设置广播数据失败: %d", param->ext_adv_data_set.status);
            break;
        }else{
            ESP_LOGI(TAG, "BLE 广播数据成功，准备进入 TDM 循环");
        }
        // 不在这里自动启动，交由主循环进行 TDM 时序控制启动
        adv_started = true;
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

    wifi_raw_tx_init();
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, wifi_mac));

    wifi_nan_build_template(&nan_packet);
    wifi_nan_set_source_mac(&nan_packet, wifi_mac);

    ble_stack_init(gap_event_handler);

    // 初始化基本ID报文
    rid_data.basic.UA_Type = 2;
    rid_data.basic.ID_Type = 1;
    RIDUasIDSerialize("RID_TEST_123", &rid_data.basic);
    rid_data.basic_en = true;

    // 初始化运行描述报文
    rid_data.rd.DescriptionType = 0;
    RIDDescription("ESP32S3_RID_TEST", &rid_data.rd);
    rid_data.rd_en = true;

    // 初始化系统报文（静态字段）
    rid_data.sys.ControlStationPosType = 1;
    rid_data.sys.RegionCode = 2;
    rid_data.sys.CoordType = 0;
    rid_data.sys.ControlStationLatitude = 350000000;
    rid_data.sys.ControlStationLongitude = 1100000000;
    rid_data.sys.UARunCategory = 1;
    rid_data.sys.UARunLevel = 0;
    rid_data.sys_en = true;

    // 初始化操作者ID报文
    rid_data.operator_id.OperatorIDType = 0; // CAA注册ID
    memset(rid_data.operator_id.OperatorID, 0, sizeof(rid_data.operator_id.OperatorID));
    strncpy(reinterpret_cast<char*>(rid_data.operator_id.OperatorID), "OP-ESP32S3-001", 20);
    rid_data.op_id_en = true;

    memset(&beacon_packet, 0, sizeof(beacon_packet));
    if (!wifi_update_beacon_ie(&beacon_packet, &rid_data)) {
        ESP_LOGW(TAG, "初始 Beacon IE 设置失败（可能 payload 尚未就绪）");
    }

    ESP_LOGI(TAG, "设备初始化完成");
}

extern "C" void app_main(void)
{
    device_init();

    ESP_ERROR_CHECK(esp_ble_gap_ext_adv_set_params(ble_get_ext_adv_handle(), ble_get_ext_adv_params()));

    // 等待初始参数和数据下发完成
    while (!adv_started) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    uint16_t tick = 0;
    const uint8_t ext_adv_inst = ext_adv.instance;

    // 使用 xTaskGetTickCount 配合 vTaskDelayUntil 实现严格的绝对时基（2Hz -> 500ms 周期）
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500);

    while (1) {
        tick = (tick + 2) % 36000;

        // ---------- 1. 数据更新 ----------
        // 更新位置向量报文
        rid_data.pos_vec.OperationalStatus = 2;
        rid_data.pos_vec.Latitude = 300000000;
        rid_data.pos_vec.Longitude = 1200000000;
        rid_data.pos_vec.AltitudeAGL = 80;
        rid_data.pos_vec.Timestamp = tick;
        rid_data.pos_vec_en = true;

        // 更新系统报文动态字段
        rid_data.sys.ControlStationLatitude = 350000000;
        rid_data.sys.ControlStationLongitude = 1100000000;
        rid_data.sys.Timestamp = tick / 10;


        // ---------- 2. 严格时分复用 (TDM) 发送状态机 ----------
        TickType_t slot_start;

        // 【窗口 1: 开启蓝牙广播 (25ms)】
        slot_start = xTaskGetTickCount();
        esp_ble_gap_ext_adv_start(1, &ext_adv);
        vTaskDelayUntil(&slot_start, pdMS_TO_TICKS(25));

        // 【窗口 2: 发送组合报文 (50ms)】
        slot_start = xTaskGetTickCount();
        if(!ble_5_0_payload_send(ble_get_ext_adv_handle(), &rid_data, &msg_counter)) {
            ESP_LOGE(TAG, "组合报文发送失败");
        }
        vTaskDelayUntil(&slot_start, pdMS_TO_TICKS(50));

        // 【窗口 3: 发送多个单独报文 (300ms)】
        // 注：ble_5_0_payload_send_step 内有 4 次 50ms 延时，即 200ms
        // 使用 vTaskDelayUntil 能够精准消化掉函数执行时间和内部延时误差，凑满 300ms 窗口
        slot_start = xTaskGetTickCount();
        if(!ble_5_0_payload_send_step(ble_get_ext_adv_handle(), &rid_data, &msg_counter)) {
            ESP_LOGE(TAG, "分体 payload 更新失败");
        }
        vTaskDelayUntil(&slot_start, pdMS_TO_TICKS(300));

        // 【窗口 4: 关闭蓝牙广播 (25ms)】
        // 挂起蓝牙，防止其与 Wi-Fi NAN 帧底层硬件资源争夺
        slot_start = xTaskGetTickCount();
        esp_ble_gap_ext_adv_stop(1, &ext_adv_inst);
        vTaskDelayUntil(&slot_start, pdMS_TO_TICKS(25));

        // 【窗口 5: 发送NAN帧 + 更新Beacon IE (100ms)】
        slot_start = xTaskGetTickCount();
        // 更新 Beacon Vendor IE（硬件自动随下次 Beacon 广播）
        wifi_update_beacon_ie(&beacon_packet, &rid_data);
        // 更新 NAN 载荷并突发发送
        wifi_nan_set_rid_payload(&nan_packet, &rid_data);
        for (int i = 0; i < 10; i++) {
            wifi_send_nan_frame(&nan_packet);
            vTaskDelay(pdMS_TO_TICKS(8));
        }
        ESP_LOGI(TAG, "TDM 循环完毕，tick=%u", tick);

        // 补齐 500ms，严格保证 2Hz 周期
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
