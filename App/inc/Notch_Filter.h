/**
 * @file    Notch_Filter.h
 * @brief   二阶陷波器 (Notch Filter / Band-Stop)
 *          使用 TI DCL DF22 结构，双线性变换 + 预扭曲离散化
 *          H(s) = (s^2 + w0^2) / (s^2 + 2*wrc*s + w0^2)
 */
#ifndef NOTCH_FILTER_H
#define NOTCH_FILTER_H

#include "DCLF32.h"

#ifdef __cplusplus
extern "C" {
#endif

/*========== 类型定义 ==========*/
typedef struct {
    DCL_DF22 df22;    /* TI DCL 二阶滤波器结构体 */
} Notch_Handle;

/*========== 接口函数 ==========*/

/**
 * @brief  初始化陷波器 (双线性变换 + 预扭曲)
 * @param  nf    陷波器句柄
 * @param  freq  陷波中心频率 (Hz), 如 100
 * @param  wrc   带宽参数 (rad/s), 越大带宽越宽; 典型值 2*PI*5 ~ 2*PI*10
 * @param  Ts    采样周期 (s), 如 50us
 */
extern void Notch_Init(Notch_Handle *nf, float freq, float wrc, float Ts);

/**
 * @brief  运行陷波器 (单步)
 * @param  nf     陷波器句柄
 * @param  input  输入信号
 * @return 滤波后输出
 */
extern float Notch_Run(Notch_Handle *nf, float input);

/**
 * @brief  重置内部状态
 */
extern void Notch_Reset(Notch_Handle *nf);

#ifdef __cplusplus
}
#endif

#endif /* NOTCH_FILTER_H */
