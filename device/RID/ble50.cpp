#include "ble50.h"

#include <string.h>

#include "esp_gap_ble_api.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "RID_BLE50"

// 将 RIDPayloadBuffer 转换为 BLE AD 广播数据格式
bool ble_5_0_ad_data(const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter, RIDBleADData *ble_data)
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

// 将单个 RID Basic 子帧转换为 BLE AD 广播数据格式（单独封装）
bool ble_5_0_ad_data_basic(const RIDSubframePayloadBuffer *rid_basic, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!rid_basic || !msg_counter || !ble_data) {
        return false;
    }

    uint8_t rid_payload_len = 25; // 25
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + rid_payload_len; // AD Type + UUID + AppCode + Counter + payload = 30
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;       // AD Type: Service Data
    ble_data->data[2] = 0xFA;       // UUID 0xFFFA (Open Drone ID) low byte
    ble_data->data[3] = 0xFF;       // UUID 0xFFFA high byte
    ble_data->data[4] = 0x0D;       // App Code
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    memcpy(&ble_data->data[6], rid_basic->buffer, 25);
    ble_data->length = 1 + ad_payload_len; // 31
    return true;
}

// 将单个 RID Location/Vector 子帧转换为 BLE AD 广播数据格式（单独封装）
bool ble_5_0_ad_data_loc(const RIDSubframePayloadBuffer *rid_loc, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!rid_loc || !msg_counter || !ble_data) {
        return false;
    }

    uint8_t rid_payload_len = 25; // 25
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + rid_payload_len; // 30
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;       // AD Type: Service Data
    ble_data->data[2] = 0xFA;       // UUID 0xFFFA low byte
    ble_data->data[3] = 0xFF;       // UUID 0xFFFA high byte
    ble_data->data[4] = 0x0D;       // App Code
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    memcpy(&ble_data->data[6], rid_loc->buffer, 25);
    ble_data->length = 1 + ad_payload_len; // 31
    return true;
}

// 将单个 RID Operation Description 子帧转换为 BLE AD 广播数据格式（单独封装）
bool ble_5_0_ad_data_rd(const RIDSubframePayloadBuffer *rid_rd, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!rid_rd || !msg_counter || !ble_data) {
        return false;
    }

    uint8_t rid_payload_len = 25; // 25
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + rid_payload_len; // 30
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;       // AD Type: Service Data
    ble_data->data[2] = 0xFA;       // UUID 0xFFFA low byte
    ble_data->data[3] = 0xFF;       // UUID 0xFFFA high byte
    ble_data->data[4] = 0x0D;       // App Code
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    memcpy(&ble_data->data[6], rid_rd->buffer, 25);
    ble_data->length = 1 + ad_payload_len; // 31
    return true;
}

// 将单个 RID System 子帧转换为 BLE AD 广播数据格式（单独封装）
bool ble_5_0_ad_data_sys(const RIDSubframePayloadBuffer *rid_sys, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!rid_sys || !msg_counter || !ble_data) {
        return false;
    }

    uint8_t rid_payload_len = 25; // 25
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + rid_payload_len; // 30
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;       // AD Type: Service Data
    ble_data->data[2] = 0xFA;       // UUID 0xFFFA low byte
    ble_data->data[3] = 0xFF;       // UUID 0xFFFA high byte
    ble_data->data[4] = 0x0D;       // App Code
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    memcpy(&ble_data->data[6], rid_sys->buffer, 25);
    ble_data->length = 1 + ad_payload_len; // 31
    return true;
}

// 将 RIDPayloadBuffer 通过 BLE 5.0 扩展广播发送
bool ble_5_0_payload_send(uint8_t adv_handle, const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter)
{
    RIDBleADData ble_data;
    if (!ble_5_0_ad_data(payload_buffer, msg_counter, &ble_data)) {
        return false;
    }

    esp_err_t ret = esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "更新广播数据失败: %d", ret);
        return false;
    }
    return true;
}

// 分体发送单个子帧数据（Message Pack 封装）
bool ble_5_0_payload_send_step(uint8_t adv_handle, const RIDBasicPacket *rid_basic, const RIDPosVecPacket *rid_pv, const RIDRDPacket *rid_rd, const RIDSYSPacket *rid_sys, uint8_t *msg_counter)
{
    if (rid_basic) {
        RIDBleADData ble_data;
        RIDSubframePayloadBuffer basic_subframe = {0};
        if (RIDBasicSerialize(rid_basic, &basic_subframe)) {
            if (ble_5_0_ad_data_basic(&basic_subframe, msg_counter, &ble_data)) {
                esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
            }
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (rid_pv) {
        RIDBleADData ble_data;
        RIDSubframePayloadBuffer pv_data = {0};
        if (RIDPosVecSerialize(rid_pv, &pv_data)) {
            if (ble_5_0_ad_data_loc(&pv_data, msg_counter, &ble_data)) {
                esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
            }
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    if (rid_rd) {
        RIDBleADData ble_data;
        RIDSubframePayloadBuffer rd_data = {0};
        if (RIDRDSerialize(rid_rd, &rd_data)) {
            if (ble_5_0_ad_data_rd(&rd_data, msg_counter, &ble_data)) {
                esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
            }
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    if (rid_sys) {
        RIDBleADData ble_data;
        RIDSubframePayloadBuffer sys_data = {0};
        if (RIDSYSSerialize(rid_sys, &sys_data)) {
            if (ble_5_0_ad_data_sys(&sys_data, msg_counter, &ble_data)) {
                esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
            }
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    return true;
}

// 将 RID Ble AD Data 通过 BLE 5.0 扩展广播发送
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