/**
 * @file    cla_control_shared.h
 * @brief   C28x ↔ CLA 共享变量声明
 *
 *          这个头文件被两侧同时 include:
 *          - C28x 侧 (Control.c, main.c): 看到的是 extern 声明
 *          - CLA 侧  (.cla 文件):          看到的是同样的 extern 声明
 *
 *          CLA 全局变量不能在 .cla 文件中定义时赋初值,
 *          所有初始化在 Cla1Task8 中完成 (C28x 软件触发).
 *
 * @note    CLA 没有 HWREGH 宏和 math.h, 也没有 GCC 风格 asm 约束.
 *          sin/cos 使用 C28x 预先填充的 400 点查找表, CLA 直接查表.
 *          sqrt 使用 Newton-Raphson 迭代 (3 次足够 RMS 精度).
 *          寄存器访问使用自定义 CLA_HWREGH 宏.
 */

#ifndef CLA_CONTROL_SHARED_H_
#define CLA_CONTROL_SHARED_H_

#include <stdint.h>

/* ================================================================ */
/*  PWM / ADC 硬件常量 — 两端共用                                     */
/* ================================================================ */

#define CLA_TBPRD           2500U   /* EPWM TBPRD: 100MHz/(2*20kHz) */
#define CLA_ADC_VREF        3.3f    /* ADC 参考电压 */
#define CLA_ADC_MAX         4096.0f /* 12-bit ADC 满量程 */
#define CLA_PWM_DB_TICKS    10U     /* EPWM 死区计数: DBRED/DBFED */
#define CLA_CMP_MIN         CLA_PWM_DB_TICKS
#define CLA_CMP_MAX         (CLA_TBPRD - CLA_PWM_DB_TICKS)

/* 400 点整周期: 20kHz / 50Hz = 400 */
#define CLA_WT_PERIOD       400U
#define CLA_WT_STEP_RAD     (6.28318530717959f / 400.0f)   /* 2π/400 */
#define CLA_PI_HALF         (1.5707963267948966f)

/* 调制深度 */
#define CLA_MOD_DEPTH       0.8f

/* RMS / 直流滤波参数 */
#define CLA_RMS_SAMPLES     400U
#define CLA_RMS_ALPHA       0.5f
#define CLA_DC_ALPHA        0.001f

/* VOFA 降频: 20kHz / 20 = 1kHz */
#define CLA_VOFA_DECIM      20U

/* sin 查找表大小 (与 WT_PERIOD 相同) */
#define CLA_SIN_TABLE_SIZE  400U

/* ================================================================ */
/*  CLA 全局变量 — 定义在 .cla 文件中, C28x 通过 extern 访问         */
/*  放在 cla_shared 段 (LS1 RAM), 两端均可读写                       */
/* ================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- DDS 相角发生器 ---- */
extern uint16_t cla_wt_idx;         /* 相角索引 [0, 399] */

/* ---- sin 查找表 (C28x 填充, CLA 读取) ---- */
/* cla_sin_table[i] = sin(2π * i / 400), 由 C28x 用 __sin() 预先填充 */
/* cos(wt) = sin_table[(idx + 100) % 400]  (100 = 400/4 = 90° 相移) */
extern float cla_sin_table[CLA_SIN_TABLE_SIZE];

/* ---- ADC 采样结果 ---- */
extern float cla_adc_raw;           /* ADC 原始码值 (0~4095) */
extern float cla_adc_voltage;       /* ADC 转换后电压 (V) */

/* ---- DDS 正弦参考 ---- */
extern float cla_wt;                /* 当前相角 (rad) [0, 2π) */
extern float cla_sin_ref;           /* 调制波 sin(wt) * 调制深度 */
extern float cla_cos_ref;           /* cos(wt) * 调制深度 */

/* ---- Hilbert 全通正交器 (一阶 DF22) ---- */
/* 直接展开 DF22 系数和状态, 避免 CLA 侧调用 C28x DCL 库 */
extern float cla_hil_kq;            /* 全通系数 kq ≈ -0.98441 */
extern float cla_hil_x1;            /* DF22 延迟 x1 */
extern float cla_hil_beta;          /* 正交输出 (滞后 90°) */

/* ---- 直流低通 + RMS ---- */
extern float cla_adc_dc;            /* 直流分量 (一阶低通) */
extern float cla_adc_ac_rms;        /* 交流有效值 */

/* RMS 累加器 */
extern float cla_rms_sum;           /* Σx */
extern float cla_rms_sumSq;         /* Σx² */
extern uint16_t cla_rms_count;      /* 周期内已采样点数 */
extern float cla_rms_result;        /* IIR 平滑后的 RMS */

/* ---- SPWM 输出 ---- */
extern float cla_cmpa;              /* EPWM1 CMPA 值 */
extern float cla_cmpb;              /* EPWM2 CMPA 值 */

/* ---- ISR 计数 / VOFA 降频 ---- */
extern uint32_t cla_isr_cnt;        /* ISR 执行计数 (调试用) */
extern uint16_t cla_vofa_div;       /* VOFA 降频分频计数器 */

/* ---- CLA → C28x VOFA 发布点 (放共享 RAM) ---- */
extern volatile uint16_t cla_vofa_seq;  /* 每 1ms 递增, 主循环据此抓取快照 */

#ifdef __cplusplus
}
#endif

#endif /* CLA_CONTROL_SHARED_H_ */
