#include "LQR.hpp"
#include "param.hpp"

static inline float clampf(float v, float lo, float hi)
{
    if (v > hi) return hi;
    if (v < lo) return lo;
    return v;
}

float LQR::compute(float theta, float ff_angle, float theta_dot,
                   float wheel_rpm, uint8_t esc_index)
{
    const float *k  = (esc_index == 0) ? g_params.lqr_k1 : g_params.lqr_k2;
    float        ff = (esc_index == 0) ? g_params.lqr_ff1 : g_params.lqr_ff2;

    float current_cmd_A = k[0] * theta
                        + k[1] * theta_dot
                        + k[2] * wheel_rpm * 0.1047f;

    current_cmd_A += ff * ff_angle;

    current_cmd_A = clampf(current_cmd_A, -g_params.lqr_max_out, g_params.lqr_max_out);

    return current_cmd_A;
}
