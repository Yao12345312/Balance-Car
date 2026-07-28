#include "param.hpp"
#include <string.h>

// =============================================================================
// 默认参数
// =============================================================================
Params g_params =
{
    /*eso_b0*/      600.0f,
    /*eso_wo*/      100.0f,
    /*eso_wo_rate*/ 100.0f,

    /*lqr_k1*/      { -20.0f, -3.5f, -0.0001f },   // ESC1: [角度, 角速度, 轮速]
    /*lqr_k2*/      { -20.0f, -3.5f,  0.0001f },   // ESC2
    /*lqr_ff1*/     0.0f,
    /*lqr_ff2*/     0.0f,
    /*lqr_max_out*/ 20.0f,

    /*mechanics_medium*/ 0.016f,   // 机械中值 (rad)
    /*kp_speed*/         0.02f,
    /*ki_speed*/         0.0005f,
    /*speed_int_limit*/  0.3f,
    /*max_tilt*/         0.15f,
    /*kp_yaw*/           0.4f,
    /*max_steer*/        2.5f,
    /*arm_angle*/        0.10f,
    /*max_balance_angle*/0.30f,
};

volatile bool g_eso_dirty = true;   // 启动时强制重算一次 beta

// =============================================================================
// 参数注册表
//
// 顺序即 MAVLink PARAM_VALUE.param_index (0 起)。新增参数追加到表尾即可,
// 地面站通过 PARAM_REQUEST_LIST 自动发现全表, 无需改 GCS 代码。
// =============================================================================
const ParamEntry g_param_table[] =
{
    {"eso_b0",      &g_params.eso_b0,        PARAM_TYPE_REAL32, false},
    {"eso_wo",      &g_params.eso_wo,        PARAM_TYPE_REAL32, false},
    {"eso_worate",  &g_params.eso_wo_rate,   PARAM_TYPE_REAL32, false},

    {"lqr_k1_0",    &g_params.lqr_k1[0],     PARAM_TYPE_REAL32, false},
    {"lqr_k1_1",    &g_params.lqr_k1[1],     PARAM_TYPE_REAL32, false},
    {"lqr_k1_2",    &g_params.lqr_k1[2],     PARAM_TYPE_REAL32, false},

    {"lqr_k2_0",    &g_params.lqr_k2[0],     PARAM_TYPE_REAL32, false},
    {"lqr_k2_1",    &g_params.lqr_k2[1],     PARAM_TYPE_REAL32, false},
    {"lqr_k2_2",    &g_params.lqr_k2[2],     PARAM_TYPE_REAL32, false},

    {"lqr_ff1",     &g_params.lqr_ff1,       PARAM_TYPE_REAL32, false},
    {"lqr_ff2",     &g_params.lqr_ff2,       PARAM_TYPE_REAL32, false},
    {"lqr_maxout",  &g_params.lqr_max_out,   PARAM_TYPE_REAL32, false},

    {"mech_mid",    &g_params.mechanics_medium,  PARAM_TYPE_REAL32, false},
    {"kp_speed",    &g_params.kp_speed,          PARAM_TYPE_REAL32, false},
    {"ki_speed",    &g_params.ki_speed,          PARAM_TYPE_REAL32, false},
    {"spd_int_lim", &g_params.speed_int_limit,   PARAM_TYPE_REAL32, false},
    {"max_tilt",    &g_params.max_tilt,          PARAM_TYPE_REAL32, false},
    {"kp_yaw",      &g_params.kp_yaw,            PARAM_TYPE_REAL32, false},
    {"max_steer",   &g_params.max_steer,         PARAM_TYPE_REAL32, false},
    {"arm_angle",   &g_params.arm_angle,         PARAM_TYPE_REAL32, false},
    {"max_bal_ang", &g_params.max_balance_angle, PARAM_TYPE_REAL32, false},
};

const uint16_t g_param_count = (uint16_t)(sizeof(g_param_table) / sizeof(g_param_table[0]));

uint16_t param_count(void)
{
    return g_param_count;
}

int16_t param_find(const char *name)
{
    if (!name)
        return -1;
    for (uint16_t i = 0; i < g_param_count; i++)
    {
        // name 最长 16 字符, strncmp 比较 16 字符即可 (不足部分表内为 '\0')
        if (strncmp(g_param_table[i].name, name, 16) == 0)
            return (int16_t)i;
    }
    return -1;
}

float param_get(uint16_t index)
{
    if (index >= g_param_count)
        return 0.0f;
    return *g_param_table[index].ptr;
}

bool param_set(uint16_t index, float value)
{
    if (index >= g_param_count)
        return false;
    if (g_param_table[index].readonly)
        return false;

    *g_param_table[index].ptr = value;

    // ESO 带宽变更 -> 置 dirty, LESO 下次 update() 重算 beta
    float *p = g_param_table[index].ptr;
    if (p == &g_params.eso_wo || p == &g_params.eso_wo_rate)
        g_eso_dirty = true;

    return true;
}
