#include "ble50.h"

#include <string.h>

#include "esp_gap_ble_api.h"
#include "esp_log.h"

#define TAG "RID_BLE50"

bool ble_ad_data(const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!payload_buffer || !msg_counter || !ble_data) {
        return false;
    }

    uint8_t payload_len = payload_buffer->length;
    if (payload_len == 0 || payload_len > 250) {
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
    memcpy(&ble_data->data[6], payload_buffer->RIDPayloadBuffer, payload_len);
    ble_data->length = 1 + ad_payload_len;
    return true;
}

bool ble_5_0_payload_send(uint8_t adv_handle, const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter)
{
    RIDBleADData ble_data;
    if (!ble_ad_data(payload_buffer, msg_counter, &ble_data)) {
        return false;
    }

    esp_err_t ret = esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "更新广播数据失败: %d", ret);
        return false;
    }
    return true;
}
