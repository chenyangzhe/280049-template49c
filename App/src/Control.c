/**
 * @file    Control.c
 * @brief   控制环：单相 DDS + SPWM 单极性倍频 + ADC
 * @note    EPWM1 SOCA 触发 ADC → ADCINT1 → DDS 正弦 + SPWM 更新 CMPA
 *
 *          控制环调参原则:
 *          - 外环 Kp < 内环 Kp  (外环带宽必须低于内环, 否则耦合震荡)
 *          - 电压外环典型带宽: 内环的 1/5 ~ 1/10
 *          - 先调好内环 (电流/PR), 再闭环外环 (电压/功率)
 *
 *          工程笔记:
 *          ⽆桥pfc必须每个周期控制⼀次否则会炸
 *          kp稍⼤就会⾃激，⼩了甩负荷的时候会过压带负荷也会掉压
 *          pwm计数器计到0的⼀瞬间触发ADC采集，采集结束后进中断，中断结束时设置占空⽐ 必须 是这个顺序 ⼀步都不能错
 *          解藕是为了解⺟线电压对传递函数的耦，必须是真实⺟线电压
 *          谁发波就⽤谁触发 必须这样 否则永远得不到准确数值
 *          电压波形和电流波形不⼀致时，pfc输出⺟线电容上电压会剧烈抖动，会⼲扰到电压外环运⾏
 *          电流谐波压下去，⺟线电压外环是⼀定100hz陷波器
 *          因为单相⽆桥pfc在pr控制下，⼤多数电流谐波全是电压外带过来的
 *          电流谐波压住，电流内环kp不能太⼩
 *          前级套陷波器是为了怕电压纹波污染电流合成
 *          后级不仅不能陷波还要放⼤100Hz部分
 *          外环kp⼀定⽐kp⼩
 *          pwm完全把限幅甩掉
 *          kr计算⽅法我之前讲过，有前馈的情况下，就等于Iac2πFacL
 *          kr就等于电感上⼯频电压，kr正好是pr⾥⾯50Hz谐振项的增益
 *          ⺟线电压没给够后果太严重了，有桥pfc是电流波形畸变，⽆桥pfc是直接炸掉，pr输出过⼤，导致管⼦⼀直开着
 *          逆变或整流最后计算出来的调制⽐duty都不做限幅
 *          这算出来的只是最⼩kr，实际上kr得在最⼤值Udc和最⼩值之间找平衡
 *          PFC⾥⺟线电压设定是要为输⼊交流有效值的1.6倍，幅度的1.15倍，这是个特殊数值，取这个数值时电流谐波失真可以最低，发热也会最⼩
 *          快慢环是错的，会引起离散步长的问题，离散步长不等引起自激
 */

#include "Control.h"
#include "PR.h"
#include "Modulator.h"
#include "RMS_algorithm.h"
#include "cla_control_shared.h"   /* CLA 共享变量 */
#include "cla.h"                   /* CLA driverlib API */
#include <math.h>
#include <stdint.h>
#include "epwm.h"
#include "adc.h"
#include "interrupt.h"
#include "board.h"

/* ================================================================ */
/*  全局变量                                                        */
/* ================================================================ */

/* 控制 ISR 周期: 20kHz → Ts = 50us */
#define CONTROL_TS (5.0e-5f)

/* 调制深度 0.0~1.0 */
#define MODULATION_DEPTH 0.8f

/* 2π */
#define TWO_PI (6.28318530717959f)

/* ISR 内观测变量 */
volatile uint32_t g_dbg_isr_cnt = 0;
volatile float    g_sin_ref  = 0.0f;   /* 调制波 sin θ */
volatile float    g_cmpa_val = 0.0f;   /* EPWM1 CMPA (正向) */
volatile float    g_cmpb_val = 0.0f;   /* EPWM2 CMPA (反向) */

/* ADC / RMS */
volatile float g_adc_voltage = 0.0f;   /* ADC 实时输入电压 (V) */
volatile float g_adc_dc      = 0.0f;   /* ADC 直流平均值 (V, 一阶低通) */
volatile float g_adc_ac_rms  = 0.0f;   /* ADC 交流有效值 (V, 去直流) */

/* Hilbert 正交变换 */
Hilbert_Handle g_hilbert;
volatile float g_h_sin   = 0.0f;   /* 原始 DDS 正弦 */
volatile float g_h_alpha = 0.0f;   /* Hilbert 同相输出 */
volatile float g_h_beta  = 0.0f;   /* Hilbert 正交输出: sin 输入 → -cos, 滞后 90° */

/* PR 闭环 */
PR_Handle      g_pr;
volatile float g_pr_err = 0.0f;
volatile float g_pr_out = 0.0f;

/* 400 点整周期计数: 20kHz / 50Hz = 400 */
#define WT_PERIOD_SAMPLES (400U)
#define WT_STEP_RAD       (TWO_PI / (float32_t)WT_PERIOD_SAMPLES)   /* 2π/400 ≈ 0.015708 */

volatile float  g_wt     = 0.0f;   /* 当前相角 (rad), [0, 2π) */
volatile float  g_cos_ref = 0.0f;   /* 参考余弦 (超前 sin 90°) */

/* 诊断变量 (确认真实频率, 避免靠 VOFA 时间轴猜) */
volatile float g_vofa_sample_hz = 1000.0f;   /* VOFA 实际采样率: 20kHz / 20 = 1kHz */
volatile float g_wt_period_ms   = 20.0f;     /* wt 锯齿周期: 400点 / 20kHz = 20ms */

/* ADC 参数 */
#define ADC_VREF 3.3f        /* ADC 参考电压 (外部 3.3V) */

/* RMS 计算 */
#define RMS_SAMPLES       400U   /* 20kHz / 50Hz = 400 个采样点 */
#define RMS_FILTER_ALPHA  0.5f
#define ADC_DC_ALPHA      0.001f /* 直流一阶低通系数 */

static RMS_Handle g_adc_rms_obj;

/* VOFA 发送降频: 20kHz / 20 = 1kHz，避免串口带宽被打满 */
#define VOFA_SEND_DECIMATION 20U

/* ================================================================ */
/*  Control_Init                                                    */
/* ================================================================ */

void Control_Init(void)
{
    /* PR 电流环: Kp=3, Ki=40, 谐振50Hz, wrc=5rad/s, Ts=50us */
    PR_Init(&g_pr, 3.0f, 40.0f, 50.0f, 5.0f, CONTROL_TS);
    PR_SetClamp(&g_pr, 0.95f, -0.95f);         /* 输出限幅 ±0.95, 避免过调制 */

    /* RMS 电压有效值滤波器 */
    RMS_Init(&g_adc_rms_obj, RMS_SAMPLES, RMS_FILTER_ALPHA);

    /* Hilbert 正交变换: fs=20kHz, f0=50Hz, sin→cos 滞后 90° */
    Hilbert_Init(&g_hilbert, 20000.0f, 50.0f);
}

/* ================================================================ */
/*  INT_myADC0_1_ISR — 已迁移至 CLA Task1                             */
/* ================================================================ */
/*  原来的 20kHz ADCINT1 ISR 全部逻辑已移植到 CLA:
 *    - ADC 读取        → cla_control.cla Task1 步骤 1
 *    - DDS sin/cos     → Task1 步骤 2 (CLA MSIN 硬件指令)
 *    - Hilbert 全通    → Task1 步骤 3 (直接展开 DF22)
 *    - 直流低通 + RMS  → Task1 步骤 5-6
 *    - SPWM 更新       → Task1 步骤 7-8 (CLA 直接写 EPWM CMPA)
 *    - VOFA 降频       → Task1 步骤 9
 *    - 清中断          → Task1 步骤 10
 *
 *  ADCINT1 现在直接硬件触发 CLA Task1 (CLA_TRIGGER_ADCA1),
 *  不再经过 C28x PIE 中断, C28x 完全释放给通信/显示/MPPT.
 *
 *  C28x 主循环通过检查 cla_vofa_seq 序号 (共享变量) 决定何时抓取 VOFA 快照.
 *  其他 cla_xxx 变量也可直接读取 (都是 float/uint, 单字读写原子).
 *
 *  如果需要调试, 可以在 .cla 文件中取消 __mdebugstop() 注释,
 *  CLA 会在任务入口暂停等待调试器.
 */

/*
 * 空的 ISR 占位 — 保留函数名防止链接器警告,
 * 实际不会被注册到 PIE (board.c 的 INTERRUPT_init 已不注册).
 * 如果 SysConfig 仍然注册了 INT_myADC0_1, 可以在 CCS 中
 * 右键 .syscfg → 修改 ADC 中断配置, 或在此保留空函数.
 */
__interrupt void INT_myADC0_1_ISR(void)
{
    /*
     * 这个函数正常情况下永远不会被执行,
     * 因为 ADCINT1 被 CLA 直接截获.
     * 如果跑到这里说明 CLA 没有正确使能 — 检查 SysConfig.
     */
    ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
