#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "RID_Payload.h"

typedef struct {
	uint8_t buf[2 + 4 + 256];
	bool enabled;
} WiFi_Beacon_packet;

bool wifi_update_beacon_ie(WiFi_Beacon_packet *beacon_packet, const RIDPayloadBuffer *payload_buffer);

