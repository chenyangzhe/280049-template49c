/**
 * @file    Hilbert.h
 * @brief   一阶全通正交器 (目标频点精确 90° 滞后)
 * @note    v_alpha = vin  (同相, 直通)
 *          v_beta  = 一阶全通滤波器, 在 f0 处精确滞后 90°
 *
 *          全通系数 (目标频点精确 ±90° 解析解):
 *          ω₀  = 2π·f₀/fs
 *          kq  = (sin(ω₀/2) - cos(ω₀/2)) / (sin(ω₀/2) + cos(ω₀/2))
 *
 *          DF22: b0=kq, b1=1, a1=kq, 其余=0
 */

#ifndef APP_INC_HILBERT_H_
#define APP_INC_HILBERT_H_

#include "DCLF32.h"

/* ================================================================ */
/*  Hilbert 正交变换结构体                                           */
/* ================================================================ */

typedef struct {
    DCL_DF22  ap_beta;     /* 正交通道: 一阶全通, 滞后 90° */

    float32_t v_alpha;     /* 同相输出 = vin */
    float32_t v_beta;      /* 正交输出 (滞后 alpha 90°, sin 输入 → -cos) */

    float32_t fs;          /* 采样频率 (Hz), 例如 20000 */
    float32_t f0;          /* 中心频率 (Hz), 例如 50 */
} Hilbert_Handle;

/* ================================================================ */
/*  接口函数                                                         */
/* ================================================================ */

void Hilbert_Init(Hilbert_Handle *h, float32_t fs, float32_t f0);
void Hilbert_Run(Hilbert_Handle *h, float32_t vin);
void Hilbert_Reset(Hilbert_Handle *h);
void Hilbert_SetFreq(Hilbert_Handle *h, float32_t f0);

#endif /* APP_INC_HILBERT_H_ */
