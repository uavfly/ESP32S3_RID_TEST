#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

inline void os_delay(double sec){
    vTaskDelay(pdMS_TO_TICKS(sec * 1000));
}

inline void os_delay_ms(double ms){
    vTaskDelay(pdMS_TO_TICKS(ms));
}