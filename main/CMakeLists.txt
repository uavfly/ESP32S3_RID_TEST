idf_component_register(
    SRCS
        "main.cpp"
        "../device/ble/ble.c"
        "../device/wifi/wifi.c"
        "../device/RID/RID_Payload.cpp"
        "../device/RID/ble50.cpp"
        "../device/RID/wifi_beacon.cpp"
        "../device/RID/wifi_nan.cpp"
    INCLUDE_DIRS
        "."
        "../include"
        "../device/ble"
        "../device/wifi"
        "../device/RID"
    REQUIRES nvs_flash bt esp_wifi esp_event esp_netif
)