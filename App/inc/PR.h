/**
 * @file    PR.h
 * @brief   基于 TI C2000 DCL DF22 的单路准 PR 控制器
 * @details 连续域模型采用
 *          PR(s) = Kp + Ki * (2*wrc*s) / (s^2 + 2*wrc*s + w0^2)，
 *          初始化时通过带预扭曲的双线性变换计算离散系数。
 *          运行时使用 DCL_runDF22_C2/C3 实现输出限幅与条件状态更新。
 */
#ifndef PR_H
#define PR_H

#include "DCLF32.h"

#ifdef __cplusplus
extern "C" {
#endif

/** PR 控制器对象。 */
typedef struct
{
    DCL_DF22 df22;   /**< TI DCL 二阶直接 II 型控制器。 */
    float     Kp;     /**< 比例增益，仅用于参数记录。 */
    float     outMax; /**< 输出上限。 */
    float     outMin; /**< 输出下限。 */
} PR_Handle;

/**
 * @brief 初始化 PR 控制器并清零内部状态。
 * @note  参数或计算结果无效时，控制器保持零输出安全系数和默认限幅 +/-1。
 */
extern void PR_Init(PR_Handle *pr, float Kp, float Ki,
                    float freq, float wrc, float Ts);

/**
 * @brief 设置输出限幅。
 * @note  NaN、Inf 或 max < min 时忽略本次设置，保留上一组有效限幅。
 */
extern void PR_SetClamp(PR_Handle *pr, float max, float min);

/** 清零 DF22 内部状态。 */
extern void PR_Reset(PR_Handle *pr);

/**
 * @brief 计算一次 PR 输出。
 * @note  先计算未限幅输出，只有严格未饱和时才更新 DF22 状态。
 */
extern float PR_Calc(PR_Handle *pr, float err);

#ifdef __cplusplus
}
#endif

#endif /* PR_H */
