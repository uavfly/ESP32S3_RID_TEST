#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "RID_Payload.hpp"

// BLE AD DATA 数据结构
typedef struct {
	uint8_t data[512];
	uint8_t length;
} RIDBleADData;

// 将 RIDPayloadBuffer 转换为 BLE AD 广播数据格式
bool ble_5_0_ad_data(const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter, RIDBleADData *ble_data);
// 将单个子帧转换为 BLE AD 广播数据格式（Message Pack 封装）
bool ble_5_0_ad_data_basic(const RIDSubframePayloadBuffer *rid_basic, uint8_t *msg_counter, RIDBleADData *ble_data);
bool ble_5_0_ad_data_loc(const RIDSubframePayloadBuffer *rid_loc, uint8_t *msg_counter, RIDBleADData *ble_data);
bool ble_5_0_ad_data_rd(const RIDSubframePayloadBuffer *rid_rd, uint8_t *msg_counter, RIDBleADData *ble_data);
bool ble_5_0_ad_data_sys(const RIDSubframePayloadBuffer *rid_sys, uint8_t *msg_counter, RIDBleADData *ble_data);

bool ble_5_0_payload_send_step(uint8_t adv_handle, const RIDBasicPacket *rid_basic, const RIDPosVecPacket *rid_pv, const RIDRDPacket *rid_rd, const RIDSYSPacket *rid_sys, uint8_t *msg_counter);
// 将 RIDPayloadBuffer 通过 BLE 5.0 扩展广播发送
bool ble_5_0_payload_send(uint8_t adv_handle, const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter);
// 将 RID Ble AD Data 通过 BLE 5.0 扩展广播发送
bool ble_5_0_ad_send(uint8_t adv_handle, RIDBleADData *ble_data);

