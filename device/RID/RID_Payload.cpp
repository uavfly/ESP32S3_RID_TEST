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

// 基本ID报文序列化
bool RIDBasicSerialize(const RIDBasicPacket *packet, RIDSubframePayloadBuffer *buf)
{
    if (!packet || !buf) return false;
    memcpy(buf->buffer, packet, 25);
    return true;
}

// RID位置向量报文序列化
bool RIDPosVecSerialize(const RIDPosVecPacket *packet, RIDSubframePayloadBuffer *buf)
{
    if (!packet || !buf) return false;
    memcpy(buf->buffer, packet, 25);
    return true;
}

// RID运行描述报文序列化
bool RIDRDSerialize(const RIDRDPacket *packet, RIDSubframePayloadBuffer *buf)
{
    if (!packet || !buf) return false;
    memcpy(buf->buffer, packet, 25);
    return true;
}

// RID系统报文序列化
bool RIDSYSSerialize(const RIDSYSPacket *packet, RIDSubframePayloadBuffer *buf)
{
    if (!packet || !buf) return false;
    memcpy(buf->buffer, packet, 25);
    return true;
}

// RID Payload 序列化
bool RIDPayloadSerialize(const RIDPayload *payload, RIDPayloadBuffer *buf)
{
    if (!payload || !buf) return false;
    uint16_t len = payload->length();
    if (len > 253) return false;
    memcpy(buf->RIDPayloadBuffer, payload->data, len);
    buf->length = (uint8_t)len;
    return true;
}
