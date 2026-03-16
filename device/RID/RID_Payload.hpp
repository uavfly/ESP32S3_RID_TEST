#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#pragma pack(push, 1)

#define RID_VERSION 1

// RID协议子帧Payload承载数据类型（防止指针溢出）
typedef struct {
    uint8_t buffer[25]; // RID子帧固定长度25字节
} RIDSubframePayloadBuffer;

// RID基本ID报文数据类型
typedef struct {
    union {
        uint8_t header;
        struct {
            uint8_t VER : 4;    // RID协议版本，默认为1
            uint8_t ID : 4;     // 报文类型
        };
    };

    union {
        uint8_t IDType_UAType;      // 原始字节（用于直接赋值或序列化）
        struct {
            uint8_t UA_Type : 4;    // Bit[3:0]：UA类型
            uint8_t ID_Type : 4;    // Bit[7:4]：ID类型
        };
    };
    uint8_t UASID[20];          // 产品唯一识别码（ASCII/二进制，20字节）
    uint8_t reserved[3];        // 预留

    inline void fixheader() {
        ID = 0;
        VER = RID_VERSION;
    }
} RIDBasicPacket;

// RID位置向量报文数据类型
typedef struct {
    union {
        uint8_t header;
        struct {
            uint8_t VER : 4;    // RID协议版本，默认为1
            uint8_t ID : 4;     // 报文类型
        };
    };
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
    inline void fixheader() {
        ID = 1;
        VER = RID_VERSION;
    }
} RIDPosVecPacket;

// RID认证报文数据类型
typedef struct {
    union {
        uint8_t header;
        struct {
            uint8_t VER : 4;    // RID协议版本，默认为1
            uint8_t ID : 4;     // 报文类型
        };
    };
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
    inline void fixheader() {
        ID = 2;
        VER = RID_VERSION;
    }
} RIDAuthFirstPacket;

typedef struct {
    union {
        uint8_t header;
        struct {
            uint8_t VER : 4;    // RID协议版本，默认为1
            uint8_t ID : 4;     // 报文类型
        };
    };
    union {
        uint8_t AuthType_DataPage;  // 原始字节（用于直接赋值或序列化）
        struct {
            uint8_t DataPage : 4;   // Bit[3:0]：数据页编号（0~15）
            uint8_t AuthType : 4;   // Bit[7:4]：认证类型（0=无，1~4=定义值，5~15=预留）
        };
    };
    uint8_t AuthData[23];           // 认证数据（Page 0为17字节；Page 1+需使用从LastPageIndex起的完整23字节区域）
    inline void fixheader() {
        ID = 2;
        VER = RID_VERSION;
    }
} RIDAuthSubframePacket;


typedef struct {
    RIDAuthFirstPacket first;
    RIDAuthSubframePacket subframes[15]; // 最多15条子帧，总认证数据容量 17 + 15×23 = 362字节
    inline void fixheader() {
        first.fixheader();
        for(int i = 0; i<15 ; i++){
            subframes[i].fixheader();
        }
    }
} RIDAuthPacket;


// RID运行描述报文数据类型
typedef struct {
    union {
        uint8_t header;
        struct {
            uint8_t VER : 4;    // RID协议版本，默认为1
            uint8_t ID : 4;     // 报文类型
        };
    };
    uint8_t DescriptionType;    // 描述类型，0=文字描述，1~200=预留，201~255=私人使用
    uint8_t Description[23];    // 描述内容，ASCII字符串
    inline void fixheader() {
        ID = 3;
        VER = RID_VERSION;
    }
} RIDRDPacket;

// RID系统报文数据类型
typedef struct {
    union {
        uint8_t header;
        struct {
            uint8_t VER : 4;    // RID协议版本，默认为1
            uint8_t ID : 4;     // 报文类型
        };
    };
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
    inline void fixheader() {
        ID = 4;
        VER = RID_VERSION;
    }
} RIDSYSPacket;

// RID操作者ID报文数据类型
typedef struct {
    union {
        uint8_t header;
        struct {
            uint8_t VER : 4;    // RID协议版本，默认为1
            uint8_t ID : 4;     // 报文类型
        };
    };
    uint8_t OperatorIDType;         // 操作者ID类型（0=CAA注册ID，1~200=预留，201~255=私有使用）
    uint8_t OperatorID[20];         // 操作者ID（ASCII字符串，20字节）
    uint8_t reserved[3];            // 预留
    inline void fixheader() {
        ID = 5;
        VER = RID_VERSION;
    }
} RIDOperatorIDPacket;

// RID Payload 数据类型
typedef struct {
    union{
        struct {
            union {
                uint8_t header;
                struct {
                    uint8_t VER : 4;    // RID协议版本，默认为1
                    uint8_t ID : 4;     // 报文类型
                };
            };          // 打包报文报头，固定值0xF1（类型0xF | 版本0x1）
            uint8_t SubPacketLength; // 子报文固定长度，固定值0x19（= 25）
            uint8_t SubPacketCount;  // 子报文数量，最大10条
            RIDSubframePayloadBuffer Buffer[10];
        };
        uint8_t data[253];
    };
    
    inline uint16_t length() const {
        return 3 + SubPacketCount * 25;
    }
    inline void fixheader() {
        ID = 0xF;
        VER = RID_VERSION;
        SubPacketLength = 25;
    }
} RIDPayload;

// RID Payload 序列化数据类型
typedef struct {
    uint8_t RIDPayloadBuffer[256]; // RID帧长度最大为3 + 10×25 = 253字节，预留256字节对齐
    uint8_t length; // 实际有效长度
} RIDPayloadBuffer;

// RID完整打包数据类型
typedef struct {

    union
    {
        RIDBasicPacket basic;
        RIDSubframePayloadBuffer basic_serialize;
    };
    
    union
    {
        RIDPosVecPacket pos_vec;
        RIDSubframePayloadBuffer pos_vec_serialize;
    };

    union
    {
        RIDRDPacket rd;
        RIDSubframePayloadBuffer rd_serialize;
    };

    union
    {
        RIDSYSPacket sys;
        RIDSubframePayloadBuffer sys_serialize;
    };
    union
    {
        RIDOperatorIDPacket operator_id;
        RIDSubframePayloadBuffer operator_id_serialize;
    };
    union
    {
        RIDAuthPacket auth;
        RIDSubframePayloadBuffer auth_serialize[16];
    };
    bool basic_en;
    bool pos_vec_en;
    bool rd_en;
    bool sys_en;
    bool op_id_en;
    bool auth_en;

    inline void fixheader() {
        basic.fixheader();
        pos_vec.fixheader();
        rd.fixheader();
        sys.fixheader();
        operator_id.fixheader();
        auth.fixheader();
    }
} RIDpacket;

#pragma pack(pop) // 恢复默认对齐行为

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

    // 禁止拷贝（引用成员绑定到 this->data，拷贝会导致引用悬空）
    RID_Data(const RID_Data&) = delete;
    RID_Data& operator=(const RID_Data&) = delete;

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
        this->data.fixheader();
    }
    
    // 初始化数据填充(填充静态数据)
    void setup(const RIDpacket &packet){
        if(packet.basic_en){
            this->data.basic = packet.basic;
            this->data.basic_en = true;
        }
        if(packet.pos_vec_en){
            this->data.pos_vec = packet.pos_vec;
            this->data.pos_vec_en = true;
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

    RIDPayload get_payload(){
        RIDPayload payload;
        uint8_t count = 0;
        this->data.fixheader();
        if(this->data.basic_en){
            payload.Buffer[count] = this->data.basic_serialize;
            count++;
        }
        if(this->data.pos_vec_en){
            payload.Buffer[count] = this->data.pos_vec_serialize;
            count++;
        }
        if(this->data.rd_en){
            payload.Buffer[count] = this->data.rd_serialize;
            count++;
        }
        if(this->data.sys_en){
            payload.Buffer[count] = this->data.sys_serialize;
            count++;
        }
        if(this->data.op_id_en){
            payload.Buffer[count] = this->data.operator_id_serialize;
            count++;
        }
        if(this->data.auth_en){
            for(int i = 0; i <= this->data.auth.first.LastPageIndex;i++){
                if(count >= 10) break;
                payload.Buffer[count] = this->data.auth_serialize[i];
                count++;
            }
        }
        payload.SubPacketCount = count;
        payload.fixheader();
        return payload;
    }
    
private:
    
    // 负载数据
    RIDpacket data;
};

// UASID 字符串序列化（写入 packet->UASID，不足补零，超出截断）
void RIDUasIDSerialize(const char *str, RIDBasicPacket *packet);

// 运行描述字符串序列化（写入 packet->Description，不足补零，超出截断）
void RIDDescription(const char *str, RIDRDPacket *packet);