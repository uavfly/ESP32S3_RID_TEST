#include "wifi_nan.h"

#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"

#define TAG "RID_NAN"

/**
 * @brief 构造 NAN Vendor Specific Public Action 帧模板（后续会动态写入 SA/BSSID）
 */
void wifi_nan_build_template(WiFi_NAN_packet *packet)
{
    if (!packet) {
        return;
    }

    // NAN 常见封装：Public Action + Vendor Specific + Wi-Fi Alliance OUI(50:6F:9A)
    // 这里提供可发送的基础模板，便于抓包验证。
    static const uint8_t template_data[] = {
        // MAC Header (24)
        0xd0, 0x00,                         // Frame Control: Action
        0x00, 0x00,                         // Duration
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // DA: Broadcast
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SA: to fill
        0x50, 0x6f, 0x9a, 0x01, 0x00, 0x00, // BSSID: NAN BSSID 示例（可替换）
        0x00, 0x00,                         // Seq Ctrl (可由系统覆盖)

        // Public Action body
        0x04,                               // Category: Public Action
        0x09,                               // Action: Vendor Specific
        0x50, 0x6f, 0x9a,                   // OUI: Wi-Fi Alliance
        0x13,                               // OUI Type: NAN
        0x00,                               // NAN subtype (demo)
        0x01,                               // NAN version/control (demo)
        0x00, 0x00,                         // 简化 payload（demo）
    };

    packet->length = sizeof(template_data);
    memcpy(packet->frame, template_data, packet->length);
}

// 将 NAN Vendor Specific Public Action 帧模板中的 SA 和 BSSID 替换为指定 MAC 地址
void wifi_nan_set_source_mac(WiFi_NAN_packet *packet, const uint8_t mac[6])
{
    if (!packet || !mac) {
        return;
    }

    memcpy(&packet->frame[10], mac, 6);
    memcpy(&packet->frame[16], mac, 6);
}

/**
 * @brief 发送一帧 NAN Action
 */
bool wifi_send_nan_frame(const WiFi_NAN_packet *packet)
{
    if (!packet || packet->length == 0) {
        return false;
    }

    esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_AP, packet->frame, packet->length, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "发送 NAN 帧失败: %d", ret);
        return false;
    }
    return true;
}