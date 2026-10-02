/**
 * @file    Notch_Filter.c
 * @brief   二阶陷波器 (Notch Filter / Band-Stop)
 *          H(s) = (s^2 + w0^2) / (s^2 + 2*wrc*s + w0^2)
 *          双线性变换 + 预扭曲离散化, 使用 TI DCL_runDF22_C1 汇编优化
 */
#include "Notch_Filter.h"
#include <math.h>

#define NOTCH_PI (3.14159265358979f)

/*========== 初始化(双线性变换+预扭曲) ==========*/
void Notch_Init(Notch_Handle *nf, float freq, float wrc, float Ts)
{
    DCL_DF22 *h = &nf->df22;
    float fs = 1.0f / Ts;
    float w0 = 2.0f * NOTCH_PI * freq;

    /* 预扭曲: 保证离散后在 w0 处频率响应与连续域一致 */
    float w0p = 2.0f * fs * tanf(w0 / (2.0f * fs));
    float den;

    /*
     * H(s) = (s^2 + w0p^2) / (s^2 + 2*wrc*s + w0p^2)
     *
     * 双线性变换 s = (2*fs)*(z-1)/(z+1):
     *   Num = (4*fs^2 + w0p^2)*z^2 + (-8*fs^2 + 2*w0p^2)*z + (4*fs^2 + w0p^2)
     *   Den = (4*fs^2 + w0p^2 + 4*fs*wrc)*z^2 + (-8*fs^2 + 2*w0p^2)*z + (4*fs^2 + w0p^2 - 4*fs*wrc)
     */
    den = 4.0f*fs*fs + w0p*w0p + 4.0f*fs*wrc;

    h->b0 =  (4.0f*fs*fs + w0p*w0p) / den;
    h->b1 = (-8.0f*fs*fs + 2.0f*w0p*w0p) / den;
    h->b2 =  h->b0;                     /* b0 == b2, 对称零点 */
    h->a1 =  h->b1;                     /* a1 == b1 */
    h->a2 = (4.0f*fs*fs + w0p*w0p - 4.0f*fs*wrc) / den;

    /* 清除状态变量 */
    h->x1 = 0.0f;
    h->x2 = 0.0f;
}

/*========== 运行陷波器 ==========*/
float Notch_Run(Notch_Handle *nf, float input)
{
    return DCL_runDF22_C1(&nf->df22, input);
}

/*========== 状态复位 ==========*/
void Notch_Reset(Notch_Handle *nf)
{
    nf->df22.x1 = 0.0f;
    nf->df22.x2 = 0.0f;
}
