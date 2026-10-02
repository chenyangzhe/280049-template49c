/**
 * @file    RMS_algorithm.c
 * @brief   混合式 RMS 有效值算法实现
 * @note    全周期积分 → 去直流 → 开方 → IIR 平滑
 *          每 20ms（一个 50Hz 周期）刷新一次原始值，IIR 平滑避免跳变
 */

#include "RMS_algorithm.h"
#include <math.h>

/* ---- 初始化 ---- */

void RMS_Init(RMS_Handle *obj, uint16_t points, float alpha)
{
    obj->sampleCount = 0;
    obj->points      = points;
    obj->sum         = 0.0f;
    obj->sumSq       = 0.0f;
    obj->dcMean      = 0.0f;
    obj->rmsResult   = 0.0f;
    obj->alpha       = alpha;
}

/* ---- 混合式 RMS 计算（每采样点调用） ---- */

float calcRMS_Hybrid(float data, RMS_Handle *obj)
{
    //
    // 步骤 1: 累加一次项和二次项
    //
    obj->sum   += data;
    obj->sumSq += data * data;
    obj->sampleCount++;

    //
    // 步骤 2: 满一个周期时结算
    //
    if (obj->sampleCount >= obj->points)
    {
        //
        // 去直流: AC_variance = mean_sq - (mean)²
        //
        float mean    = obj->sum   / (float)obj->points;
        obj->dcMean = mean;
        float meanSq  = obj->sumSq / (float)obj->points;
        float acVar   = meanSq - (mean * mean);

        if (acVar < 0.0f) {
            acVar = 0.0f;
        }

        //
        // 开方得到本周期原始 RMS
        // sqrtf 在 FPUfastRTS 链接后由硬件 TMU 加速
        //
        float newRms = sqrtf(acVar);

        //
        // 一阶 IIR 低通: y[n] = y[n-1] + α·(x[n] - y[n-1])
        // α=0.5 时: 新值贡献 50%，旧值保留 50%，指数趋近
        //
        obj->rmsResult = obj->rmsResult + obj->alpha * (newRms - obj->rmsResult);

        //
        // 清零，准备下一个周期
        //
        obj->sum         = 0.0f;
        obj->sumSq       = 0.0f;
        obj->sampleCount = 0;
    }

    //
    // 步骤 3: 返回平滑后的结果（每个采样点都返回，不会跳变）
    //
    return obj->rmsResult;
}
