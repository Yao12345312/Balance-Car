#pragma once

#include <stdint.h>

#define PI  3.1415926f
#define D2R 0.017453f

// =============================================================================
// LQR 线性二次型调节器
//
// 移植自 balance-car-7_14 参考工程, 重构为 C++ 类。
// 增益 K / 前馈 ff / 限幅 max_out 实时绑定 g_params, MAVLink 在线修改后
// 下一次 compute() 立即生效。
//
// 控制律:
//   cmd = K[θ, θ̇, ω_rad] + ff*ff_angle,  再限幅到 [-max_out, +max_out]
// 其中 wheel_rpm 先换算为 rad/s (×0.1047)。
// =============================================================================

class LQR
{
  public:
    // esc_index: 0=ESC1 1=ESC2, 对应 g_params.lqr_k1 / lqr_k2
    float compute(float theta, float ff_angle, float theta_dot,
                  float wheel_rpm, uint8_t esc_index);
};
