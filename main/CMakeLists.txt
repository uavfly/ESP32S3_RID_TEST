idf_component_register(SRCS "main.c" "RID_Payload.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash bt esp_wifi esp_event esp_netif)