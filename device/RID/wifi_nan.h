#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "RID_Payload.hpp"

// NAN 帧头长度: MAC(24) + Category(1) + Action(1) + OUI(3) + OUI_Type(1) = 30
#define WIFI_NAN_HDR_LEN  30

// Wi-Fi NAN 数据结构
typedef struct {
	uint8_t  frame[300];  // 帧头(30) + RID 载荷(最大253) = 283
	uint16_t length;
} WiFi_NAN_packet;

// 构造 NAN Vendor Specific Public Action 帧模板（后续会动态写入 SA/BSSID）
void wifi_nan_build_template(WiFi_NAN_packet *packet);
// 将 NAN Vendor Specific Public Action 帧模板中的 SA 和 BSSID 替换为指定 MAC 地址
void wifi_nan_set_source_mac(WiFi_NAN_packet *packet, const uint8_t mac[6]);
// 将 RID 载荷写入 NAN 帧体（OUI Type 之后）
bool wifi_nan_set_rid_payload(WiFi_NAN_packet *packet, const RIDPayloadBuffer *payload);
// 发送一帧 NAN Action
bool wifi_send_nan_frame(const WiFi_NAN_packet *packet);

