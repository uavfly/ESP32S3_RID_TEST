#include "wifi_beacon.h"

#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"

#define TAG "RID_BEACON"


/**
 * @brief 将最新 RID payload 更新到 AP Beacon 的 Vendor-Specific IE
 *
 * 符合 ASTM F3411-22a Wi-Fi Beacon 传输层规范：
 *   OUI = 0xFA 0x0B 0xBC (ASTM International)，OUI Type = 0x0D (Open Drone ID Remote ID)
 * AP 以 100 TU 间隔自动发送 Beacon，每帧均携带最新 RID payload。
 * 此函数只需在 payload 更新时调用一次，硬件层面持续广播无需重复注入。
 */
bool wifi_update_beacon_ie(WiFi_Beacon_packet *beacon_packet, RID_Data *rid_data)
{
    if (!beacon_packet || !rid_data) {
        return false;
    }

    RIDPayload payload_buffer = rid_data->get_payload();
    uint8_t payload_len = payload_buffer.length();
    if (payload_len == 0) return false;
    if (payload_len > 250) {
        ESP_LOGE(TAG, "RID payload 长度非法: %u", payload_len);
        return false;
    }

    beacon_packet->buf[0] = 0xDD;                        // Element ID: Vendor Specific
    beacon_packet->buf[1] = (uint8_t)(5 + payload_len);  // length = OUI(3) + type(1) + appcode(1) + payload
    beacon_packet->buf[2] = 0xFA;                        // ASTM OUI byte 0
    beacon_packet->buf[3] = 0x0B;                        // ASTM OUI byte 1
    beacon_packet->buf[4] = 0xBC;                        // ASTM OUI byte 2
    beacon_packet->buf[5] = 0x0D;                        // OUI Type: Open Drone ID Remote ID
    beacon_packet->buf[6] = 0x0D;                        // App Code: Open Drone ID (ASTM F3411-22a 12.5.2)
    memcpy(&beacon_packet->buf[7], payload_buffer.data, payload_len);

    if (beacon_packet->enabled) {
        esp_err_t disable_ret = esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON,
                                                       WIFI_VND_IE_ID_0, NULL);
        if (disable_ret != ESP_OK) {
            ESP_LOGE(TAG, "禁用 Beacon Vendor IE 失败: %d", disable_ret);
            return false;
        }
        beacon_packet->enabled = false;
    }

    esp_err_t ret = esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON,
                                           WIFI_VND_IE_ID_0, beacon_packet->buf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置 Beacon Vendor IE 失败: %d", ret);
        return false;
    }
    beacon_packet->enabled = true;
    return true;
}
