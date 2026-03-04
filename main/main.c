#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "RID_Payload.h"
#include "nvs_flash.h"

#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_err.h>
#include <esp_bt.h>

#define TAG "RID"

// BLE 广播句柄号，固定为 0
static const uint8_t EXT_ADV_HANDLE = 0;

// UUID 0xFFFA 小端序
#define RID_UUID_LSB 0xFA
#define RID_UUID_MSB 0xFF

// RID 数据结构体
static RIDBasicPacket    basicPacket;
static RIDPosVecPacket   posVecPacket;
static RIDRDPacket       rdPacket;
static RIDSYSPacket      sysPacket;
static RIDPayloadBuffer  payloadBuffer;

// AD 数据缓冲区：AD Length(1) + AD Type(1) + UUID(2) + AppCode(1) + Counter(1) + RID Payload最大253字节
static uint8_t adv_data[259];
static uint16_t adv_data_len = 0;

// ASTM F3411-22a Message Counter (0~255 递增)
static uint8_t msg_counter = 0;

// 广播已启动标志（由 GAP 回调置位）
static volatile bool adv_started = false;

// 扩展广播参数（Coded PHY S=8，非可连接非可扫描）
static esp_ble_gap_ext_adv_params_t ext_adv_params = {
    .type            = ESP_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED,
    .interval_min    = 0x0140,  // 200ms
    .interval_max    = 0x0140,  // 200ms（固定间隔，减少接收端“卡顿”感）
    .channel_map     = ADV_CHNL_ALL,
    .own_addr_type   = BLE_ADDR_TYPE_PUBLIC,
    .filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .tx_power        = EXT_ADV_TX_PWR_NO_PREFERENCE,
    .primary_phy     = ESP_BLE_GAP_PHY_CODED,   // 主信道 Coded PHY
    .max_skip        = 0,
    .secondary_phy   = ESP_BLE_GAP_PHY_CODED,   // 数据信道 Coded PHY (S=8)
    .sid             = 0,
    .scan_req_notif  = false,
};

// 扩展广播实例
static esp_ble_gap_ext_adv_t ext_adv = {
    .instance   = 0,
    .duration   = 0,    // 持续广播（不超时）
    .max_events = 0,    // 不限制事件次数
};

// Wi-Fi 发送使用的信道（2.4G）
static const uint8_t WIFI_TX_CHANNEL = 6;

// Wi-Fi Beacon Vendor IE 缓冲区（ASTM F3411-22a：element_id + length + OUI(3) + type + RID payload）

typedef struct{
    uint8_t buf[2+4+256];
    bool enabled;
}WiFi_Beacon_packet;

// Wi-Fi NAN Action 发送缓冲区（Vendor Specific Public Action）

typedef struct{
    uint8_t frame[64];
    uint8_t length;
}WiFi_NAN_packet;

WiFi_NAN_packet nan_packet;

WiFi_Beacon_packet beacon_packet;
/**
 * @brief 构造 NAN Vendor Specific Public Action 帧模板（后续会动态写入 SA/BSSID）
 */
static void build_wifi_nan_template(void)
{
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

    wifi_nan_len = sizeof(template_data);
    memcpy(wifi_nan_frame, template_data, wifi_nan_len);
}

/**
 * @brief 用当前 STA MAC 地址填充 Beacon/NAN 帧的 SA 与 BSSID
 */
static void patch_wifi_frame_addr(void)
{
    uint8_t mac[6] = {0};
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, mac));

    // NAN Action: SA[10..15]
    memcpy(&wifi_nan_frame[10], mac, 6);
}

/**
 * @brief 初始化 Wi-Fi 并启用 802.11 原始管理帧发送
 *
 * 使用 AP 模式：信道固定不漂移，WIFI_IF_AP 接口允许发送任意目标 MAC
 * 的管理帧（STA 模式有广播过滤且会后台扫描切换信道）。
 */
static void wifi_raw_tx_init(void)
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

    build_wifi_nan_template();
    patch_wifi_frame_addr();

    ESP_LOGI(TAG, "Wi-Fi Raw TX 初始化完成，APSTA 模式，信道=%u，Beacon IE 方式", WIFI_TX_CHANNEL);
}

/**
 * @brief 将最新 RID payload 更新到 AP Beacon 的 Vendor-Specific IE
 *
 * 符合 ASTM F3411-22a Wi-Fi Beacon 传输层规范：
 *   OUI = 0xFA 0x0B 0xBC (ASTM International)，OUI Type = 0x0D (Open Drone ID Remote ID)
 * AP 以 100 TU 间隔自动发送 Beacon，每帧均携带最新 RID payload。
 * 此函数只需在 payload 更新时调用一次，硬件层面持续广播无需重复注入。
 */
bool wifi_update_beacon_ie(WiFi_Beacon_packet beacon_packet, RIDPayloadBuffer payloadBuffer)
{
    uint8_t payload_len = payloadBuffer.length;
    if (payload_len == 0) return false;
    if (payload_len > 250) {
        ESP_LOGE(TAG, "RID payload 长度非法: %u", payload_len);
        return false;
    }

    beacon_packet.buf[0] = 0xDD;                        // Element ID: Vendor Specific
    beacon_packet.buf[1] = (uint8_t)(5 + payload_len);  // length = OUI(3) + type(1) + appcode(1) + payload
    beacon_packet.buf[2] = 0xFA;                        // ASTM OUI byte 0
    beacon_packet.buf[3] = 0x0B;                        // ASTM OUI byte 1
    beacon_packet.buf[4] = 0xBC;                        // ASTM OUI byte 2
    beacon_packet.buf[5] = 0x0D;                        // OUI Type: Open Drone ID Remote ID
    beacon_packet.buf[6] = 0x0D;                        // App Code: Open Drone ID (ASTM F3411-22a 12.5.2)
    memcpy(&beacon_packet.buf[7], payloadBuffer.RIDPayloadBuffer, payload_len);

    if (beacon_packet.enabled) {
        esp_err_t disable_ret = esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON,
                                                       WIFI_VND_IE_ID_0, NULL);
        if (disable_ret != ESP_OK) {
            ESP_LOGE(TAG, "禁用 Beacon Vendor IE 失败: %d", disable_ret);
            return false;
        }
        beacon_packet.enabled = false;
    }

    esp_err_t ret = esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON,
                                           WIFI_VND_IE_ID_0, beacon_packet.buf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置 Beacon Vendor IE 失败: %d", ret);
        return false;
    }
    beacon_packet.enabled = true;
    return true;
}

/**
 * @brief 发送一帧 NAN Action
 */
static void wifi_send_nan_frame(void)
{
    esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_AP, wifi_nan_frame, wifi_nan_len, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "发送 NAN 帧失败: %d", ret);
    }
}

typedef struct
{
    uint8_t data[512];
    uint8_t length;
}RIDBleADData;


/**
 * @brief 将 payloadBuffer 打包为 BLE AD Data
 *
 * ASTM F3411-22a BLE 传输层格式:
 *   [AD Length][AD Type=0x16][UUID 0xFFFA LE][AppCode=0x0D][MsgCounter][RID Payload]
 */
static void build_adv_data(void)
{
    uint8_t payload_len = payloadBuffer.length;
    // AD Data = [AD Type 0x16][UUID LSB][UUID MSB][AppCode 0x0D][Counter][RID Payload]
    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + payload_len;  // Type + UUID + AppCode + Counter + Payload
    adv_data[0] = ad_payload_len;                   // AD Length
    adv_data[1] = 0x16;                             // AD Type: Service Data
    adv_data[2] = RID_UUID_LSB;                     // UUID 0xFFFA LE
    adv_data[3] = RID_UUID_MSB;
    adv_data[4] = 0x0D;                             // Application Code: Open Drone ID
    adv_data[5] = msg_counter++;                    // Message Counter (0~255, 自动回绕)
    memcpy(&adv_data[6], payloadBuffer.RIDPayloadBuffer, payload_len);
    adv_data_len = 1 + ad_payload_len;              // Length字段本身(1B) + AD内容
}

bool ble_ad_data(RIDPayloadBuffer payloadBuffer, RIDBleADData *bledata){
    uint8_t payload_len = payloadBuffer.length;
    if (payload_len == 0) return false;
    if (payload_len > 250) {
        ESP_LOGE(TAG, "RID payload 长度非法: %u", payload_len);
        return false;
    }

    uint8_t ad_payload_len = 1 + 2 + 1 + 1 + payload_len;  // Type + UUID + AppCode + Counter + Payload
    bledata->data[0] = ad_payload_len;                // AD Length
    bledata->data[1] = 0x16;                        // AD Type: Service Data
    bledata->data[2] = 0xFA;                        // UUID 0xFFFA LE
    bledata->data[3] = 0xFF;
    bledata->data[4] = 0x0D;                        // Application Code: Open Drone ID
    bledata->data[5] = msg_counter++;               // Message Counter (0~255, 自动回绕)
    memcpy(&bledata->data[6], payloadBuffer.RIDPayloadBuffer, payload_len);
    bledata->length = 1 + ad_payload_len;           // Length字段本身(1B) + AD内容
    return true;
}

/**
 * @brief GAP 事件回调
 */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
        if (param->ext_adv_set_params.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "设置广播参数失败: %d", param->ext_adv_set_params.status);
            break;
        }
        // 参数设置成功，设置广播数据
        ESP_ERROR_CHECK(esp_ble_gap_config_ext_adv_data_raw(
            EXT_ADV_HANDLE, adv_data_len, adv_data));
        break;

    case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        if (param->ext_adv_data_set.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "设置广播数据失败: %d", param->ext_adv_data_set.status);
            break;
        }
        // 仅首次数据设置完成后启动广播，运行中更新数据无需重启
        if (!adv_started) {
            ESP_ERROR_CHECK(esp_ble_gap_ext_adv_start(1, &ext_adv));
        }
        break;

    case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
        if (param->ext_adv_start.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "BLE 广播启动失败: %d", param->ext_adv_start.status);
        } else {
            adv_started = true;
            ESP_LOGI(TAG, "BLE 5.0 RID 广播已启动 (Coded PHY S=8)");
        }
        break;

    case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, "BLE 广播已停止");
        break;

    default:
        break;
    }
}

/**
 * @brief 初始化 Bluedroid BLE 协议栈
 */
static void ble_init(void)
{
    // 释放经典蓝牙内存，仅使用 BLE
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // 初始化 BT 控制器
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    // 初始化 Bluedroid
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // 注册 GAP 回调
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));

    ESP_LOGI(TAG, "BLE 初始化完成");
}

/**
 * @brief RID 报文数据填充（静态部分，启动时调用一次）
 */
static void rid_setup(void)
{
    basicPacket.UA_Type = 2; // UA类型2（旋翅）
    basicPacket.ID_Type = 1; // ID类型1（产品唯一识别码）
    RIDUasIDSerialize("TEST_UASID_12345", &basicPacket);

    rdPacket.DescriptionType = 0; // 文字描述
    RIDDescription("Test RID Description", &rdPacket);

    sysPacket.ControlStationPosType = 1; // 控制站实时位置
    sysPacket.RegionCode  = 2;           // 中国
    sysPacket.CoordType   = 0;           // WGS84
    sysPacket.ControlStationLatitude  = 350000000;  // 35.0度
    sysPacket.ControlStationLongitude = 1100000000; // 110.0度
    sysPacket.AreaCount         = 0;
    sysPacket.AreaRadius        = 0;
    sysPacket.AreaAltitudeUpper = 0;
    sysPacket.AreaAltitudeLower = 0;
    sysPacket.UARunCategory     = 1; // 开放类
    sysPacket.UARunLevel        = 0; // 微型
    sysPacket.ControlStationAltitude = 0;
    sysPacket.Timestamp = 0;
    sysPacket.reserved  = 0;
}

/**
 * @brief 更新实时位置数据并重新广播（需周期调用）
 *
 * @param lat_1e7   纬度，单位 1e-7 度
 * @param lon_1e7   经度，单位 1e-7 度
 * @param alt_agl   距地高度 m
 * @param ts_tenths 当前小时内 1/10 秒计数
 */
void rid_update(int32_t lat_1e7, int32_t lon_1e7, int16_t alt_agl, uint16_t ts_tenths)
{
    posVecPacket.OperationalStatus = 2; // 空中飞行
    posVecPacket.AltitudeType      = 0; // 使用气压高度
    posVecPacket.TrackAngleEW      = 0;
    posVecPacket.SpeedMultiplier   = 0;
    posVecPacket.TrackAngle        = 0;
    posVecPacket.GroundSpeed       = 0;
    posVecPacket.VerticalSpeed     = 0;
    posVecPacket.Latitude          = lat_1e7;
    posVecPacket.Longitude         = lon_1e7;
    posVecPacket.AltitudeAGL       = alt_agl;
    posVecPacket.PressureAltitude  = 0;
    posVecPacket.GeometricAltitude = 0;
    posVecPacket.Accuracy          = 0;
    posVecPacket.SpeedAccuracy     = 0;
    posVecPacket.Timestamp         = ts_tenths;
    posVecPacket.TimestampAccuracy = 0;
    posVecPacket.reserved          = 0;
    sysPacket.Timestamp            = ts_tenths / 10;
}

bool ble_5_0_payload_send(RIDPayloadBuffer payloadBuffer){
    if (RIDPacket(&basicPacket, &posVecPacket, &rdPacket, &sysPacket, &payloadBuffer)) {
        RIDBleADData bledata;
        if (ble_ad_data(payloadBuffer, &bledata)) {
            esp_err_t ret = esp_ble_gap_config_ext_adv_data_raw(
                EXT_ADV_HANDLE, bledata.length, bledata.data);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "更新广播数据失败: %d", ret);
                return false;
            }
            return true;
        }
    }
    return false;

}

void app_main(void)
{
    // NVS 初始化（Wi-Fi 和 BLE 均需要，必须最先调用）
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化 Wi-Fi Raw TX（Beacon / NAN）
    wifi_raw_tx_init();

    // 初始化 BLE
    ble_init();

    // 填充静态 RID 数据
    rid_setup();

    // 预打包初始广播数据（广播启动前先构建好 adv_data 缓冲区）
    rid_update(300000000, 1200000000, 80, 0);

    // 触发 GAP 回调链：set_params → config_data → ext_adv_start
    ESP_ERROR_CHECK(esp_ble_gap_ext_adv_set_params(EXT_ADV_HANDLE, &ext_adv_params));

    // 等待广播启动后再进入更新循环
    while (!adv_started) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // 主循环：200ms 更新一次 BLE 广播数据，1 秒更新一次 Wi-Fi Beacon/NAN
    // tick 单位为 1/10 秒，每循环 200ms 递增 2，范围 0~35999（1小时内）
    uint16_t tick = 0;
    uint8_t sec_div = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(200));
        tick = (tick + 2) % 36000;
        // TODO: 替换为真实 GNSS 坐标和时间戳
        rid_update(300000000, 1200000000, 80, tick);
        sec_div++;

        // 更新 BLE 广播数据
        ble_5_0_payload_send(payloadBuffer);
        // 每 1 秒刷新一次 Beacon IE，并发送 NAN burst，降低与 BLE 的空口竞争
        if (sec_div >= 5) {
            sec_div = 0;
            // 刷新 Beacon Vendor IE（AP 以 100 TU 自动广播，此处仅更新 IE 内容）
            wifi_update_beacon_ie(&beacon_packet, payloadBuffer);
            // 连续发送 10 帧 NAN Action，帧间间隔 5ms
            for (int i = 0; i < 10; i++) {
                wifi_send_nan_frame();
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            ESP_LOGI(TAG, "RID 广播已更新 tick=%u", tick);
        }
    }
}


