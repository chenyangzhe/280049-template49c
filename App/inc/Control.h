/**
 * @file    Control.h
 * @brief   控制环：单相 DDS + SPWM 单极性倍频
 */

#ifndef APP_INC_CONTROL_H_
#define APP_INC_CONTROL_H_

#include <stdint.h>
#include "PR.h"
#include "Hilbert.h"

/* ================================================================ */
/*  PWM 参数                                                        */
/* ================================================================ */

#define SIN_CARRIER_RATIO   400   /* 载波比 = 20kHz / 50Hz */
#define SIN_TBPRD           2500  /* TBPRD = 100MHz / (2 × 20kHz) */
#define SIN_DEADBAND_MIN    5
#define SIN_DEADBAND_MAX    2495

/* ================================================================ */
/*  观测变量                                                        */
/* ================================================================ */

extern volatile uint32_t g_dbg_isr_cnt;
extern volatile float    g_sin_ref;     /* 调制波 sin θ */
extern volatile float    g_cmpa_val;    /* EPWM1 CMPA (正向) */
extern volatile float    g_cmpb_val;    /* EPWM2 CMPA (反向) */

/* PR */
extern PR_Handle      g_pr;
extern volatile float g_pr_err;
extern volatile float g_pr_out;

/* ADC / RMS */
extern volatile float g_adc_voltage;   /* ADC 实时输入电压 (V) */
extern volatile float g_adc_dc;        /* ADC 直流平均值 (V, 一阶低通) */
extern volatile float g_adc_ac_rms;    /* ADC 交流有效值 (V, 去直流) */

/* Hilbert 正交变换 (VOFA 观测) */
extern Hilbert_Handle g_hilbert;
extern volatile float g_wt;            /* 当前相角 (rad), [0, 2π) */
extern volatile float g_cos_ref;       /* 参考 cos(wt) = sin(wt + π/2) */
extern volatile float g_h_sin;         /* Hilbert 输入 (= sin) */
extern volatile float g_h_alpha;       /* Hilbert 同相输出 */
extern volatile float g_h_beta;        /* Hilbert 正交输出 (sin 输入 → -cos, 滞后 90°) */

/* 诊断变量 (确认真实频率) */
extern volatile float g_vofa_sample_hz;  /* VOFA 实际采样率 = 1000Hz */
extern volatile float g_wt_period_ms;    /* wt 周期 = 20ms */

/* ================================================================ */
/*  初始化                                                          */
/* ================================================================ */

void Control_Init(void);

#endif /* APP_INC_CONTROL_H_ */
