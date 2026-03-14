#pragma once

#include <stdint.h>
#include "esp_gap_ble_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// BLE 栈初始化，注册 GAP 回调函数
void ble_stack_init(esp_gap_ble_cb_t gap_cb);
// 获取 BLE 扩展广播实例句柄
uint8_t ble_get_ext_adv_handle(void);
// 获取 BLE 扩展广播参数配置
const esp_ble_gap_ext_adv_params_t *ble_get_ext_adv_params(void);

#ifdef __cplusplus
}
#endif

