/**
 * @file    VSG.h
 * @brief   简化虚拟同步机 (VSG) — 有功-频率摆动方程
 * @note    离散实现:
 *          J * d(dw)/dt = (Pref - Pget) / w0 - D * dw
 *          w   = w0 + dw
 *          theta = ∫ w dt
 *
 *          输入 Pget 必须是经过低通滤波的平均有功功率,
 *          不能直接用瞬时 v*i。
 *
 *          输出: w (rad/s), theta (rad)
 */

#ifndef APP_INC_VSG_H_
#define APP_INC_VSG_H_

#include <stdint.h>
#include "DCLF32.h"

/* ================================================================ */
/*  常数                                                            */
/* ================================================================ */

#define VSG_TWO_PI      (6.283185307179586f)
#define VSG_W0_50HZ     (314.1592653589793f)

/* ================================================================ */
/*  VSG 控制结构体                                                   */
/* ================================================================ */

typedef struct {
    float32_t Pref;      /* 有功功率给定 (W) */
    float32_t Pget;      /* 实际平均有功功率 (W), 来自 LPF */

    float32_t w0;        /* 额定角频率 (rad/s) */
    float32_t w;         /* 当前角频率 (rad/s) */
    float32_t dw;        /* 频率偏移 (rad/s) */

    float32_t J;         /* 虚拟惯量: 越大, 频率变化越慢 */
    float32_t D;         /* 阻尼系数: 越大, 频率偏移衰减越强 */
    float32_t Ts;        /* 控制周期 (s) */

    float32_t theta;     /* 输出相角 (rad), [0, 2π) */

    float32_t dwMax;     /* 最大频偏 (rad/s), 相对 w0, 例如 +2Hz = +12.566 rad/s */
    float32_t dwMin;     /* 最小频偏 (rad/s), 相对 w0, 例如 -2Hz = -12.566 rad/s */
} VSG_Handle;

/* ================================================================ */
/*  接口函数                                                         */
/* ================================================================ */

void VSG_Init(VSG_Handle *vsg,
              float32_t Pref,
              float32_t w0,
              float32_t J,
              float32_t D,
              float32_t Ts);

void VSG_Update(VSG_Handle *vsg, float32_t Pget);

void VSG_SetPref(VSG_Handle *vsg, float32_t Pref);

void VSG_Reset(VSG_Handle *vsg);

void VSG_SetTheta(VSG_Handle *vsg, float32_t theta);

#endif /* APP_INC_VSG_H_ */
