#include "task_Control.hpp"
#include "sensors/drv_ICM20948.hpp"
#include "drv_TFMini.hpp"
#include "drv_US100.hpp"
#include "drv_CubeFOC.hpp"
#include "MahonyAHRS.hpp"
#include "LESO.hpp"
#include "LQR.hpp"
#include "param.hpp"

#include <cmath>

// =============================================================================
// 全局共享对象 (声明于 task_Control.hpp, 控制任务创建, 通信任务消费)
// =============================================================================
osMessageQueueId_t g_mavSensorQueue = NULL;
osSemaphoreId_t    g_controlModeSem = NULL;

volatile uint8_t g_rc_control_mode = 0xFF;   // 上电默认停机
volatile float   g_rc_speed_target = 0.0f;
volatile float   g_rc_manual_y     = 0.0f;

// 速度环积分 (运行时状态, 非可调参数)
static float speed_integral = 0.0f;
// 速度环目标倾角 (速度环输出 -> 平衡环前馈)
static float g_delta_theta_ref = 0.0f;

namespace {
// 最短角差 (处理 ±π 翻转)
static inline float AngleDiffRad(float a, float b)
{
    return atan2f(sinf(a - b), cosf(a - b));
}
} // namespace

// =============================================================================
// 速度环 (目标速度 -> 目标倾角)
// dt = 0.004s (控制环 2ms, 每 2 周期跑一次 = 4ms)
// =============================================================================
static float SpeedLoopControl(const int32_t *wheel_speed)
{
    float avg_speed = (wheel_speed[0] - wheel_speed[1]) * 0.5f * 0.1047f;
    float speed_error = g_rc_speed_target - avg_speed;
    speed_integral += speed_error * 0.004f;
    if (speed_integral > g_params.speed_int_limit) speed_integral = g_params.speed_int_limit;
    if (speed_integral < -g_params.speed_int_limit) speed_integral = -g_params.speed_int_limit;

    float speed_out = g_params.kp_speed * speed_error + g_params.ki_speed * speed_integral;
    if (speed_out >  g_params.max_tilt) speed_out =  g_params.max_tilt;
    if (speed_out < -g_params.max_tilt) speed_out = -g_params.max_tilt;
    return speed_out;
}

// =============================================================================
// 转向环 (目标偏航角速度 -> 转向量)
// dt = 0.010s (控制环 2ms, 每 5 周期跑一次 = 10ms)
// =============================================================================
static float YawRateControl(float yaw_target, float yaw_measured)
{
    float yaw_error = yaw_target - yaw_measured;
    float steer_out = g_params.kp_yaw * yaw_error;
    if (steer_out >  g_params.max_steer) steer_out =  g_params.max_steer;
    if (steer_out < -g_params.max_steer) steer_out = -g_params.max_steer;
    return steer_out;
}

// =============================================================================
// 平衡车控制 (LQR + 双 LESO + 安全保护 + 自动 arm/disarm)
// =============================================================================
static void BalanceCarControl(
    CubeFOC *esc,
    CubeFOC::ESCStatusCache esc_status[],
    const float *theta,
    const float *theta_dot,
    const int32_t *wheel_speed,
    int32_t *esc_current_cmd,
    float delta_theta_ref,
    uint32_t &calib_counter,
    uint32_t &arm_counter,
    bool &control_armed,
    LQR &lqr,
    LESO &eso_l,
    LESO &eso_r)
{
    const uint32_t arm_count_need = 20U;

    // 温度保护
    if (esc_status[ESC1_Index].temperature > ESC_MAX_Temperature ||
        esc_status[ESC2_Index].temperature > ESC_MAX_Temperature)
    {
        g_rc_control_mode = 0xFF;
        esc_current_cmd[0] = 0;
        esc_current_cmd[1] = 0;
        esc->send_esc_current_commands(esc_current_cmd, 2);
        return;
    }

    // 转速保护
    if (esc_status[ESC1_Index].rpm > ESC_MAX_SpeedRPM ||
        esc_status[ESC2_Index].rpm > ESC_MAX_SpeedRPM)
    {
        g_rc_control_mode = 0xFF;
        esc_current_cmd[0] = 0;
        esc_current_cmd[1] = 0;
        esc->send_esc_current_commands(esc_current_cmd, 2);
        return;
    }

    // 未校准则持续发校准指令
    if (!esc_status[ESC1_Index].calib_flag ||
        !esc_status[ESC2_Index].calib_flag)
    {
        esc_current_cmd[0] = 0;
        esc_current_cmd[1] = 0;
        control_armed = false;
        arm_counter = 0;

        if ((calib_counter++ % 20U) == 0U)
        {
            const uint8_t esc_id = (uint8_t)((calib_counter / 20U) % 2U);
            esc->calib_esc_command(esc_id + 1U);
        }
        esc->send_esc_current_commands(esc_current_cmd, 2);
        return;
    }
    calib_counter = 0;

    // 自动解锁: 角度小于阈值持续 N 周期
    if (!control_armed)
    {
        if (fabsf(theta[0]) < g_params.arm_angle)
            arm_counter++;
        else
            arm_counter = 0;

        if (arm_counter >= arm_count_need)
        {
            //TODO: 无蜂鸣器驱动, 原 buzzer.beep(500,200)
            control_armed = true;
        }

        esc_current_cmd[0] = 0;
        esc_current_cmd[1] = 0;
        esc->send_esc_current_commands(esc_current_cmd, 2);
        return;
    }

    // 失衡保护: 超出最大平衡角度强制 disarm
    if (fabsf(theta[0]) > g_params.max_balance_angle)
    {
        esc_current_cmd[0] = 0;
        esc_current_cmd[1] = 0;
        control_armed = false;
        arm_counter = 0;
        esc->send_esc_current_commands(esc_current_cmd, 2);
        return;
    }

    // LQR + LESO (角度误差 -> 电流)
    float theta_err = theta[0] - delta_theta_ref;

    float u0 = lqr.compute(theta_err, theta[0], theta_dot[0], (float)wheel_speed[0], ESC1_Index);
    float u1 = lqr.compute(theta_err, theta[0], theta_dot[0], (float)wheel_speed[1], ESC2_Index);

    // 双 LESO 估计扰动并前馈补偿 (dt=0.002s  500Hz)
    eso_l.update(theta_err, u0, 0.002f);
    eso_r.update(theta_err, u1, 0.002f);

    float balance_l = u0 - eso_l.z3() / g_params.eso_b0;
    float balance_r = u1 - eso_r.z3() / g_params.eso_b0;

    // 转向量 (每 5 周期更新一次, 由调用方控制频率)
    static float steer_out = 0.0f;
    static uint32_t steer_loop_counter = 0;
    if ((++steer_loop_counter % 5U) == 0U)
    {
        steer_out = YawRateControl((float)g_rc_manual_y, theta_dot[1]);
    }

    // 左右轮叠加转向量, A -> mA
    esc_current_cmd[0] = (int32_t)(-(balance_l + steer_out) * 1000.0f);
    esc_current_cmd[1] = (int32_t)((balance_r - steer_out) * 1000.0f);

    esc->send_esc_current_commands(esc_current_cmd, 2);
}

// =============================================================================
// 轮速测试模式
// =============================================================================
static void WheelSpeedTestControl(
    CubeFOC *esc,
    CubeFOC::ESCStatusCache esc_status[],
    int32_t *esc_current_cmd,
    uint32_t &calib_counter,
    uint32_t &arm_counter,
    bool &control_armed)
{
    // 温度保护
    if (esc_status[ESC1_Index].temperature > ESC_MAX_Temperature ||
        esc_status[ESC2_Index].temperature > ESC_MAX_Temperature)
    {
        g_rc_control_mode = 0xFF;
        esc_current_cmd[0] = 0;
        esc_current_cmd[1] = 0;
        esc->send_esc_current_commands(esc_current_cmd, 2);
        return;
    }

    // 未校准
    if (!esc_status[ESC1_Index].calib_flag ||
        !esc_status[ESC2_Index].calib_flag)
    {
        esc_current_cmd[0] = 0;
        esc_current_cmd[1] = 0;
        control_armed = false;
        arm_counter = 0;

        if ((calib_counter++ % 20U) == 0U)
        {
            const uint8_t esc_id = (uint8_t)((calib_counter / 20U) % 2U);
            esc->calib_esc_command(esc_id + 1U);
        }
        esc->send_esc_current_commands(esc_current_cmd, 2);
        return;
    }
    calib_counter = 0;

    static uint32_t test_timer = 0;
    static bool forward_phase = true;
    const uint32_t phase_duration = 1000U;
    const int32_t test_current = 500;

    if (forward_phase)
    {
        esc_current_cmd[0] =  test_current;
        esc_current_cmd[1] =  test_current;
    }
    else
    {
        esc_current_cmd[0] = -test_current;
        esc_current_cmd[1] = -test_current;
    }

    if (++test_timer >= phase_duration)
    {
        test_timer = 0;
        forward_phase = !forward_phase;
    }
    esc->send_esc_current_commands(esc_current_cmd, 2);
}

// =============================================================================
// 控制任务主循环 (500Hz / 2ms)
// =============================================================================
void StartControlTask(void *argument)
{

    auto *imu = drv_icm20948();
    auto *esc = drv_cubefoc();
	auto *tfmini = drv_tfmini();
	auto *us100 = drv_us100();

	
    // AHRS: 500Hz, Kp=2.0, Ki=0.002
    MahonyAHRS ahrs(500.0f, 2.0f, 0.002f);

    float ax = 0, ay = 0, az = 0;
    float gx = 0, gy = 0, gz = 0;
    float roll = 0, pitch = 0, yaw = 0;

    CubeFOC::ESCStatusCache esc_status[Max_ESC_Num] = {};
    int32_t esc_current_cmd[2] = {0};

    uint32_t calib_counter = 0;
    uint32_t arm_counter   = 0;
    bool     control_armed = false;

    LQR  lqr;
    LESO eso_l;
    LESO eso_r;
	
    // 创建传感数据队列 (控制任务 -> 通信任务)
    g_mavSensorQueue = osMessageQueueNew(8, sizeof(MavSensorData_t), NULL);
    // 创建模式切换信号量
    g_controlModeSem = osSemaphoreNew(1, 0, NULL);

    uint32_t next_wake = osKernelGetTickCount();

    while (1)
    {
        bool imu_ok = (imu->getAllMotionDataCalibrated(ax, ay, az, gx, gy, gz) == 0);
		
		if (us100->updateObstacleAvoidance() && g_rc_speed_target > 0.0f) {
			g_rc_speed_target = 0.0f;
		}
		
		
        if (imu_ok)
        {
            // 无磁力计, mag 传 0
            ahrs.update(gx, gy, gz, ax, ay, az, 0, 0, 0);
            ahrs.getEulerRad(roll, pitch, yaw);

            // 每 10 周期 (20ms) 打包一帧传感数据入队
            static uint8_t sensor_send_counter = 0;
            if ((++sensor_send_counter % 10U) == 0U)
            {
                CubeFOC::ESCStatusCache st;
                float voltage = 0.0f;
                if (esc->get_esc_status(ESC1_Index, st))
                    voltage = st.voltage;

                MavSensorData_t data;
                data.roll        = roll;
                data.pitch       = pitch;
                data.yaw         = yaw;
                data.rollspeed   = gx;
                data.pitchspeed  = gy;
                data.yawspeed    = gz;
                data.voltage     = voltage;   // ESC 总线电压 = 电池电压
                data.current     = -1.0f;     // 无电流计, 未知
                data.battery_remaining = 0;   // 未知
                osMessageQueuePut(g_mavSensorQueue, &data, 0, 0);
            }
        }

        // CAN 收发
        esc->spin_once();
        esc->get_esc_status(ESC1_Index, esc_status[ESC1_Index]);
        esc->get_esc_status(ESC2_Index, esc_status[ESC2_Index]);

        // 失衡时清速度积分与目标倾角
        if (fabsf(roll) > 1.0f)
        {
            speed_integral = 0.0f;
            g_delta_theta_ref = 0.0f;
        }

        // 角度 = 实际角度 - 机械中值
        const float theta[1] = { AngleDiffRad(roll, g_params.mechanics_medium) };
        // theta_dot: [平衡用 X 角速度, 转向用 Z 角速度]
        const float theta_dot[2] = { gx, gz };
        const int32_t wheel_speed[2] = { esc_status[ESC1_Index].rpm, esc_status[ESC2_Index].rpm };

        // 消费模式切换信号 (acquire 0 立即返回, 用于清标志)
        if (g_controlModeSem != NULL)
            osSemaphoreAcquire(g_controlModeSem, 0);

        // 模式分发
        if (g_rc_control_mode == 0 || g_rc_control_mode == 1)
        {
            // 平衡模式 / 单点保持: 速度环每 2 周期跑一次
            static uint32_t speed_loop_counter = 0;
            if ((++speed_loop_counter % 2U) == 0U)
                g_delta_theta_ref = SpeedLoopControl(wheel_speed);

            BalanceCarControl(esc, esc_status, theta, theta_dot, wheel_speed,
                              esc_current_cmd, g_delta_theta_ref,
                              calib_counter, arm_counter, control_armed,
                              lqr, eso_l, eso_r);
        }
        else if (g_rc_control_mode == 4)
        {
            WheelSpeedTestControl(esc, esc_status, esc_current_cmd,
                                  calib_counter, arm_counter, control_armed);
        }
        else
        {
            // 停机
            esc_current_cmd[0] = 0;
            esc_current_cmd[1] = 0;
            esc->send_esc_current_commands(esc_current_cmd, 2);
            control_armed = false;
            arm_counter = 0;
        }

        next_wake += 2U;   // 500Hz
        osDelayUntil(next_wake);
    }
}
