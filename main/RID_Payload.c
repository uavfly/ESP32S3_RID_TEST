#include "RID_Payload.h"

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
    if (!packet || !buf) {
        return false;
    }
    buf->RIDSubframePayloadBuffer[0] = 0x01; // 报头：类型0x0 | 版本0x1
    buf->RIDSubframePayloadBuffer[1] = packet->IDType_UAType; // ID类型和UA类型
    for (int i = 0; i < 20; i++) {
        buf->RIDSubframePayloadBuffer[2 + i] = packet->UASID[i]; // UASID
    }
    for (int i = 0; i < 3; i++) {
        buf->RIDSubframePayloadBuffer[22 + i] = packet->reserved[i]; // 预留
    }
    return true;
}

// RID位置向量报文序列化
bool RIDPosVecSerialize(const RIDPosVecPacket *packet, RIDSubframePayloadBuffer *buf)
{
    if (!packet || !buf) {
        return false;
    }
    buf->RIDSubframePayloadBuffer[0] = 0x11; // 报头：类型0x1 | 版本0x1
    buf->RIDSubframePayloadBuffer[1] = packet->Flag; // 标志位
    buf->RIDSubframePayloadBuffer[2] = packet->TrackAngle; // 航迹角
    buf->RIDSubframePayloadBuffer[3] = packet->GroundSpeed; // 地速
    buf->RIDSubframePayloadBuffer[4] = packet->VerticalSpeed; // 垂直速度
    for (int i = 0; i < 4; i++) {
        buf->RIDSubframePayloadBuffer[5 + i] = (packet->Latitude >> (8 * i)) & 0xFF; // 纬度
        buf->RIDSubframePayloadBuffer[9 + i] = (packet->Longitude >> (8 * i)) & 0xFF; // 经度
    }
    for (int i = 0; i < 2; i++) {
        buf->RIDSubframePayloadBuffer[13 + i] = (packet->PressureAltitude >> (8 * i)) & 0xFF; // 气压高度
        buf->RIDSubframePayloadBuffer[15 + i] = (packet->GeometricAltitude >> (8 * i)) & 0xFF; // 几何高度
        buf->RIDSubframePayloadBuffer[17 + i] = (packet->AltitudeAGL >> (8 * i)) & 0xFF; // 距地高度
        buf->RIDSubframePayloadBuffer[21 + i] = (packet->Timestamp >> (8 * i)) & 0xFF; // 时间戳
    }
    buf->RIDSubframePayloadBuffer[19] = packet->Accuracy; // 精度
    buf->RIDSubframePayloadBuffer[20] = packet->SpeedAccuracy; // 速度精度
    buf->RIDSubframePayloadBuffer[23] = packet->TimestampAccuracy; // 时间戳精度
    buf->RIDSubframePayloadBuffer[24] = packet->reserved; // 预留
    return true;
}

// RID运行描述报文序列化
bool RIDRDSerialize(const RIDRDPacket *packet, RIDSubframePayloadBuffer *buf)
{
    if (!packet || !buf) {
        return false;
    }
    buf->RIDSubframePayloadBuffer[0] = 0x31; // 报头：类型0x3 | 版本0x1
    buf->RIDSubframePayloadBuffer[1] = packet->DescriptionType; // 描述类型
    for (int i = 0; i < 23; i++) {
        buf->RIDSubframePayloadBuffer[2 + i] = packet->Description[i]; // 描述内容
    }
    return true;
}

// RID系统报文序列化
bool RIDSYSSerialize(const RIDSYSPacket *packet, RIDSubframePayloadBuffer *buf)
{
    if (!packet || !buf) {
        return false;
    }
    buf->RIDSubframePayloadBuffer[0] = 0x41; // 报头：类型0x4 | 版本0x1
    buf->RIDSubframePayloadBuffer[1] = packet->Tag; // 标志字节
    for (int i = 0; i < 4; i++) {
        buf->RIDSubframePayloadBuffer[2 + i] = (packet->ControlStationLatitude >> (8 * i)) & 0xFF; // 控制站纬度
        buf->RIDSubframePayloadBuffer[6 + i] = (packet->ControlStationLongitude >> (8 * i)) & 0xFF; // 控制站经度
    }
    buf->RIDSubframePayloadBuffer[12] = packet->AreaRadius; // 运行区域半径
    for (int i = 0; i < 2; i++) {
        buf->RIDSubframePayloadBuffer[10 + i] = (packet->AreaCount >> (8 * i)) & 0xFF; // 运行区域计数
        buf->RIDSubframePayloadBuffer[13 + i] = (packet->AreaAltitudeUpper >> (8 * i)) & 0xFF; // 运行区域高度上限
        buf->RIDSubframePayloadBuffer[15 + i] = (packet->AreaAltitudeLower >> (8 * i)) & 0xFF; // 运行区域高度下限
        buf->RIDSubframePayloadBuffer[18 + i] = (packet->ControlStationAltitude >> (8 * i)) & 0xFF; // 控制站高度
    }
    for (int i = 0; i < 4; i++) {
        buf->RIDSubframePayloadBuffer[20 + i] = (packet->Timestamp >> (8 * i)) & 0xFF; // 时间戳
    }
    buf->RIDSubframePayloadBuffer[17] = packet->UARunCategory_Level; // UA运行类别/等级
    buf->RIDSubframePayloadBuffer[24] = packet->reserved; // 预留
    return true;
}

// RID Payload 序列化
bool RIDPayloadSerialize(const RIDPayload *payload, RIDPayloadBuffer *buf)
{
    if (!payload || !buf) {
        return false;
    }
    buf->RIDPayloadBuffer[0] = payload->Header; // 打包报文报头
    buf->RIDPayloadBuffer[1] = payload->SubPacketLength; // 子报文固定长度
    buf->RIDPayloadBuffer[2] = payload->SubPacketCount; // 子报文数量
    for (int i = 0; i < payload->SubPacketCount; i++) {
        for (int j = 0; j < 25; j++) {
            buf->RIDPayloadBuffer[3 + i * 25 + j] = payload->SubPackets[i][j]; // 子报文内容
        }
    }
    buf->length = 3 + payload->SubPacketCount * 25; // 实际有效长度
    return true;
}

/**
 * @brief 将报文打包为 RIDPayloadBuffer 数据。最小必送集，包含基本ID报文、位置向量报文和系统报文。
 *
 * @param basic    基本ID报文数据（输入，只读）
 * @param posVec   位置向量报文数据（输入，只读）
 * @param sys      系统报文数据（输入，只读）
 * @param payloadbuff  序列化输出
 * @return true    打包成功
 * @return false   参数无效
 */
bool RIDPacketNano(const RIDBasicPacket *basic, const RIDPosVecPacket *posVec,
               const RIDSYSPacket *sys, RIDPayloadBuffer *payloadbuff)
{
    if (!basic || !posVec || !sys || !payloadbuff) {
        return false;
    }
    RIDPayload payload;
    payload.Header = 0xF1; // 打包报文报头，类型0xF | 版本0x1
    payload.SubPacketLength = 0x19; // 子报文固定长度，25字节
    payload.SubPacketCount = 3; // 子报文数量
    // 子报文1：基本ID报文 (0x0)
    if (!RIDBasicSerialize(basic, (RIDSubframePayloadBuffer *)payload.SubPackets[0])) {
        return false;
    }
    // 子报文2：位置向量报文 (0x1)
    if (!RIDPosVecSerialize(posVec, (RIDSubframePayloadBuffer *)payload.SubPackets[1])) {
        return false;
    }
    // 子报文3：系统报文 (0x4)
    if (!RIDSYSSerialize(sys, (RIDSubframePayloadBuffer *)payload.SubPackets[2])) {
        return false;
    }
    if (!RIDPayloadSerialize(&payload, payloadbuff)) {
        return false;
    }
    return true;
}

/**
 * @name RIDPacket 完整打包函数（与Nano相比增加位置描述报文）
 * @brief 将报文打包为 RIDPayloadBuffer 数据。完整报文集，包含基本ID报文、位置向量报文、位置描述报文和系统报文。
 *
 * @param basic    基本ID报文数据（输入，只读）
 * @param posVec   位置向量报文数据（输入，只读）
 * @param rd       位置描述报文数据（输入，只读）
 * @param sys      系统报文数据（输入，只读）
 * @param payloadbuff  序列化输出
 * @return true    打包成功
 * @return false   参数无效
 */
bool RIDPacket(const RIDBasicPacket *basic, const RIDPosVecPacket *posVec, const RIDRDPacket *rd,
               const RIDSYSPacket *sys, RIDPayloadBuffer *payloadbuff)
{
    if (!basic || !posVec || !rd || !sys || !payloadbuff) {
        return false;
    }
    RIDPayload payload;
    payload.Header = 0xF1; // 打包报文报头，类型0xF | 版本0x1
    payload.SubPacketLength = 0x19; // 子报文固定长度，25字节
    payload.SubPacketCount = 4; // 子报文数量
    // 子报文1：基本ID报文 (0x0)
    if (!RIDBasicSerialize(basic, (RIDSubframePayloadBuffer *)payload.SubPackets[0])) {
        return false;
    }
    // 子报文2：位置向量报文 (0x1)
    if (!RIDPosVecSerialize(posVec, (RIDSubframePayloadBuffer *)payload.SubPackets[1])) {
        return false;
    }
    // 子报文3：位置描述报文 (0x3)
    if (!RIDRDSerialize(rd, (RIDSubframePayloadBuffer *)payload.SubPackets[2])) {
        return false;
    }
    // 子报文4：系统报文 (0x4)
    if (!RIDSYSSerialize(sys, (RIDSubframePayloadBuffer *)payload.SubPackets[3])) {
        return false;
    }
    if (!RIDPayloadSerialize(&payload, payloadbuff)) {
        return false;
    }
    return true;
}
