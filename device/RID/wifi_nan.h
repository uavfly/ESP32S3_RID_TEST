#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint8_t frame[64];
	uint8_t length;
} WiFi_NAN_packet;

void wifi_nan_build_template(WiFi_NAN_packet *packet);
void wifi_nan_set_source_mac(WiFi_NAN_packet *packet, const uint8_t mac[6]);
bool wifi_send_nan_frame(const WiFi_NAN_packet *packet);

