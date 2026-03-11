#include "ble50.h"

#include <string.h>

#include "esp_gap_ble_api.h"
#include "esp_log.h"

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

// 将单个 RID Basic 子帧转换为 BLE AD 广播数据格式（Message Pack 封装）
bool ble_5_0_ad_data_basic(const RIDSubframePayloadBuffer *rid_basic, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!rid_basic || !msg_counter || !ble_data) {
        return false;
    }

    // Message Pack: [0xF1] [0x19] [count=1] + 1×25字节子帧 = 28字节
    uint8_t rid_payload_len = 3 + 1 * 25; // 28
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + rid_payload_len; // AD Type + UUID + AppCode + Counter + payload = 33
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;       // AD Type: Service Data
    ble_data->data[2] = 0xFA;       // UUID 0xFFFA (Open Drone ID) low byte
    ble_data->data[3] = 0xFF;       // UUID 0xFFFA high byte
    ble_data->data[4] = 0x0D;       // App Code
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    // Message Pack 报头
    ble_data->data[6] = 0xF1;       // Header: 类型0xF | 版本0x1
    ble_data->data[7] = 0x19;       // SubPacketLength: 25
    ble_data->data[8] = 0x01;       // SubPacketCount: 1
    // 子帧数据
    memcpy(&ble_data->data[9], rid_basic->RIDSubframePayloadBuffer, 25);
    ble_data->length = 1 + ad_payload_len; // 34
    return true;
}

// 将单个 RID Location/Vector 子帧转换为 BLE AD 广播数据格式（Message Pack 封装）
bool ble_5_0_ad_data_loc(const RIDSubframePayloadBuffer *rid_loc, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!rid_loc || !msg_counter || !ble_data) {
        return false;
    }

    uint8_t rid_payload_len = 3 + 1 * 25; // 28
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + rid_payload_len; // 33
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;       // AD Type: Service Data
    ble_data->data[2] = 0xFA;       // UUID 0xFFFA low byte
    ble_data->data[3] = 0xFF;       // UUID 0xFFFA high byte
    ble_data->data[4] = 0x0D;       // App Code
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    ble_data->data[6] = 0xF1;       // Header: 类型0xF | 版本0x1
    ble_data->data[7] = 0x19;       // SubPacketLength: 25
    ble_data->data[8] = 0x01;       // SubPacketCount: 1
    memcpy(&ble_data->data[9], rid_loc->RIDSubframePayloadBuffer, 25);
    ble_data->length = 1 + ad_payload_len; // 34
    return true;
}

// 将单个 RID Operation Description 子帧转换为 BLE AD 广播数据格式（Message Pack 封装）
bool ble_5_0_ad_data_rd(const RIDSubframePayloadBuffer *rid_rd, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!rid_rd || !msg_counter || !ble_data) {
        return false;
    }

    uint8_t rid_payload_len = 3 + 1 * 25; // 28
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + rid_payload_len; // 33
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;       // AD Type: Service Data
    ble_data->data[2] = 0xFA;       // UUID 0xFFFA low byte
    ble_data->data[3] = 0xFF;       // UUID 0xFFFA high byte
    ble_data->data[4] = 0x0D;       // App Code
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    ble_data->data[6] = 0xF1;       // Header: 类型0xF | 版本0x1
    ble_data->data[7] = 0x19;       // SubPacketLength: 25
    ble_data->data[8] = 0x01;       // SubPacketCount: 1
    memcpy(&ble_data->data[9], rid_rd->RIDSubframePayloadBuffer, 25);
    ble_data->length = 1 + ad_payload_len; // 34
    return true;
}

// 将单个 RID System 子帧转换为 BLE AD 广播数据格式（Message Pack 封装）
bool ble_5_0_ad_data_sys(const RIDSubframePayloadBuffer *rid_sys, uint8_t *msg_counter, RIDBleADData *ble_data)
{
    if (!rid_sys || !msg_counter || !ble_data) {
        return false;
    }

    uint8_t rid_payload_len = 3 + 1 * 25; // 28
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + rid_payload_len; // 33
    ble_data->data[0] = ad_payload_len;
    ble_data->data[1] = 0x16;       // AD Type: Service Data
    ble_data->data[2] = 0xFA;       // UUID 0xFFFA low byte
    ble_data->data[3] = 0xFF;       // UUID 0xFFFA high byte
    ble_data->data[4] = 0x0D;       // App Code
    ble_data->data[5] = *msg_counter;
    (*msg_counter)++;
    ble_data->data[6] = 0xF1;       // Header: 类型0xF | 版本0x1
    ble_data->data[7] = 0x19;       // SubPacketLength: 25
    ble_data->data[8] = 0x01;       // SubPacketCount: 1
    memcpy(&ble_data->data[9], rid_sys->RIDSubframePayloadBuffer, 25);
    ble_data->length = 1 + ad_payload_len; // 34
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
bool ble_5_0_payload_send_step(uint8_t adv_handle, const RIDBasicPacket *rid_basic, const RID *rid_loc, const RIDSubframePayloadBuffer *rid_rd, const RIDSubframePayloadBuffer *rid_sys, uint8_t *msg_counter)
{
    if (rid_basic) {
        RIDBleADData ble_data;
        if (ble_5_0_ad_data_basic(rid_basic, msg_counter, &ble_data)) {
            esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
        }
    }
    if (rid_loc) {
        RIDBleADData ble_data;
        if (ble_5_0_ad_data_loc(rid_loc, msg_counter, &ble_data)) {
            esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
        }
    }
    if (rid_rd) {
        RIDBleADData ble_data;
        if (ble_5_0_ad_data_rd(rid_rd, msg_counter, &ble_data)) {
            esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
        }
    }
    if (rid_sys) {
        RIDBleADData ble_data;
        if (ble_5_0_ad_data_sys(rid_sys, msg_counter, &ble_data)) {
            esp_ble_gap_config_ext_adv_data_raw(adv_handle, ble_data.length, ble_data.data);
        }
    }
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