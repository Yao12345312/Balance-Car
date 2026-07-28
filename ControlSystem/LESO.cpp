#include "LESO.hpp"
#include "param.hpp"

// =============================================================================
// 三阶角度 LESO
//
// 参考工程 beta 约定 (wo = 100 rad/s @ 500Hz):
//   beta1 = 3 * wo
//   beta2 = 3 * wo^2
//   beta3 = wo^3
// =============================================================================

LESO::LESO()
    : z1_(0.0f), z2_(0.0f), z3_(0.0f),
      b1_(0.0f), b2_(0.0f), b3_(0.0f),
      last_wo_(-1.0f)
{
}

void LESO::recomputeBeta(void)
{
    float wo = g_params.eso_wo;
    b1_ = 3.0f * wo;
    b2_ = 3.0f * wo * wo;
    b3_ = wo * wo * wo;
    last_wo_ = wo;
}

void LESO::update(float theta, float u, float dt)
{
    // wo 在线变更 (param_set 置位 g_eso_dirty) 或本实例首次运行 -> 重算 beta
    if (g_eso_dirty || g_params.eso_wo != last_wo_)
        recomputeBeta();

    float b0 = g_params.eso_b0;

    // 观测误差: 实际角度 - 估计角度
    float e = theta - z1_;

    // z1: 角度估计
    z1_ += dt * (z2_ + b1_ * e);

    // z2: 角速度估计 (含控制量 b0*u)
    z2_ += dt * (z3_ + b0 * u + b2_ * e);

    // z3: 扰动估计 (扩张状态)
    z3_ += dt * (b3_ * e);
}

// =============================================================================
// 二阶角速度 LESO
//
// 参考 beta 约定:
//   beta1 = 2 * wo
//   beta2 = wo^2
// =============================================================================

LESO_Rate::LESO_Rate()
    : z1_(0.0f), z2_(0.0f),
      b1_(0.0f), b2_(0.0f),
      last_wo_(-1.0f)
{
}

void LESO_Rate::recomputeBeta(void)
{
    float wo = g_params.eso_wo_rate;
    b1_ = 2.0f * wo;
    b2_ = wo * wo;
    last_wo_ = wo;
}

void LESO_Rate::update(float rate, float u, float dt)
{
    if (g_eso_dirty || g_params.eso_wo_rate != last_wo_)
        recomputeBeta();

    float b0 = g_params.eso_b0;

    float e = rate - z1_;

    // z1: 角速度估计 (含控制量 b0*u)
    z1_ += dt * (b0 * u + z2_ + b1_ * e);

    // z2: 扰动估计
    z2_ += dt * (b2_ * e);
}
