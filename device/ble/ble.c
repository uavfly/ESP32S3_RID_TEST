#include "ble.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"

static const uint8_t EXT_ADV_HANDLE = 0;

static const esp_ble_gap_ext_adv_params_t ext_adv_params = {
    .type            = ESP_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED,
    .interval_min    = 0x0140,
    .interval_max    = 0x0140,
    .channel_map     = ADV_CHNL_ALL,
    .own_addr_type   = BLE_ADDR_TYPE_PUBLIC,
    .filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .tx_power        = EXT_ADV_TX_PWR_NO_PREFERENCE,
    .primary_phy     = ESP_BLE_GAP_PHY_CODED,
    .max_skip        = 0,
    .secondary_phy   = ESP_BLE_GAP_PHY_CODED,
    .sid             = 0,
    .scan_req_notif  = false,
};

void ble_stack_init(esp_gap_ble_cb_t gap_cb)
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_cb));
}

uint8_t ble_get_ext_adv_handle(void)
{
    return EXT_ADV_HANDLE;
}

const esp_ble_gap_ext_adv_params_t *ble_get_ext_adv_params(void)
{
    return &ext_adv_params;
}
