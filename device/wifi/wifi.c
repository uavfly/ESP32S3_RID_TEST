#include "wifi.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#define TAG "WIFI"

// Wi-Fi 发送使用的信道（2.4G）
static const uint8_t WIFI_TX_CHANNEL = 6;

/**
 * @brief 初始化 Wi-Fi 并启用 802.11 原始管理帧发送
 *
 * 使用 AP 模式：信道固定不漂移，WIFI_IF_AP 接口允许发送任意目标 MAC
 * 的管理帧（STA 模式有广播过滤且会后台扫描切换信道）。
 */
void wifi_raw_tx_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    // APSTA 模式：保留 AP 固定信道发送能力，同时预留 STA 侧能力
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid          = "RID_RAW",
            .ssid_len      = 7,
            .channel       = WIFI_TX_CHANNEL,
            .password      = "",
            .max_connection = 0,
            .authmode      = WIFI_AUTH_OPEN,
            .ssid_hidden   = 1,      // 隐藏内置 SSID，减少干扰
            .beacon_interval = 100,   // Beacon 间隔 100 TU ≈ 102.4ms（Remote ID 标准默认值）
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi Raw TX 初始化完成，APSTA 模式，信道=%u，Beacon IE 方式", WIFI_TX_CHANNEL);
}