/**
 * @file    Hilbert.c
 * @brief   一阶全通正交器 — 目标频点精确 90° 滞后
 * @note    v_alpha = vin  (同相, 直通)
 *          v_beta  = 一阶全通 H(z) = (kq + z⁻¹) / (1 + kq·z⁻¹)
 *                    在 f0 处精确相位滞后 90°
 *
 *          系数:
 *          ω₀  = 2π·f₀/fs
 *          kq  = (sin(ω₀/2) - cos(ω₀/2)) / (sin(ω₀/2) + cos(ω₀/2))
 *
 *          fs=20000, f0=50 时, kq ≈ -0.98441
 *
 *          调用周期: 与 ISR 同步 (20kHz)
 *          周期估计: 1 × DCL_runDF22_C4 ≈ 15 cycle
 */

#include "Hilbert.h"
#include <math.h>

/* ================================================================ */
/*  常数                                                            */
/* ================================================================ */

#define HILBERT_TWO_PI   (6.283185307179586f)
#define HILBERT_PI_OVER2 (1.5707963267948966f)

/* ================================================================ */
/*  工具: 计算正交系数 kq (目标频点精确 90°)                         */
/*  kq = (sin(ω₀/2) - cos(ω₀/2)) / (sin(ω₀/2) + cos(ω₀/2))         */
/*  派生: 一阶全通 H(z) = (kq + z⁻¹) / (1 + kq·z⁻¹),                 */
/*        在 ω₀ 处 ∠H(e^{jω₀}) = -90°                               */
/* ================================================================ */

static float32_t Hilbert_CalcKq(float32_t fs, float32_t f0)
{
    float32_t w0, w0_half, sin_half, cos_half, denom;

    w0       = HILBERT_TWO_PI * f0 / fs;
    w0_half  = w0 * 0.5f;

    /* cos(w0/2) = sin(w0/2 + π/2), 两个都走 TMU __sin */
    cos_half = __sin(w0_half + HILBERT_PI_OVER2);
    sin_half = __sin(w0_half);

    denom = sin_half + cos_half;
    if (denom < 1.0e-6f && denom > -1.0e-6f) {
        denom = 1.0f;   /* 防御: f0→0 或 f0→fs/2 极端情况 */
    }

    return (sin_half - cos_half) / denom;
}

/* ================================================================ */
/*  工具: 给 DF22 填入一阶全通系数                                    */
/*  H(z) = (k + z⁻¹) / (1 + k·z⁻¹)                                  */
/*  DF22: b0=k, b1=1, b2=0, a1=k, a2=0                              */
/* ================================================================ */

static void Hilbert_SetAPSect(DCL_DF22 *ap, float32_t k)
{
    ap->b0  = k;
    ap->b1  = 1.0f;
    ap->b2  = 0.0f;
    ap->a1  = k;
    ap->a2  = 0.0f;
    ap->x1  = 0.0f;
    ap->x2  = 0.0f;
    ap->sps = NULL_ADDR;
    ap->css = NULL_ADDR;
}

/* ================================================================ */
/*  Hilbert_Init                                                    */
/* ================================================================ */

void Hilbert_Init(Hilbert_Handle *h, float32_t fs, float32_t f0)
{
    float32_t kq;

    h->fs = fs;
    h->f0 = f0;

    kq = Hilbert_CalcKq(fs, f0);

    Hilbert_SetAPSect(&h->ap_beta, kq);

    h->v_alpha = 0.0f;
    h->v_beta  = 0.0f;
}

/* ================================================================ */
/*  Hilbert_Run — 每个控制周期调用一次 (20kHz ISR)                    */
/*  vin: 输入信号 (如 DDS 正弦)                                      */
/*  v_alpha = vin (直通)                                             */
/*  v_beta  = 一阶全通, 在 f0 处精确滞后 90°                          */
/* ================================================================ */

#pragma CODE_SECTION(Hilbert_Run, ".TI.ramfunc");
void Hilbert_Run(Hilbert_Handle *h, float32_t vin)
{
    h->v_alpha = vin;
    h->v_beta  = DCL_runDF22_C4(&h->ap_beta, vin);
}

/* ================================================================ */
/*  Hilbert_Reset — 清状态, v_alpha/v_beta 归零                      */
/* ================================================================ */

void Hilbert_Reset(Hilbert_Handle *h)
{
    h->ap_beta.x1 = 0.0f;
    h->ap_beta.x2 = 0.0f;

    h->v_alpha = 0.0f;
    h->v_beta  = 0.0f;
}

/* ================================================================ */
/*  Hilbert_SetFreq — 频率变化时在线更新系数                         */
/*  注意: 直接修改 active 系数, 非原子操作                           */
/*        建议在停机/同步点调用, 或加关中断临界区                     */
/* ================================================================ */

void Hilbert_SetFreq(Hilbert_Handle *h, float32_t f0)
{
    float32_t kq;

    h->f0 = f0;
    kq   = Hilbert_CalcKq(h->fs, f0);

    h->ap_beta.b0 = kq;
    h->ap_beta.a1 = kq;
}
