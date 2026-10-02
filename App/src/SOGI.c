/**
 * @file    SOGI.c
 * @brief   二阶广义积分器 (SOGI) — DCL DF22 实现
 * @note    参考 TI PMP23069 (TTPLPFC) 实现:
 *          - 双线性变换离散化 (Tustin)
 *          - k = √2, 基波 f0 = 50/60 Hz
 *          - 控制 ISR 频率通常 10kHz~100kHz
 *
 *          两个 DF22 共享分母, 仅分子不同:
 *          H_d(s):  分子 = k·ωₙ·s          → b0 =  2kωₙTs / den
 *                                             b1 =  0
 *                                             b2 = -2kωₙTs / den
 *          H_q(s):  分子 = k·ωₙ²           → b0 =  kωₙ²Ts² / den
 *                                             b1 =  2kωₙ²Ts² / den
 *                                             b2 =  kωₙ²Ts² / den
 *          分母:    den = 4 + 2kωₙTs + ωₙ²Ts²
 *          a1 = (2ωₙ²Ts² - 8) / den
 *          a2 = (4 - 2kωₙTs + ωₙ²Ts²) / den
 *
 *          PMP23069 参数 (100kHz, 60Hz):
 *            b0_d = 222.2862, b1_d = -222.034  (H_d 近似形式)
 */

#include "SOGI.h"
#include <math.h>

void SOGI_Init(SOGI_Handle *s, float f0, float Ts, float k)
{
    float wn  = 2.0f * 3.14159265358979f * f0;
    float Ts2 = Ts * Ts;
    float wn2 = wn * wn;

    /* 双线性变换公共分母 */
    float den = 4.0f + 2.0f * k * wn * Ts + wn2 * Ts2;

    /* ── H_d(z): 同相 DF22 ── */
    DCL_DF22 *hd = &s->df22_d;
    hd->b0 =  (2.0f * k * wn * Ts) / den;
    hd->b1 =  0.0f;
    hd->b2 = -(2.0f * k * wn * Ts) / den;
    hd->a1 =  (2.0f * wn2 * Ts2 - 8.0f) / den;
    hd->a2 =  (4.0f - 2.0f * k * wn * Ts + wn2 * Ts2) / den;
    hd->x1 = 0.0f;
    hd->x2 = 0.0f;

    /* ── H_q(z): 正交 DF22 (分母相同) ── */
    DCL_DF22 *hq = &s->df22_q;
    hq->b0 =  (k * wn2 * Ts2) / den;
    hq->b1 =  (2.0f * k * wn2 * Ts2) / den;
    hq->b2 =  (k * wn2 * Ts2) / den;
    hq->a1 = hd->a1;    /* 共享分母系数 */
    hq->a2 = hd->a2;
    hq->x1 = 0.0f;
    hq->x2 = 0.0f;

    /* 保存参数 */
    s->k  = k;
    s->wn = wn;
    s->Ts = Ts;
    s->v_alpha = 0.0f;
    s->v_beta  = 0.0f;
}

void SOGI_Run(SOGI_Handle *s, float vin)
{
    s->v_alpha = DCL_runDF22_C1(&s->df22_d, vin);
    s->v_beta  = DCL_runDF22_C1(&s->df22_q, vin);
}

void SOGI_Reset(SOGI_Handle *s)
{
    s->df22_d.x1 = 0.0f;
    s->df22_d.x2 = 0.0f;
    s->df22_q.x1 = 0.0f;
    s->df22_q.x2 = 0.0f;
    s->v_alpha = 0.0f;
    s->v_beta  = 0.0f;
}

void SOGI_SetFreq(SOGI_Handle *s, float f0)
{
    /* 仅更新频率相关系数, 保留 Ts 和 k */
    SOGI_Init(s, f0, s->Ts, s->k);
}
