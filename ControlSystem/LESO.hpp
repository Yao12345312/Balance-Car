#pragma once

#include <stdint.h>

// =============================================================================
// 线性扩张状态观测器 (LESO)
//
// 移植自 balance-car-7_14 参考工程, 重构为 C++ 类。
// 参数 (b0, wo) 实时绑定 Basic/param.hpp 的 g_params, MAVLink 在线修改后
// 下一个 update() 周期立即生效 (beta 由 wo 派生, wo 变更时自动重算)。
//
// 坐标系/单位与参考工程一致:
//   theta : 角度测量值 (rad)
//   rate  : 角速度测量值 (rad/s)
//   u     : 控制量 (电流 A)
//   b0    : 控制有效性 (rad/s^2 per A)
// =============================================================================

// 三阶角度 LESO: 状态 z1=角度 z2=角速度 z3=扰动
class LESO
{
  public:
    LESO();

    void update(float theta, float u, float dt);

    float z1() const { return z1_; }   // 角度估计 (rad)
    float z2() const { return z2_; }   // 角速度估计 (rad/s)
    float z3() const { return z3_; }   // 扰动估计 (rad/s^2)

  private:
    float z1_, z2_, z3_;
    float b1_, b2_, b3_;    // beta = 3*wo, 3*wo^2, wo^3
    float last_wo_;         // 上次重算时用的 wo, 用于检测变更

    void recomputeBeta();
};

// 二阶角速度 LESO: 状态 z1=角速度 z2=扰动
class LESO_Rate
{
  public:
    LESO_Rate();

    void update(float rate, float u, float dt);

    float z1() const { return z1_; }   // 角速度估计 (rad/s)
    float z2() const { return z2_; }   // 扰动估计 (rad/s^2)

  private:
    float z1_, z2_;
    float b1_, b2_;         // beta = 2*wo, wo^2
    float last_wo_;

    void recomputeBeta();
};
