#include "RID_Payload.hpp"

// UASID字符串序列化
void RIDUasIDSerialize(const char *str, RIDBasicPacket *packet)
{
    if (!str || !packet) return;
    uint8_t i = 0;
    while (i < 20 && str[i] != '\0') {
        packet->UASID[i] = (uint8_t)str[i];
        i++;
    }
    // 不足部分补零
    memset(packet->UASID + i, 0, 20 - i);
}

// 描述内容字符串序列化
void RIDDescription(const char *str, RIDRDPacket *packet)
{
    if (!str || !packet) return;
    uint8_t i = 0;
    while (i < 23 && str[i] != '\0') {
        packet->Description[i] = (uint8_t)str[i];
        i++;
    }
    // 不足部分补零
    memset(packet->Description + i, 0, 23 - i);
}
