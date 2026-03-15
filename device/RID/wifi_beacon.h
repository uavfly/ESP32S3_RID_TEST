#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "RID_Payload.hpp"

// Wi-Fi Beacon 广播数据结构
typedef struct {
	uint8_t buf[2 + 4 + 256];
	bool enabled;
} WiFi_Beacon_packet;

// 将最新 RID payload 更新到 AP Beacon 的 Vendor-Specific IE
bool wifi_update_beacon_ie(WiFi_Beacon_packet *beacon_packet, RID_Data *rid_data);

