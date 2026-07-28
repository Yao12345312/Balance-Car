#include "MahonyAHRS.hpp"

MahonyAHRS::MahonyAHRS(float freq, float kp, float ki)
{
    sampleFreq = freq;
    twoKp = 2.0f * kp;
    twoKi = 2.0f * ki;

    q0 = 1.0f;
    q1 = q2 = q3 = 0.0f;

    integralFBx = integralFBy = integralFBz = 0.0f;
}

inline float MahonyAHRS::invSqrt(float x)
{
    float inv = 1.0f / sqrtf(x);

    return inv;
}

void MahonyAHRS::update(float gx, float gy, float gz,
                        float ax, float ay, float az,
                        float mx, float my, float mz)
{
    float recipNorm;
    float hx, hy, bx, bz;
    float vx, vy, vz, wx, wy, wz;
    float ex, ey, ez;

    float gyro_norm = sqrtf(gx * gx + gy * gy + gz * gz);
    float magWeight_dynamic;

    if (gyro_norm > 0.7f)
        magWeight_dynamic = 0.0f;
    else if (gyro_norm > 0.3f)
        magWeight_dynamic = 0.001f;
    else
        magWeight_dynamic = 0.003f;

    recipNorm = invSqrt(ax * ax + ay * ay + az * az);
    ax *= recipNorm;
    ay *= recipNorm;
    az *= recipNorm;

    recipNorm = invSqrt(mx * mx + my * my + mz * mz);
    mx *= recipNorm;
    my *= recipNorm;
    mz *= recipNorm;

    hx = 2.0f * (mx * (0.5f - q2 * q2 - q3 * q3) + my * (q1 * q2 - q0 * q3) + mz * (q1 * q3 + q0 * q2));
    hy = 2.0f * (mx * (q1 * q2 + q0 * q3) + my * (0.5f - q1 * q1 - q3 * q3) + mz * (q2 * q3 - q0 * q1));
    bx = sqrtf(hx * hx + hy * hy);
    bz = 2.0f * (mx * (q1 * q3 - q0 * q2) + my * (q2 * q3 + q0 * q1) + mz * (0.5f - q1 * q1 - q2 * q2));

    vx = 2.0f * (q1 * q3 - q0 * q2);
    vy = 2.0f * (q0 * q1 + q2 * q3);
    vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

    wx = 2.0f * bx * (0.5f - q2 * q2 - q3 * q3) + 2.0f * bz * (q1 * q3 - q0 * q2);
    wy = 2.0f * bx * (q1 * q2 - q0 * q3) + 2.0f * bz * (q0 * q1 + q2 * q3);
    wz = 2.0f * bx * (q0 * q2 + q1 * q3) + 2.0f * bz * (0.5f - q1 * q1 - q2 * q2);

    ex = (ay * vz - az * vy);
    ey = (az * vx - ax * vz);
    ez = (ax * vy - ay * vx);

    if (fabsf(ez) > 0.2f)
        ez = 0;

    integralFBx += twoKi * ex * (1.0f / sampleFreq);
    integralFBy += twoKi * ey * (1.0f / sampleFreq);
    integralFBz += twoKi * ez * (1.0f / sampleFreq);

    gx += twoKp * ex + integralFBx;
    gy += twoKp * ey + integralFBy;
    gz += twoKp * ez + integralFBz;

    gx *= (0.5f / sampleFreq);
    gy *= (0.5f / sampleFreq);
    gz *= (0.5f / sampleFreq);

    float qa = q0;
    float qb = q1;
    float qc = q2;

    q0 += (-qb * gx - qc * gy - q3 * gz);
    q1 += (qa * gx + qc * gz - q3 * gy);
    q2 += (qa * gy - qb * gz + q3 * gx);
    q3 += (qa * gz + qb * gy - qc * gx);

    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
}

void MahonyAHRS::getQuaternion(float &w, float &x, float &y, float &z)
{
    w = q0;
    x = q1;
    y = q2;
    z = q3;
}

void MahonyAHRS::getEuler(float &roll, float &pitch, float &yaw)
{
    roll  = atan2f(2 * (q0 * q1 + q2 * q3), 1 - 2 * (q1 * q1 + q2 * q2));
    pitch = asinf(2 * (q0 * q2 - q3 * q1));
    yaw   = atan2f(2 * (q0 * q3 + q1 * q2), 1 - 2 * (q2 * q2 + q3 * q3));

    roll  *= 57.29578f;
    pitch *= 57.29578f;
    yaw   *= 57.29578f;
}

void MahonyAHRS::getEulerRad(float &roll, float &pitch, float &yaw)
{
    roll  = atan2f(2 * (q0 * q1 + q2 * q3), 1 - 2 * (q1 * q1 + q2 * q2));
    pitch = asinf(2 * (q0 * q2 - q3 * q1));
    yaw   = atan2f(2 * (q0 * q3 + q1 * q2), 1 - 2 * (q2 * q2 + q3 * q3));
}
