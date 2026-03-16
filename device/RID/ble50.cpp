#include "ble50.h"

#include <string.h>

#include "esp_gap_ble_api.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "RID_BLE50"

// 将 RID 完整 Payload 转换为 BLE AD 广播数据格式
bool ble_5_0_ad_data_payload(RID_Data *rid_data, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!rid_data || !msg_counter || !ble_data) {
        return false;
    }

    RIDPayload payload = rid_data->get_payload();
    uint16_t payload_len = payload.length();
    
    if (payload_len == 0 || payload_len > 600) {
        ESP_LOGE(TAG, "RID payload 长度非法: %u", payload_len);
        return false;
    }

    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + payload_len;
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;
    ble_data->data[2] = 0xFA;
    ble_data->data[3] = 0xFF;
    ble_data->data[4] = 0x0D;
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    memcpy(&ble_data->data[6], payload.data, payload_len);
    ble_data->length = 1 + ad_payload_len;
    return true;
}

// 将单个子帧转换为 BLE AD 广播数据格式（通用 Message Pack 封装）
bool ble_5_0_ad_data_subframe(const RIDSubframePayloadBuffer *subframe, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!subframe || !msg_counter || !ble_data) {
        return false;
    }

    uint8_t rid_payload_len = 25; 
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + rid_payload_len; 
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;       // AD Type: Service Data
    ble_data->data[2] = 0xFA;       // UUID 0xFFFA (Open Drone ID) low byte
    ble_data->data[3] = 0xFF;       // UUID 0xFFFA high byte
    ble_data->data[4] = 0x0D;       // App Code
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    memcpy(&ble_data->data[6], subframe->buffer, 25);
    ble_data->length = 1 + ad_payload_len; 
    return true;
}

// 将 RIDPayload 通过 BLE 5.0 扩展广播完整串发
bool ble_5_0_payload_send(uint8_t adv_handle, RID_Data *rid_data, uint8_t *msg_counter)
{
    if (!rid_data || !msg_counter) {
        return false;
    }

    RIDBleADData ble_data;
    if (!ble_5_0_ad_data_payload(rid_data, msg_counter, &ble_data)) {
        return false;
    }

    esp_err_t ret = esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "更新广播数据失败: %d", ret);
        return false;
    }
    return true;
}

// 分体发送单个子帧数据，通过 RID_Data::get_payload() 获取序列化后的子帧再逐个发送
bool ble_5_0_payload_send_step(uint8_t adv_handle, RID_Data *rid_data, uint8_t *msg_counter)
{
    if (!rid_data || !msg_counter) {
        return false;
    }

    // get_payload() 内部调用 fixheader() 并通过 union 完成序列化
    RIDPayload payload = rid_data->get_payload();
    uint8_t count = payload.SubPacketCount;
    if (count == 0) {
        return true;
    }

    // 根据子帧数量动态计算每帧延时，确保总时长适配 300ms 窗口
    uint32_t delay_ms = 300 / count;
    if (delay_ms < 30) delay_ms = 30;

    RIDBleADData ble_data;
    for (int i = 0; i < count; i++) {
        if (ble_5_0_ad_data_subframe(&payload.Buffer[i], msg_counter, &ble_data)) {
            esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    return true;
}

// 将 RID Ble AD Data 通过 BLE 5.0 扩展广播直接发送
bool ble_5_0_ad_send(uint8_t adv_handle, RIDBleADData *ble_data)
{
    if (!ble_data) {
        return false;
    }

    esp_err_t ret = esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data->length, ble_data->data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "更新广播数据失败: %d", ret);
        return false;
    }
    return true;
}