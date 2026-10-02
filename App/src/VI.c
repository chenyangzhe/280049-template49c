/**
 * @file    VI.c
 * @brief   虚拟阻抗 (Virtual Admittance) — DCL DF22 实现
 * @note    连续域传递函数: Y(s) = 1 / (sL + R)
 *
 *          双线性变换 s = (2/Ts)·(z-1)/(z+1):
 *
 *          H(z) = (1 + z⁻¹) / ((2L/Ts+R) + (R-2L/Ts)·z⁻¹)
 *
 *          归一化为 DF22 标准形式 y = b0·x + b1·x⁻¹ - a1·y⁻¹:
 *            b0 = b1 = Ts / (2L + R·Ts)
 *            a1 = (R·Ts - 2L) / (2L + R·Ts)
 *            a2 = 0,  b2 = 0
 *
 *          物理含义: 电压误差经过虚拟导纳 1/(sL+R) 生成电流基准
 *                   使逆变器对外呈现阻抗 Z(s) = sL + R
 */

#include "VI.h"

void VI_Init(VI_Handle *vi, float L, float R, float Ts)
{
    DCL_DF22 *h = &vi->df22;

    /* 计算分母 den = 2L + R·Ts */
    float den = 2.0f * L + R * Ts;

    /* DF22 系数 */
    h->b0 = Ts / den;
    h->b1 = Ts / den;        /* 分子 (1 + z⁻¹), b0 == b1 */
    h->b2 = 0.0f;
    h->a1 = (R * Ts - 2.0f * L) / den;
    h->a2 = 0.0f;

    /* 清空状态变量 */
    h->x1 = 0.0f;
    h->x2 = 0.0f;

    /* 保存参数 */
    vi->L      = L;
    vi->R      = R;
    vi->Ts     = Ts;
    vi->outMax =  1e6f;   /* 默认不限制 */
    vi->outMin = -1e6f;
}

float VI_Calc(VI_Handle *vi, float err)
{
    float out = DCL_runDF22_C1(&vi->df22, err);

    if (out > vi->outMax) out = vi->outMax;
    if (out < vi->outMin) out = vi->outMin;

    return out;
}

void VI_SetClamp(VI_Handle *vi, float max, float min)
{
    vi->outMax = max;
    vi->outMin = min;
}

void VI_Reset(VI_Handle *vi)
{
    vi->df22.x1 = 0.0f;
    vi->df22.x2 = 0.0f;
}

void VI_UpdateParams(VI_Handle *vi, float L, float R)
{
    /* 从结构体恢复 Ts */
    float Ts = vi->Ts;
    if (Ts == 0.0f) return;

    vi->L = L;
    vi->R = R;

    float den = 2.0f * L + R * Ts;
    vi->df22.b0 = Ts / den;
    vi->df22.b1 = Ts / den;
    vi->df22.a1 = (R * Ts - 2.0f * L) / den;
}
