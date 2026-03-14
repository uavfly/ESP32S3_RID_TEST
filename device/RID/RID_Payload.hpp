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

// RID认证报文数据类型
typedef struct {
    union {
        uint8_t AuthType_DataPage;  // 原始字节（用于直接赋值或序列化）
        struct {
            uint8_t DataPage : 4;   // Bit[3:0]：数据页编号（0~15）
            uint8_t AuthType : 4;   // Bit[7:4]：认证类型（0=无，1~4=定义值，5~15=预留）
        };
    };
    uint8_t LastPageIndex;          // 最后一页索引（仅Page 0有意义）
    uint8_t Length;                  // 认证数据总长度（仅Page 0有意义）
    uint32_t Timestamp;             // 认证时间戳，基准2019-01-01 00:00:00，uint32，秒（仅Page 0有意义）
    uint8_t AuthData[17];           // 认证数据（Page 0为17字节；Page 1+需使用从LastPageIndex起的完整23字节区域）
} RIDAuthFirstPacket;

typedef struct {
    union {
        uint8_t AuthType_DataPage;  // 原始字节（用于直接赋值或序列化）
        struct {
            uint8_t DataPage : 4;   // Bit[3:0]：数据页编号（0~15）
            uint8_t AuthType : 4;   // Bit[7:4]：认证类型（0=无，1~4=定义值，5~15=预留）
        };
    };
    uint8_t AuthData[23];           // 认证数据（Page 0为17字节；Page 1+需使用从LastPageIndex起的完整23字节区域）
} RIDAuthSubframePacket;


typedef struct {
    RIDAuthFirstPacket first;
    RIDAuthSubframePacket subframes[15]; // 最多15条子帧，总认证数据容量 17 + 15×23 = 362字节
} RIDAuthPacket;


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

// RID操作者ID报文数据类型
typedef struct {
    uint8_t OperatorIDType;         // 操作者ID类型（0=CAA注册ID，1~200=预留，201~255=私有使用）
    uint8_t OperatorID[20];         // 操作者ID（ASCII字符串，20字节）
    uint8_t reserved[3];            // 预留
} RIDOperatorIDPacket;

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
    RIDOperatorIDPacket operator_id;
    RIDAuthPacket auth;
    bool basic_en;
    bool pos_vec_en;
    bool rd_en;
    bool sys_en;
    bool op_id_en;
    bool auth_en;
} RIDpacket;

// 总消息负载序列化数据类型
typedef struct {
    uint8_t basic[25];
    uint8_t pos_vec[25];
    uint8_t rd[25];
    uint8_t sys[25];
    uint8_t operator_id[25];
    uint8_t auth[16][25];    // 最多16页认证子帧
    uint8_t auth_page_count; // 认证页数
    uint8_t data[528];       // 3 + 21×25 = 528（5个非认证子帧 + 16个认证子帧）
    uint16_t size;
    bool basic_en;
    bool pos_vec_en;
    bool rd_en;
    bool sys_en;
    bool op_id_en;
    bool auth_en;
} RIDpacketSerialized;

class RID_Data{
public:
    
    // 子帧数据引用（简化访问，如 RID_Data.basic 等价于 RID_Data.data.basic）
    RIDBasicPacket       &basic;
    RIDPosVecPacket      &pos_vec;
    RIDRDPacket          &rd;
    RIDSYSPacket         &sys;
    RIDOperatorIDPacket  &operator_id;
    RIDAuthPacket        &auth;

    // 使能标志引用
    bool &basic_en;
    bool &pos_vec_en;
    bool &rd_en;
    bool &sys_en;
    bool &op_id_en;
    bool &auth_en;

    // 构造函数：绑定引用到 data 内部成员
    RID_Data()
        : basic(data.basic)
        , pos_vec(data.pos_vec)
        , rd(data.rd)
        , sys(data.sys)
        , operator_id(data.operator_id)
        , auth(data.auth)
        , basic_en(data.basic_en)
        , pos_vec_en(data.pos_vec_en)
        , rd_en(data.rd_en)
        , sys_en(data.sys_en)
        , op_id_en(data.op_id_en)
        , auth_en(data.auth_en)
    {
        // 初始化数据
        this->init();
    }

    // 初始化数据
    void init(){
        memset(&data, 0, sizeof(RIDpacket));
    }
    // 初始化数据填充(填充静态数据)
    void setup(const RIDpacket &packet){
        if(packet.basic_en){
            this->data.basic = packet.basic;
            this->data.basic_en = true;
        }
        if(packet.rd_en){
            this->data.rd = packet.rd;
            this->data.rd_en = true;
        }
        if(packet.op_id_en){
            this->data.operator_id = packet.operator_id;
            this->data.op_id_en = true;
        }
        if(packet.auth_en){
            this->data.auth = packet.auth;
            this->data.auth_en = true;
        }
        if(packet.sys_en){
            this->data.sys.Tag = packet.sys.Tag;
            this->data.sys.UARunCategory_Level = packet.sys.UARunCategory_Level;
            this->data.sys.AreaRadius = packet.sys.AreaRadius;
            this->data.sys.AreaAltitudeUpper = packet.sys.AreaAltitudeUpper;
            this->data.sys.AreaAltitudeLower = packet.sys.AreaAltitudeLower;
            this->data.sys.AreaCount = packet.sys.AreaCount;
            this->data.sys_en = true;
        }
    }
    // 清空数据
    void clean(){
        this->init();
    }
    // 更新数据
    void update_basic(const RIDBasicPacket &basic){
        this->data.basic = basic;
        this->basic_en = true;
    }
    void update_pos_vec(const RIDPosVecPacket &pos_vec){
        this->data.pos_vec = pos_vec;
        this->pos_vec_en = true;
    }
    void update_rd(const RIDRDPacket &rd){
        this->data.rd = rd;
        this->rd_en = true;
    }
    void update_sys(const RIDSYSPacket &sys){
        this->data.sys = sys;
        this->sys_en = true;
    }
    void update_operator_id(const RIDOperatorIDPacket &operator_id){
        this->data.operator_id = operator_id;
        this->op_id_en = true;
    }
    void update_auth(const RIDAuthPacket &auth){
        this->data.auth = auth;
        this->auth_en = true;
    }
    void update(const RIDpacket &packet){
        if(packet.basic_en){
            this->update_basic(packet.basic);
        }
        if(packet.pos_vec_en){
            this->update_pos_vec(packet.pos_vec);
        }
        if(packet.rd_en){
            this->update_rd(packet.rd);
        }
        if(packet.sys_en){
            this->update_sys(packet.sys);
        }
        if(packet.op_id_en){
            this->update_operator_id(packet.operator_id);
        }
        if(packet.auth_en){
            this->update_auth(packet.auth);
        }
    }
    
private:
    // 序列化
    RIDSubframePayloadBuffer basic_serialize(const RIDBasicPacket &packet){

        RIDSubframePayloadBuffer buf;

        buf.RIDSubframePayloadBuffer[0] = 0x01; // 报头：类型0x0 | 版本0x1
        buf.RIDSubframePayloadBuffer[1] = packet.IDType_UAType; // ID类型和UA类型
        for (int i = 0; i < 20; i++) {
            buf.RIDSubframePayloadBuffer[2 + i] = packet.UASID[i]; // UASID
        }
        for (int i = 0; i < 3; i++) {
            buf.RIDSubframePayloadBuffer[22 + i] = packet.reserved[i]; // 预留
        }
        return buf;
    }

    RIDSubframePayloadBuffer pos_vec_serialize(const RIDPosVecPacket &packet){
        RIDSubframePayloadBuffer buf;
        buf.RIDSubframePayloadBuffer[0] = 0x11; // 报头：类型0x1 | 版本0x1
        buf.RIDSubframePayloadBuffer[1] = packet.Flag; // 标志位
        buf.RIDSubframePayloadBuffer[2] = packet.TrackAngle; // 航迹角
        buf.RIDSubframePayloadBuffer[3] = packet.GroundSpeed; // 地速
        buf.RIDSubframePayloadBuffer[4] = packet.VerticalSpeed; // 垂直速度
        for (int i = 0; i < 4; i++) {
            buf.RIDSubframePayloadBuffer[5 + i] = (packet.Latitude >> (8 * i)) & 0xFF; // 纬度
            buf.RIDSubframePayloadBuffer[9 + i] = (packet.Longitude >> (8 * i)) & 0xFF; // 经度
        }
        for (int i = 0; i < 2; i++) {
            buf.RIDSubframePayloadBuffer[13 + i] = (packet.PressureAltitude >> (8 * i)) & 0xFF; // 气压高度
            buf.RIDSubframePayloadBuffer[15 + i] = (packet.GeometricAltitude >> (8 * i)) & 0xFF; // 几何高度
            buf.RIDSubframePayloadBuffer[17 + i] = (packet.AltitudeAGL >> (8 * i)) & 0xFF; // 距地高度
            buf.RIDSubframePayloadBuffer[21 + i] = (packet.Timestamp >> (8 * i)) & 0xFF; // 时间戳
        }
        buf.RIDSubframePayloadBuffer[19] = packet.Accuracy; // 精度
        buf.RIDSubframePayloadBuffer[20] = packet.SpeedAccuracy; // 速度精度
        buf.RIDSubframePayloadBuffer[23] = packet.TimestampAccuracy; // 时间戳精度
        buf.RIDSubframePayloadBuffer[24] = packet.reserved; // 预留
        return buf;
    }

    RIDSubframePayloadBuffer rd_serialize(const RIDRDPacket &packet){
        RIDSubframePayloadBuffer buf;
        buf.RIDSubframePayloadBuffer[0] = 0x31; // 报头：类型0x3 | 版本0x1
        buf.RIDSubframePayloadBuffer[1] = packet.DescriptionType; // 描述类型
        for (int i = 0; i < 23; i++) {
            buf.RIDSubframePayloadBuffer[2 + i] = packet.Description[i]; // 描述内容
        }
        return buf;
    }

    RIDSubframePayloadBuffer sys_serialize(const RIDSYSPacket &packet){
        RIDSubframePayloadBuffer buf;
        buf.RIDSubframePayloadBuffer[0] = 0x41; // 报头：类型0x4 | 版本0x1
        buf.RIDSubframePayloadBuffer[1] = packet.Tag; // 标志位
        for (int i = 0; i < 4; i++) {
            buf.RIDSubframePayloadBuffer[2 + i] = (packet.ControlStationLatitude >> (8 * i)) & 0xFF; // 控制站纬度
            buf.RIDSubframePayloadBuffer[6 + i] = (packet.ControlStationLongitude >> (8 * i)) & 0xFF; // 控制站经度
        }
        for (int i = 0; i < 2; i++) {
            buf.RIDSubframePayloadBuffer[10 + i] = (packet.AreaCount >> (8 * i)) & 0xFF; // 运行区域计数
        }
        buf.RIDSubframePayloadBuffer[12] = packet.AreaRadius; // 运行区域半径
        for (int i = 0; i < 2; i++) {
            buf.RIDSubframePayloadBuffer[13 + i] = (packet.AreaAltitudeUpper >> (8 * i)) & 0xFF; // 运行区域高度上限
            buf.RIDSubframePayloadBuffer[15 + i] = (packet.AreaAltitudeLower >> (8 * i)) & 0xFF; // 运行区域高度下限
        }
        buf.RIDSubframePayloadBuffer[17] = packet.UARunCategory_Level; // UA运行类别/等级
        for (int i = 0; i < 2; i++) {
            buf.RIDSubframePayloadBuffer[18 + i] = (packet.ControlStationAltitude >> (8 * i)) & 0xFF; // 控制站高度
        }
        for (int i = 0; i < 4; i++) {
            buf.RIDSubframePayloadBuffer[20 + i] = (packet.Timestamp >> (8 * i)) & 0xFF; // 时间戳
        }
        buf.RIDSubframePayloadBuffer[24] = packet.reserved; // 预留
        return buf;
    }

    RIDSubframePayloadBuffer operator_id_serialize(const RIDOperatorIDPacket &packet){
        RIDSubframePayloadBuffer buf;
        buf.RIDSubframePayloadBuffer[0] = 0x21; // 报头：类型0x2 | 版本0x1
        buf.RIDSubframePayloadBuffer[1] = packet.OperatorIDType; // 操作者ID类型
        for (int i = 0; i < 20; i++) {
            buf.RIDSubframePayloadBuffer[2 + i] = packet.OperatorID[i]; // 操作者ID
        }
        for (int i = 0; i < 3; i++) {
            buf.RIDSubframePayloadBuffer[22 + i] = packet.reserved[i]; // 预留
        }
        return buf;
    }

    // 认证Page 0序列化
    RIDSubframePayloadBuffer auth_first_serialize(const RIDAuthFirstPacket &packet){
        RIDSubframePayloadBuffer buf;
        buf.RIDSubframePayloadBuffer[0] = 0x51; // 报头：类型0x5 | 版本0x1
        buf.RIDSubframePayloadBuffer[1] = packet.AuthType_DataPage; // 认证类型和数据页编号
        buf.RIDSubframePayloadBuffer[2] = packet.LastPageIndex; // 最后一页索引
        buf.RIDSubframePayloadBuffer[3] = packet.Length; // 认证数据总长度
        for (int i = 0; i < 4; i++) {
            buf.RIDSubframePayloadBuffer[4 + i] = (packet.Timestamp >> (8 * i)) & 0xFF; // 认证时间戳
        }
        for (int i = 0; i < 17; i++) {
            buf.RIDSubframePayloadBuffer[8 + i] = packet.AuthData[i]; // 认证数据
        }
        return buf;
    }

    // 认证Page 1+序列化
    RIDSubframePayloadBuffer auth_sub_serialize(const RIDAuthSubframePacket &packet){
        RIDSubframePayloadBuffer buf;
        buf.RIDSubframePayloadBuffer[0] = 0x51; // 报头：类型0x5 | 版本0x1
        buf.RIDSubframePayloadBuffer[1] = packet.AuthType_DataPage; // 认证类型和数据页编号
        for (int i = 0; i < 23; i++) {
            buf.RIDSubframePayloadBuffer[2 + i] = packet.AuthData[i]; // 认证数据
        }
        return buf;
    }

    RIDSubframePayloadBuffer[16] auth_serialize(const RIDAuthPacket &packet){
        
    }

    RIDpacketSerialized serialize(const RIDpacket &packet){
        RIDpacketSerialized serialized;
        serialized.basic_en = packet.basic_en;
        serialized.pos_vec_en = packet.pos_vec_en;
        serialized.rd_en = packet.rd_en;
        serialized.sys_en = packet.sys_en;
        serialized.op_id_en = packet.op_id_en;
        serialized.auth_en = packet.auth_en;

        serialized.data[0] = 0xF1; // 打包报文报头，固定值0xF1（类型0xF | 版本0x1）
        serialized.data[1] = 0x19; // 子报文固定长度，固定值0x19（= 25）
        serialized.data[2] = 0; // 子报文数量

        if (packet.basic_en) {
            RIDSubframePayloadBuffer buf = this->basic_serialize(packet.basic);
            memcpy(serialized.basic, buf.RIDSubframePayloadBuffer, 25);
            memcpy(serialized.data + 3 + (serialized.data[2] * 25), buf.RIDSubframePayloadBuffer, 25);
            serialized.data[2]++; // 子报文数量加1
        }
        if (packet.pos_vec_en) {
            RIDSubframePayloadBuffer buf = this->pos_vec_serialize(packet.pos_vec);
            memcpy(serialized.pos_vec, buf.RIDSubframePayloadBuffer, 25);
            memcpy(serialized.data + 3 + (serialized.data[2] * 25), buf.RIDSubframePayloadBuffer, 25);
            serialized.data[2]++; // 子报文数量加1
        }
        if (packet.rd_en) {
            RIDSubframePayloadBuffer buf = this->rd_serialize(packet.rd);
            memcpy(serialized.rd, buf.RIDSubframePayloadBuffer, 25);
            memcpy(serialized.data + 3 + (serialized.data[2] * 25), buf.RIDSubframePayloadBuffer, 25);
            serialized.data[2]++; // 子报文数量加1
        }
        if (packet.sys_en) {
            RIDSubframePayloadBuffer buf = this->sys_serialize(packet.sys);
            memcpy(serialized.sys, buf.RIDSubframePayloadBuffer, 25);
            memcpy(serialized.data + 3 + (serialized.data[2] * 25), buf.RIDSubframePayloadBuffer, 25);
            serialized.data[2]++; // 子报文数量加1
        }
        if (packet.op_id_en) {
            RIDSubframePayloadBuffer buf = this->operator_id_serialize(packet.operator_id);
            memcpy(serialized.operator_id, buf.RIDSubframePayloadBuffer, 25);
            memcpy(serialized.data + 3 + (serialized.data[2] * 25), buf.RIDSubframePayloadBuffer, 25);
            serialized.data[2]++; // 子报文数量加1
        }
        serialized.auth_page_count = 0;
        if (packet.auth_en) {
            // Page 0
            RIDSubframePayloadBuffer buf = this->auth_first_serialize(packet.auth.first);
            memcpy(serialized.auth[0], buf.RIDSubframePayloadBuffer, 25);
            memcpy(serialized.data + 3 + (serialized.data[2] * 25), buf.RIDSubframePayloadBuffer, 25);
            serialized.data[2]++;
            serialized.auth_page_count = 1;
            // Page 1+ （根据 LastPageIndex 确定页数）
            uint8_t lastPage = packet.auth.first.LastPageIndex;
            for (int p = 1; p <= lastPage && p <= 15; p++) {
                buf = this->auth_sub_serialize(packet.auth.subframes[p - 1]);
                memcpy(serialized.auth[p], buf.RIDSubframePayloadBuffer, 25);
                memcpy(serialized.data + 3 + (serialized.data[2] * 25), buf.RIDSubframePayloadBuffer, 25);
                serialized.data[2]++;
                serialized.auth_page_count++;
            }
        }
        serialized.size = 3 + serialized.data[2] * 25; // 计算有效长度
        return serialized;
    }

    // 反序列化
    RIDBasicPacket basic_deserialize(const RIDSubframePayloadBuffer &buf){
        RIDBasicPacket packet;
        packet.IDType_UAType = buf.RIDSubframePayloadBuffer[1];
        for (int i = 0; i < 20; i++) {
            packet.UASID[i] = buf.RIDSubframePayloadBuffer[2 + i];
        }
        for (int i = 0; i < 3; i++) {
            packet.reserved[i] = buf.RIDSubframePayloadBuffer[22 + i];
        }
        return packet;
    }

    RIDPosVecPacket pos_vec_deserialize(const RIDSubframePayloadBuffer &buf){
        RIDPosVecPacket packet;
        packet.Flag = buf.RIDSubframePayloadBuffer[1];
        packet.TrackAngle = buf.RIDSubframePayloadBuffer[2];
        packet.GroundSpeed = buf.RIDSubframePayloadBuffer[3];
        packet.VerticalSpeed = buf.RIDSubframePayloadBuffer[4];
        packet.Latitude = 0;
        packet.Longitude = 0;
        for (int i = 0; i < 4; i++) {
            packet.Latitude |= ((int32_t)buf.RIDSubframePayloadBuffer[5 + i]) << (8 * i);
            packet.Longitude |= ((int32_t)buf.RIDSubframePayloadBuffer[9 + i]) << (8 * i);
        }
        packet.PressureAltitude = 0;
        packet.GeometricAltitude = 0;
        packet.AltitudeAGL = 0;
        packet.Timestamp = 0;
        for (int i = 0; i < 2; i++) {
            packet.PressureAltitude |= ((int16_t)buf.RIDSubframePayloadBuffer[13 + i]) << (8 * i);
            packet.GeometricAltitude |= ((int16_t)buf.RIDSubframePayloadBuffer[15 + i]) << (8 * i);
            packet.AltitudeAGL |= ((int16_t)buf.RIDSubframePayloadBuffer[17 + i]) << (8 * i);
            packet.Timestamp |= ((uint16_t)buf.RIDSubframePayloadBuffer[21 + i]) << (8 * i);
        }
        packet.Accuracy = buf.RIDSubframePayloadBuffer[19];
        packet.SpeedAccuracy = buf.RIDSubframePayloadBuffer[20];
        packet.TimestampAccuracy = buf.RIDSubframePayloadBuffer[23];
        packet.reserved = buf.RIDSubframePayloadBuffer[24];
        return packet;
    }

    RIDRDPacket rd_deserialize(const RIDSubframePayloadBuffer &buf){
        RIDRDPacket packet;
        packet.DescriptionType = buf.RIDSubframePayloadBuffer[1];
        for (int i = 0; i < 23; i++) {
            packet.Description[i] = buf.RIDSubframePayloadBuffer[2 + i];
        }
        return packet;
    }

    RIDSYSPacket sys_deserialize(const RIDSubframePayloadBuffer &buf){
        RIDSYSPacket packet;
        packet.Tag = buf.RIDSubframePayloadBuffer[1];
        packet.ControlStationLatitude = 0;
        packet.ControlStationLongitude = 0;
        for (int i = 0; i < 4; i++) {
            packet.ControlStationLatitude |= ((int32_t)buf.RIDSubframePayloadBuffer[2 + i]) << (8 * i);
            packet.ControlStationLongitude |= ((int32_t)buf.RIDSubframePayloadBuffer[6 + i]) << (8 * i);
        }
        packet.AreaCount = 0;
        for (int i = 0; i < 2; i++) {
            packet.AreaCount |= ((uint16_t)buf.RIDSubframePayloadBuffer[10 + i]) << (8 * i);
        }
        packet.AreaRadius = buf.RIDSubframePayloadBuffer[12];
        packet.AreaAltitudeUpper = 0;
        packet.AreaAltitudeLower = 0;
        for (int i = 0; i < 2; i++) {
            packet.AreaAltitudeUpper |= ((int16_t)buf.RIDSubframePayloadBuffer[13 + i]) << (8 * i);
            packet.AreaAltitudeLower |= ((int16_t)buf.RIDSubframePayloadBuffer[15 + i]) << (8 * i);
        }
        packet.UARunCategory_Level = buf.RIDSubframePayloadBuffer[17];
        packet.ControlStationAltitude = 0;
        for (int i = 0; i < 2; i++) {
            packet.ControlStationAltitude |= ((int16_t)buf.RIDSubframePayloadBuffer[18 + i]) << (8 * i);
        }
        packet.Timestamp = 0;
        for (int i = 0; i < 4; i++) {
            packet.Timestamp |= ((uint32_t)buf.RIDSubframePayloadBuffer[20 + i]) << (8 * i);
        }
        packet.reserved = buf.RIDSubframePayloadBuffer[24];
        return packet;
    }

    RIDOperatorIDPacket operator_id_deserialize(const RIDSubframePayloadBuffer &buf){
        RIDOperatorIDPacket packet;
        packet.OperatorIDType = buf.RIDSubframePayloadBuffer[1];
        for (int i = 0; i < 20; i++) {
            packet.OperatorID[i] = buf.RIDSubframePayloadBuffer[2 + i];
        }
        for (int i = 0; i < 3; i++) {
            packet.reserved[i] = buf.RIDSubframePayloadBuffer[22 + i];
        }
        return packet;
    }

    // 认证Page 0反序列化
    RIDAuthFirstPacket auth_first_deserialize(const RIDSubframePayloadBuffer &buf){
        RIDAuthFirstPacket packet;
        packet.AuthType_DataPage = buf.RIDSubframePayloadBuffer[1];
        packet.LastPageIndex = buf.RIDSubframePayloadBuffer[2];
        packet.Length = buf.RIDSubframePayloadBuffer[3];
        packet.Timestamp = 0;
        for (int i = 0; i < 4; i++) {
            packet.Timestamp |= ((uint32_t)buf.RIDSubframePayloadBuffer[4 + i]) << (8 * i);
        }
        for (int i = 0; i < 17; i++) {
            packet.AuthData[i] = buf.RIDSubframePayloadBuffer[8 + i];
        }
        return packet;
    }

    // 认证Page 1+反序列化
    RIDAuthSubframePacket auth_sub_deserialize(const RIDSubframePayloadBuffer &buf){
        RIDAuthSubframePacket packet;
        packet.AuthType_DataPage = buf.RIDSubframePayloadBuffer[1];
        for (int i = 0; i < 23; i++) {
            packet.AuthData[i] = buf.RIDSubframePayloadBuffer[2 + i];
        }
        return packet;
    }

    RIDpacket deserialize(const RIDPayloadBuffer &buf){
        RIDpacket packet;
        if (buf.length < 3) {
            // 无效数据，长度不足
            return packet;
        }
        uint8_t header = buf.RIDPayloadBuffer[0];
        uint8_t subPacketLength = buf.RIDPayloadBuffer[1];
        uint8_t subPacketCount = buf.RIDPayloadBuffer[2];
        if (header != 0xF1 || subPacketLength != 25 || subPacketCount > 10 || buf.length < 3 + subPacketCount * subPacketLength) {
            // 无效数据，格式错误
            return packet;
        }
        for (int i = 0; i < subPacketCount; i++) {
            RIDSubframePayloadBuffer subPacketData;
            memcpy(subPacketData.RIDSubframePayloadBuffer, buf.RIDPayloadBuffer + 3 + i * subPacketLength, sizeof(subPacketData.RIDSubframePayloadBuffer));
            switch (subPacketData.RIDSubframePayloadBuffer[0]) { // 根据子报文类型判断
                case 0x01: // 基本ID报文
                    packet.basic = this->basic_deserialize(subPacketData);
                    packet.basic_en = true;
                    break;
                case 0x11: // 位置向量报文
                    packet.pos_vec = this->pos_vec_deserialize(subPacketData);
                    packet.pos_vec_en = true;
                    break;
                case 0x31: // 位置描述报文
                    packet.rd = this->rd_deserialize(subPacketData);
                    packet.rd_en = true;
                    break;
                case 0x41: // 系统报文
                    packet.sys = this->sys_deserialize(subPacketData);
                    packet.sys_en = true;
                    break;
                case 0x21: // 操作者ID报文
                    packet.operator_id = this->operator_id_deserialize(subPacketData);
                    packet.op_id_en = true;
                    break;
                case 0x51: { // 认证报文
                    uint8_t dataPage = subPacketData.RIDSubframePayloadBuffer[1] & 0x0F;
                    if (dataPage == 0) {
                        packet.auth.first = this->auth_first_deserialize(subPacketData);
                    } else if (dataPage <= 15) {
                        packet.auth.subframes[dataPage - 1] = this->auth_sub_deserialize(subPacketData);
                    }
                    packet.auth_en = true;
                    break;
                }
                default:
                    // 未知子报文类型，忽略
                    break;
            }
        }
        return packet;
    }

    // 负载数据
    RIDpacket data;
};

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
/* bool RIDPacket(const RIDBasicPacket *basic, const RIDPosVecPacket *posVec, const RIDRDPacket *rd,
               const RIDSYSPacket *sys, RIDPayloadBuffer *payloadbuff); */
// RID数据初始化
void RID_DATA_INIT(RIDBasicPacket *basic, RIDPosVecPacket *posVec, RIDRDPacket *rd, RIDSYSPacket *sys, const char *uasid, const char *description);
// RID数据更新
void RID_DATA_UPDATE(RIDPosVecPacket *posVec, RIDSYSPacket *sys, int32_t lat_1e7, int32_t lon_1e7, int16_t alt_agl, uint16_t ts_tenths);