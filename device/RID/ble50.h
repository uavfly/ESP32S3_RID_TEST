#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "RID_Payload.h"

typedef struct {
	uint8_t data[512];
	uint8_t length;
} RIDBleADData;

bool ble_ad_data(const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter, RIDBleADData *ble_data);
bool ble_5_0_payload_send(uint8_t adv_handle, const RIDPayloadBuffer *payload_buffer, uint8_t *msg_counter);

