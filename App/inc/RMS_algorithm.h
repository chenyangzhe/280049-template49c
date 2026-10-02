/**
 * @file    RMS_algorithm.h
 * @brief   混合式 RMS 算法头文件
 * @note    采样点数建议为整周期（50Hz 下 400 点 → 20kHz 采样率）
 *          IIR 平滑系数默认 0.5，可在初始化时调整
 */

#ifndef APP_INC_RMS_ALGORITHM_H_
#define APP_INC_RMS_ALGORITHM_H_

#include <stdint.h>

/* ---- RMS 控制对象 ---- */

typedef struct {
    uint16_t sampleCount;   ///< 当前周期已采样点数
    uint16_t points;        ///< 一个周期总点数（如 400 点 = 20ms@20kHz）
    float    sum;           ///< 一次项累加 Σx → 算 DC 分量
    float    sumSq;         ///< 二次项累加 Σx² → 算均方值
    float    dcMean;        ///< 直流分量 = Σx / N
    float    rmsResult;     ///< 最终输出的交流有效值
    float    alpha;         ///< IIR 平滑系数 (0~1)，越小越平滑
} RMS_Handle;

/* ---- 初始化 ---- */

/**
 * @brief  初始化 RMS 控制对象
 * @param  obj     RMS 句柄指针
 * @param  points  一个周期的采样点数（50Hz→400点@20kHz, 60Hz→333点@20kHz）
 * @param  alpha   IIR 平滑系数（0.5: 适中, 0.1: 很平滑, 1.0: 不平滑）
 */
void RMS_Init(RMS_Handle *obj, uint16_t points, float alpha);

/* ---- 每采样点调用 ---- */

/**
 * @brief  每采样点调用一次，返回平滑后的交流有效值
 * @param  obj   RMS 句柄指针
 * @param  data  当前采样值
 * @return 交流真有效值（已去直流 + IIR 平滑）
 * @note   每次返回的都是 IIR 平滑后的值，不会在周期边界跳变
 *         ISR 中调用，主循环中读取返回值
 */
float calcRMS_Hybrid(float data, RMS_Handle *obj);

#endif /* APP_INC_RMS_ALGORITHM_H_ */
