#pragma once

#include <stdint.h>
#include "esp_gap_ble_api.h"

void ble_stack_init(esp_gap_ble_cb_t gap_cb);
uint8_t ble_get_ext_adv_handle(void);
const esp_ble_gap_ext_adv_params_t *ble_get_ext_adv_params(void);

