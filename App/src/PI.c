/**
 * @file    PI.c
 * @brief   PI 控制器 — DCL DF22 实现, 带 Anti-Windup
 * @note    PI(s) = Kp + Ki/s
 *          双线性变换离散化: b0=Kp+Ki*Ts/2, b1=-Kp+Ki*Ts/2, a1=-1
 *          DCL_runDF22_C1 汇编优化
 */

#include "PI.h"

#include <math.h>
#include <stddef.h>

static void PI_SetSafeState(PI_Handle *pi)
{
    DCL_DF22 *h = &pi->df22;

    h->b0  = 0.0f;
    h->b1  = 0.0f;
    h->b2  = 0.0f;
    h->a1  = -1.0f;
    h->a2  = 0.0f;
    h->x1  = 0.0f;
    h->x2  = 0.0f;

    pi->Kp     = 0.0f;
    pi->outMax = 1.0f;
    pi->outMin = -1.0f;
}

void PI_Init(PI_Handle *pi, float Kp, float Ki, float Ts)
{
    DCL_DF22 *h;
    float b0, b1;

    if (pi == NULL) {
        return;
    }

    PI_SetSafeState(pi);

    if ((!isfinite(Kp)) || (!isfinite(Ki)) || (!isfinite(Ts)) ||
        (Ts <= 0.0f)) {
        return;
    }

    b0 = Kp + Ki * Ts * 0.5f;
    b1 = -Kp + Ki * Ts * 0.5f;

    if ((!isfinite(b0)) || (!isfinite(b1))) {
        return;
    }

    h = &pi->df22;
    h->b0 = b0;
    h->b1 = b1;
    h->b2 = 0.0f;
    h->a1 = -1.0f;
    h->a2 = 0.0f;
    h->x1 = 0.0f;
    h->x2 = 0.0f;

    pi->Kp = Kp;
}

float PI_Calc(PI_Handle *pi, float err)
{
    DCL_DF22 *h;
    float x1_save;
    float out;

    if ((pi == NULL) || (!isfinite(err))) {
        return 0.0f;
    }

    h = &pi->df22;

    /*
     * Anti-Windup 策略: 运行前保存积分状态, 若输出饱和,
     * 按饱和比例压缩积分状态, 防止积分器持续累积。
     * PI 的 a2=0, 只有 x1 是积分状态。
     */
    x1_save = h->x1;
    out = DCL_runDF22_C1(h, err);

    if (!isfinite(out)) {
        PI_Reset(pi);
        return 0.0f;
    }

    if (out > pi->outMax) {
        float ratio = pi->outMax / out;
        if (isfinite(ratio)) {
            h->x1 = x1_save * ratio;
        }
        out = pi->outMax;
    } else if (out < pi->outMin) {
        float ratio = pi->outMin / out;
        if (isfinite(ratio)) {
            h->x1 = x1_save * ratio;
        }
        out = pi->outMin;
    }

    if ((!isfinite(h->x1)) || (!isfinite(h->x2))) {
        PI_Reset(pi);
        return 0.0f;
    }

    return out;
}

void PI_SetClamp(PI_Handle *pi, float max, float min)
{
    if ((pi == NULL) || (!isfinite(max)) || (!isfinite(min)) ||
        (max < min)) {
        return;
    }

    pi->outMax = max;
    pi->outMin = min;
}

void PI_Reset(PI_Handle *pi)
{
    if (pi == NULL) {
        return;
    }

    pi->df22.x1 = 0.0f;
    pi->df22.x2 = 0.0f;
}
