/**
 * @file    VSG.c
 * @brief   简化虚拟同步机 (VSG) — 有功-频率摆动方程
 * @note    离散化形式:
 *          dwDot = ((Pref - Pget) / w0 - D * dw) / J
 *          dw    += Ts * dwDot
 *          w      = w0 + dw
 *          theta += Ts * w
 *
 *          调用周期: 与 ISR 同步 (20kHz)
 *          Pget 必须来自 LPF_PQ 的平均有功功率, 不能直接用瞬时 v*i
 *
 *          调试建议: 先用 dwMax/dwMin = ±2*pi*2 (约 ±2Hz)
 *                     确认稳定后再根据需求放宽
 */

#include "VSG.h"

/* ================================================================ */
/*  内部工具函数                                                     */
/* ================================================================ */

static inline float32_t VSG_Clamp(float32_t x, float32_t max, float32_t min)
{
    if (x > max) {
        x = max;
    } else if (x < min) {
        x = min;
    }
    return x;
}

/* ================================================================ */
/*  VSG_Init                                                        */
/* ================================================================ */

void VSG_Init(VSG_Handle *vsg,
              float32_t Pref,
              float32_t w0,
              float32_t J,
              float32_t D,
              float32_t Ts)
{
    vsg->Pref  = Pref;
    vsg->Pget  = 0.0f;

    vsg->w0    = w0;
    vsg->w     = w0;
    vsg->dw    = 0.0f;

    vsg->J     = J;
    vsg->D     = D;
    vsg->Ts    = Ts;

    vsg->theta = 0.0f;

    /* 调试阶段建议先限制在 48Hz~52Hz: dwMax = ±2*pi*2 */
    vsg->dwMax =  12.5663706f;
    vsg->dwMin = -12.5663706f;
}

/* ================================================================ */
/*  VSG_Update — 每个控制周期调用一次 (20kHz ISR)                    */
/*  Pget: 平均有功功率 (来自 LPF_PQ.P)                               */
/*  更新 vsg->w 和 vsg->theta                                        */
/*  周期估计: ~30 cycle (4 次乘加 + 1 次除法 + 限幅), 适合 40kHz ISR */
/* ================================================================ */

#pragma CODE_SECTION(VSG_Update, ".TI.ramfunc");
void VSG_Update(VSG_Handle *vsg, float32_t Pget)
{
    float32_t pErr;
    float32_t dwDot;

    vsg->Pget = Pget;

    /*
     * 摇摆方程:
     * d(dw)/dt = ((Pref - Pget) / w0 - D * dw) / J
     */
    pErr = vsg->Pref - vsg->Pget;

    dwDot = ((pErr / vsg->w0) - (vsg->D * vsg->dw)) / vsg->J;

    vsg->dw += vsg->Ts * dwDot;

    /* 频率偏移限幅, 防止 P 计算异常导致频率飞掉 */
    vsg->dw = VSG_Clamp(vsg->dw, vsg->dwMax, vsg->dwMin);

    vsg->w = vsg->w0 + vsg->dw;

    /* 相角积分 */
    vsg->theta += vsg->Ts * vsg->w;

    if (vsg->theta >= VSG_TWO_PI) {
        vsg->theta -= VSG_TWO_PI;
    } else if (vsg->theta < 0.0f) {
        vsg->theta += VSG_TWO_PI;
    }
}

/* ================================================================ */
/*  VSG_SetPref — 运行时修改功率给定                                 */
/* ================================================================ */

void VSG_SetPref(VSG_Handle *vsg, float32_t Pref)
{
    vsg->Pref = Pref;
}

/* ================================================================ */
/*  VSG_Reset — 故障/软启动/模式切换时清状态                         */
/* ================================================================ */

void VSG_Reset(VSG_Handle *vsg)
{
    vsg->Pget  = 0.0f;
    vsg->dw    = 0.0f;
    vsg->w     = vsg->w0;
    vsg->theta = 0.0f;
}

/* ================================================================ */
/*  VSG_SetTheta — 预同步时对齐相角到 PLL 或其他参考角度              */
/* ================================================================ */

void VSG_SetTheta(VSG_Handle *vsg, float32_t theta)
{
    vsg->theta = theta;
}
