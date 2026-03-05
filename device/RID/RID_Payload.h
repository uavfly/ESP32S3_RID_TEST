#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// RID协议子帧Payload承载数据类型（防止指针溢出）
typedef struct {
    uint8_t RIDSubframePayloadBuffer[25]; // RID子帧固定长度25字节
} RIDSubframePayloadBuffer;

// RID基本ID报文数据类型
typedef struct {
    union {
        uint8_t IDType_UAType;      // 原始字节（用于直接赋值或序列化）
        struct {
            uint8_t UA_Type : 4;    // Bit[3:0]：UA类型
            uint8_t ID_Type : 4;    // Bit[7:4]：ID类型
        };
    };
    uint8_t UASID[20];          // 产品唯一识别码（ASCII/二进制，20字节）
    uint8_t reserved[3];        // 预留
} RIDBasicPacket;

// RID位置向量报文数据类型
typedef struct {
    union
    {
        uint8_t Flag;               // 原始字节（用于直接赋值或序列化）
        struct
        {
            // 位域从 LSB 开始分配，声明顺序对应 Bit[0]→Bit[7]
            uint8_t SpeedMultiplier   : 1;  // Bit[0]：速度乘数
            uint8_t TrackAngleEW      : 1;  // Bit[1]：航迹角 E/W 方向标志
            uint8_t AltitudeType      : 1;  // Bit[2]：高度类型位
            uint8_t FlagReserved      : 1;  // Bit[3]：预留
            uint8_t OperationalStatus : 4;  // Bit[7:4]：运行状态
        };
    };
    uint8_t TrackAngle;   // 航迹角，0~179，配合EW标志表示0~359°
    uint8_t GroundSpeed;  // 地速，单位参考ASTM F3411-22a
    int8_t VerticalSpeed;  // 垂直速度，爬升为正，下降为负（可选，无效填0）
    int32_t Latitude;       // 纬度，int32，小端序，单位1e-7度
    int32_t Longitude;      // 经度，int32，小端序，单位1e-7度
    int16_t PressureAltitude; // 气压高度，基准1013.25 mbar，int16，小端序（可选）
    int16_t GeometricAltitude; // 几何高度，基于当前坐标系，int16，小端序（可选）
    int16_t AltitudeAGL;   // 距地高度，基于起飞地或地表，int16，小端序
    union {
        uint8_t Accuracy;           // 原始字节
        struct {
            uint8_t HorizAccuracy : 4;  // Bit[3:0]：水平精度（可选）
            uint8_t VertAccuracy  : 4;  // Bit[7:4]：垂直精度（可选）
        };
    };
    union {
        uint8_t SpeedAccuracy;      // 原始字节
        struct {
            uint8_t SpeedAcc      : 4;  // Bit[3:0]：速度精度（可选）
            uint8_t SpeedAccRsvd  : 4;  // Bit[7:4]：预留
        };
    };
    uint16_t Timestamp;     // 时间戳，当前小时内已过去的 1/10 秒数，uint16，小端序
    union {
        uint8_t TimestampAccuracy;  // 原始字节
        struct {
            uint8_t TimestampAcc  : 4;  // Bit[3:0]：精度，0.1s 倍数（0=未知，max=1.5s）（可选）
            uint8_t TsAccRsvd     : 4;  // Bit[7:4]：预留
        };
    };
    uint8_t reserved;      // 预留
} RIDPosVecPacket;

// RID运行描述报文数据类型
typedef struct {
    uint8_t DescriptionType;    // 描述类型，0=文字描述，1~200=预留，201~255=私人使用
    uint8_t Description[23];    // 描述内容，ASCII字符串
} RIDRDPacket;

// RID系统报文数据类型
typedef struct {
    union {
        uint8_t Tag;                // 原始字节（用于直接赋值或序列化）
        struct {
            // 位域从 LSB 开始分配，声明顺序对应 Bit[0]→Bit[7]
            uint8_t ControlStationPosType : 2;  // Bit[1:0]：控制站位置类型
            uint8_t RegionCode            : 3;  // Bit[4:2]：等级归属区域（0=未定义，2=中国）
            uint8_t CoordType             : 2;  // Bit[6:5]：坐标系类型
            uint8_t TagReserved           : 1;  // Bit[7]：预留
        };
    };
    int32_t ControlStationLatitude;   // 控制站纬度，int32，小端序，单位1e-7度
    int32_t ControlStationLongitude;  // 控制站经度，int32，小端序，单位1e-7度
    uint16_t AreaCount;              // 运行区域计数，区域内航空器数量，uint16，小端序（可选）
    uint8_t AreaRadius;              // 运行区域半径，半径值 × 10（可选）
    int16_t AreaAltitudeUpper;       // 运行区域高度上限，几何高度，int16，小端序（可选）
    int16_t AreaAltitudeLower;       // 运行区域高度下限，几何高度，int16，小端序（可选）
    union {
        uint8_t UARunCategory_Level;     // UA运行类别/等级，Bit[7:4]：运行类别（0=未定义,1=开放,2=特许,3=审定,4~15=预留）
                                        // UA等级（0=微型,1=轻型,2=小型,3=其他具备RID功能,4~15=预留）
        struct {
            uint8_t UARunLevel : 4;       // Bit[3:0]：UA等级（0=微型,1=轻型,2=小型,3=其他具备RID功能,4~15=预留）
            uint8_t UARunCategory : 4;    // Bit[7:4]：运行类别（0=未定义,1=开放,2=特许,3=审定,4~15=预留）
        };
    };
    int16_t ControlStationAltitude;   // 控制站高度，几何高度，int16（可选）
    uint32_t Timestamp;              // 时间戳，位置相关信息的Unix时间戳，基准2019-01-01 00:00:00，uint32，秒（可选）
    uint8_t reserved;                // 预留
} RIDSYSPacket;

// RID Payload 数据类型
typedef struct {
    uint8_t Header;          // 打包报文报头，固定值0xF1（类型0xF | 版本0x1）
    uint8_t SubPacketLength; // 子报文固定长度，固定值0x19（= 25）
    uint8_t SubPacketCount;  // 子报文数量，最大10条
    uint8_t SubPackets[10][25]; // 子报文内容，最多10条，每条25字节
} RIDPayload;

// RID Payload 序列化数据类型
typedef struct {
    uint8_t RIDPayloadBuffer[256]; // RID帧长度最大为3 + 10×25 = 253字节，预留256字节对齐
    uint8_t length; // 实际有效长度
} RIDPayloadBuffer;

// RID完整打包数据类型
typedef struct {
    RIDBasicPacket basic;
    RIDPosVecPacket pos_vec;
    RIDRDPacket rd;
    RIDSYSPacket sys;
} RIDContext;

// UASID 字符串序列化（写入 packet->UASID，不足补零，超出截断）
void RIDUasIDSerialize(const char *str, RIDBasicPacket *packet);

// 运行描述字符串序列化（写入 packet->Description，不足补零，超出截断）
void RIDDescription(const char *str, RIDRDPacket *packet);

// 基本ID报文序列化
bool RIDBasicSerialize(const RIDBasicPacket *packet, RIDSubframePayloadBuffer *buf);

// RID位置向量报文序列化
bool RIDPosVecSerialize(const RIDPosVecPacket *packet, RIDSubframePayloadBuffer *buf);

// RID运行描述报文序列化
bool RIDRDSerialize(const RIDRDPacket *packet, RIDSubframePayloadBuffer *buf);

// RID系统报文序列化
bool RIDSYSSerialize(const RIDSYSPacket *packet, RIDSubframePayloadBuffer *buf);

// RID Payload 序列化
bool RIDPayloadSerialize(const RIDPayload *payload, RIDPayloadBuffer *buf);


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
               const RIDSYSPacket *sys, RIDPayloadBuffer *payloadbuff);

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
               const RIDSYSPacket *sys, RIDPayloadBuffer *payloadbuff);
// RID数据初始化
void RID_DATA_INIT(RIDBasicPacket *basic, RIDPosVecPacket *posVec, RIDRDPacket *rd, RIDSYSPacket *sys, const char *uasid, const char *description);
// RID数据更新
void RID_DATA_UPDATE(RIDPosVecPacket *posVec, RIDSYSPacket *sys, int32_t lat_1e7, int32_t lon_1e7, int16_t alt_agl, uint16_t ts_tenths);