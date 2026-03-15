#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "RID_Payload.hpp"

// BLE AD DATA 数据结构
typedef struct {
	uint8_t data[512];
	uint16_t length;
} RIDBleADData;

// 将 RID 完整 payload 转换为 BLE AD 广播数据格式
bool ble_5_0_ad_data_payload(RID_Data *rid_data, uint8_t *msg_counter, RIDBleADData *ble_data);

// 将单个子帧转换为 BLE AD 广播数据格式（Message Pack 封装）
bool ble_5_0_ad_data_subframe(const RIDSubframePayloadBuffer *subframe, uint8_t *msg_counter, RIDBleADData *ble_data);

// 将设备数据分体通过 BLE 5.0 扩展广播发送
bool ble_5_0_payload_send_step(uint8_t adv_handle, RID_Data *rid_data, uint8_t *msg_counter);

// 将设备数据聚合 Payload 通过 BLE 5.0 扩展广播发送
bool ble_5_0_payload_send(uint8_t adv_handle, RID_Data *rid_data, uint8_t *msg_counter);

// 将 RID Ble AD Data 通过 BLE 5.0 扩展广播直接下发
bool ble_5_0_ad_send(uint8_t adv_handle, RIDBleADData *ble_data);

