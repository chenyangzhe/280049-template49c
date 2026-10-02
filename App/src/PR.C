/**
 * @file    PR.C
 * @brief   基于 TI DCL DF22 的单路准 PR 控制器实现
 * @note    使用带预扭曲的双线性变换计算系数；运行时通过
 *          DCL_runDF22_C2/C3 实现限幅后的条件状态更新。
 */

#include "PR.h"

#include <math.h>
#include <stddef.h>

#define PR_PI (3.14159265358979f)

/**
 * @brief 将控制器置为确定的零输出安全状态。
 */
static void PR_SetSafeState(PR_Handle *pr)
{
    DCL_DF22 *h = &pr->df22;

    h->b0  = 0.0f;
    h->b1  = 0.0f;
    h->b2  = 0.0f;
    h->a1  = 0.0f;
    h->a2  = 0.0f;
    h->x1  = 0.0f;
    h->x2  = 0.0f;
    h->sps = NULL_ADDR;
    h->css = NULL_ADDR;

    pr->Kp     = 0.0f;
    pr->outMax = 1.0f;
    pr->outMin = -1.0f;
}

void PR_Init(PR_Handle *pr, float Kp, float Ki,
             float freq, float wrc, float Ts)
{
    DCL_DF22 *h;
    float fs;
    float w0;
    float w0p;
    float den;
    float resonantB0;
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;

    if (pr == NULL) {
        return;
    }

    PR_SetSafeState(pr);

    if ((!isfinite(Kp)) || (!isfinite(Ki)) || (!isfinite(freq)) ||
        (!isfinite(wrc)) || (!isfinite(Ts)) || (Ts <= 0.0f) ||
        (freq <= 0.0f) || (wrc <= 0.0f)) {
        return;
    }

    fs = 1.0f / Ts;
    if ((!isfinite(fs)) || (freq >= (0.5f * fs))) {
        return;
    }

    w0 = 2.0f * PR_PI * freq;
    if (!isfinite(w0)) {
        return;
    }

    /* 预扭曲使离散谐振峰对准目标频率。 */
    w0p = 2.0f * fs * tanf(w0 / (2.0f * fs));
    if (!isfinite(w0p)) {
        return;
    }

    den = (4.0f * fs * fs) + (w0p * w0p) + (4.0f * fs * wrc);
    if ((!isfinite(den)) || (den <= 0.0f)) {
        return;
    }

    resonantB0 = (4.0f * Ki * wrc * fs) / den;
    a1 = ((-8.0f * fs * fs) + (2.0f * w0p * w0p)) / den;
    a2 = ((4.0f * fs * fs) + (w0p * w0p) -
          (4.0f * fs * wrc)) / den;

    /* Kp 与谐振支路通分合并，不额外引入近似。 */
    b0 = resonantB0 + Kp;
    b1 = Kp * a1;
    b2 = -resonantB0 + (Kp * a2);

    if ((!isfinite(b0)) || (!isfinite(b1)) || (!isfinite(b2)) ||
        (!isfinite(a1)) || (!isfinite(a2))) {
        return;
    }

    h = &pr->df22;
    h->b0 = b0;
    h->b1 = b1;
    h->b2 = b2;
    h->a1 = a1;
    h->a2 = a2;

    pr->Kp = Kp;
}

void PR_SetClamp(PR_Handle *pr, float max, float min)
{
    if ((pr == NULL) || (!isfinite(max)) || (!isfinite(min)) ||
        (max < min)) {
        return;
    }

    pr->outMax = max;
    pr->outMin = min;
}

void PR_Reset(PR_Handle *pr)
{
    if (pr == NULL) {
        return;
    }

    pr->df22.x1 = 0.0f;
    pr->df22.x2 = 0.0f;
}

float PR_Calc(PR_Handle *pr, float err)
{
    DCL_DF22 *h;
    float out;
    int16_t saturated;

    if ((pr == NULL) || (!isfinite(err))) {
        return 0.0f;
    }

    h = &pr->df22;
    out = DCL_runDF22_C2(h, err);

    if (!isfinite(out)) {
        PR_Reset(pr);
        return 0.0f;
    }

    saturated = DCL_runClamp_C2(&out, pr->outMax, pr->outMin);
    if (saturated == 0) {
        DCL_runDF22_C3(h, err, out);

        if ((!isfinite(h->x1)) || (!isfinite(h->x2))) {
            PR_Reset(pr);
            return 0.0f;
        }
    }

    return out;
}
