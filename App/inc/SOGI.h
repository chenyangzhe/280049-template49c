/**
 * @file    SOGI.h
 * @brief   二阶广义积分器 (Second Order Generalized Integrator) — 基于 TI DCL DF22
 * @note    连续域传递函数:
 *            H_d(s) = k·ωₙ·s / (s² + k·ωₙ·s + ωₙ²)   同相输出
 *            H_q(s) = k·ωₙ²   / (s² + k·ωₙ·s + ωₙ²)   正交输出
 *
 *          双线性变换离散化, 由 DCL_runDF22_C1 (汇编优化) 执行
 *          典型应用: SOGI-PLL 正交信号发生器 (单相电网锁相)
 */

#ifndef SOGI_H
#define SOGI_H

#include "DCLF32.h"

typedef struct {
    DCL_DF22  df22_d;    /* 同相 H_d(z) — DF22 滤波器 */
    DCL_DF22  df22_q;    /* 正交 H_q(z) — DF22 滤波器 */
    float     k;          /* 阻尼因子 (典型值 √2 ≈ 1.4142) */
    float     wn;         /* 谐振角频率 (rad/s), 如 2π·50 = 314.16 */
    float     Ts;         /* 采样周期 (s) */
    float     v_alpha;    /* 同相输出 (与输入同相) */
    float     v_beta;     /* 正交输出 (滞后 90°) */
} SOGI_Handle;

extern void SOGI_Init(SOGI_Handle *s, float f0, float Ts, float k);
extern void SOGI_Run(SOGI_Handle *s, float vin);
extern void SOGI_Reset(SOGI_Handle *s);
extern void SOGI_SetFreq(SOGI_Handle *s, float f0);

#endif
