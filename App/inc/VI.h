/**
 * @file    VI.h
 * @brief   虚拟阻抗 (Virtual Admittance) — 基于 TI DCL DF22
 * @note    连续域: Y(s) = 1 / (sL + R)
 *          双线性变换离散化, 由 DCL_runDF22_C1 (汇编优化) 执行
 *          工程中实际效果: 电压误差 → 电流基准
 */

#ifndef VI_H
#define VI_H

#include "DCLF32.h"

typedef struct {
    DCL_DF22  df22;      /* TI DCL 二阶滤波器 */
    float     L;          /* 虚拟电感 (H) */
    float     R;          /* 虚拟电阻 (Ω) */
    float     Ts;         /* 采样周期 (s) */
    float     outMax;     /* 输出上限 */
    float     outMin;     /* 输出下限 */
} VI_Handle;

extern void VI_Init(VI_Handle *vi, float L, float R, float Ts);
extern void VI_SetClamp(VI_Handle *vi, float max, float min);
extern void VI_Reset(VI_Handle *vi);
extern void VI_UpdateParams(VI_Handle *vi, float L, float R);
extern float VI_Calc(VI_Handle *vi, float err);

#endif
