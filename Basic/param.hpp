#pragma once

#include <stdint.h>

// =============================================================================
// 参数类型 (与 MAVLink MAV_PARAM_TYPE 对齐, 这里本地定义避免 Basic 反向依赖
// Drivers/middleWare/MAVlink)
// =============================================================================
#define PARAM_TYPE_UINT8   1
#define PARAM_TYPE_INT8    2
#define PARAM_TYPE_UINT16  3
#define PARAM_TYPE_INT16   4
#define PARAM_TYPE_UINT32  5
#define PARAM_TYPE_INT32   6
#define PARAM_TYPE_REAL32  9   // MAV_PARAM_TYPE_REAL32, 本表全部使用 float

// =============================================================================
// 全局参数存储
//
// 所有控制器/观测器参数集中在此结构, 默认值在 param.cpp。
// 控制环热路径直接读字段 (单 float 读在 Cortex-M7 上原子), MAVLink 在线修改后
// 下一个控制周期立即生效。
// =============================================================================
struct Params
{
    // ---- ESO (线性扩张状态观测器) ----
    float eso_b0;          // 控制有效性 (rad/s^2 per A), 越大扰动估计越强
    float eso_wo;          // 角度ESO带宽 wo (rad/s), beta1/2/3 由它派生 (3wo/3wo^2/wo^3)
    float eso_wo_rate;     // 角速度ESO带宽 wo (rad/s), beta1/2 由它派生 (2wo/wo^2)

    // ---- LQR (线性二次型调节器) ----
    float lqr_k1[3];       // ESC1 增益: [角度, 角速度, 轮速(rad/s)]
    float lqr_k2[3];       // ESC2 增益
    float lqr_ff1;         // ESC1 前馈角度增益
    float lqr_ff2;         // ESC2 前馈角度增益
    float lqr_max_out;     // 电流指令限幅 (A)

    // ---- 速度环 / 转向环 / 机械中值 / 保护 (移植自 balance-car-7_14) ----
    float mechanics_medium; // 机械中值 (rad), 角度零点偏置
    float kp_speed;         // 速度环 P
    float ki_speed;         // 速度环 I
    float speed_int_limit;  // 速度积分限幅
    float max_tilt;         // 速度环输出限幅 (倾角 rad)
    float kp_yaw;           // 转向环 (偏航角速度) P
    float max_steer;        // 转向输出限幅
    float arm_angle;        // 自动解锁判定角度阈值 (rad)
    float max_balance_angle;// 失衡保护角度 (rad), 超出强制 disarm
};

extern Params g_params;

// 编译期出厂默认值 (与 g_params 初始值同源, 恢复出厂参数用)
extern const Params k_param_defaults;

// 参数脏标志: param_set() 置位, 通信任务固化到 Flash 成功后清除
extern volatile bool g_params_dirty;

// =============================================================================
// 参数注册表
//
// 每个字段对应一条 entry, name 即 MAVLink param_id (<=16 字符, 不含 '\0')。
// 新增参数只需在 g_param_table 追加条目 -> 地面站自动发现, 无需改 GCS 代码。
// =============================================================================
struct ParamEntry
{
    const char name[16];   // 参数名 (不足16字符以 '\0' 填充)
    float     *ptr;        // 指向 g_params 内字段
    uint8_t    type;       // PARAM_TYPE_*
    bool       readonly;   // true = 仅广播不可在线写
};

extern const ParamEntry g_param_table[];
extern const uint16_t   g_param_count;

// =============================================================================
// 查询/修改接口 (线程安全: 控制环只读 g_params, 通信环通过本接口写)
// =============================================================================
uint16_t param_count(void);
int16_t  param_find(const char *name);         // 返回索引, -1 = 未找到
float    param_get(uint16_t index);            // 越界返回 0
bool     param_set(uint16_t index, float value);   // 写入并触发派生标志, 成功 true

// ESO 带宽变更标志: param_set 改 eso_wo / eso_wo_rate 后置位,
// LESO 在下次 update() 重算 beta (避免控制热路径做乘方运算)
extern volatile bool g_eso_dirty;
