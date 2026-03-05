#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "RID_Payload.h"

// BLE AD DATA 数据结构
typedef struct {
	uint8_t data[512];
	uint8_t length;
} RIDBleADData;

// 将 RIDPayloadBuffer 转换为 BLE AD 广播数据格式
bool ble_5_0_ad_data(const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter, RIDBleADData *ble_data);
// 将 RIDPayloadBuffer 通过 BLE 5.0 扩展广播发送
bool ble_5_0_payload_send(uint8_t adv_handle, const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter);
// 将 RID Ble AD Data 通过 BLE 5.0 扩展广播发送
bool ble_5_0_ad_send(uint8_t adv_handle, RIDBleADData *ble_data);

