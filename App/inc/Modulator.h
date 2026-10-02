/**
 * @file    Modulator.h
 * @brief   逆变器调制算法：SPWM / SVPWM / DPWMA
 * @note    统一接口：Z() 零序分量、Control() 占空比映射、Update() 一步到位
 */

#ifndef APP_INC_MODULATOR_H_
#define APP_INC_MODULATOR_H_

#include <stdint.h>

/* ================================================================ */
/*  SPWM — 单极性倍频 SPWM（单相 H 桥）                              */
/* ================================================================ */

/**
 * @brief  SPWM 零序分量（即为 0，无注入）
 */
float SPWM_Z(float Ua, float Ub, float Uc);

/**
 * @brief  SPWM 占空比映射：调制波 [-1, 1] → CMPA [0, TBPRD]
 */
uint32_t SPWM_Control(float Ux, float U0, float TBPRD);

/**
 * @brief  SPWM 一步到位：单相调制波 → 互补两路 CMPA
 * @param  Ua         A 相调制波 [-1.0, 1.0]
 * @param  TBPRD      ePWM 周期值 (2500)
 * @param  CmpA       输出: EPWM1A 比较值 (正向)
 * @param  CmpB       输出: EPWM1B 比较值 (反向)
 */
void SPWM_Update(float Ua, uint16_t TBPRD, uint32_t *CmpA, uint32_t *CmpB);

/* ================================================================ */
/*  SVPWM — Min-Max 注入（等效 SVPWM，母线利用率 ×1.1547）           */
/* ================================================================ */

/**
 * @brief  SVPWM 零序分量 u0 = -(umax+umin)/2
 */
float SVPWM_Z(float Ua, float Ub, float Uc);

/**
 * @brief  SVPWM 占空比映射：注入零序 + 标幺 → CMPA
 */
uint32_t SVPWM_Control(float Ux, float U0, float TBPRD);

/**
 * @brief  SVPWM 一步到位：三相 → 三个 CMPA
 */
void SVPWM_Update(float Va, float Vb, float Vc, uint16_t TBPRD,
                  uint32_t *Comp_A, uint32_t *Comp_B, uint32_t *Comp_C);

/* ================================================================ */
/*  DPWMA — 不连续 PWM (每 60° 钳位一相，开关损耗约为 SVPWM 的 2/3)  */
/* ================================================================ */

/**
 * @brief  DPWMA 零序分量：扇区 + 钳位判断
 */
float DPWMA_Z(float Ua, float Ub, float Uc);

/**
 * @brief  DPWMA 占空比映射（含钳位保护）
 */
uint32_t DPWMA_Control(float Ux, float U0, float TBPRD);

/**
 * @brief  DPWMA 一步到位：三相 → 三个 CMPA（每 60° 钳位一相）
 */
void DPWMA_Update(float Va, float Vb, float Vc, uint16_t TBPRD,
                  uint32_t *Comp_A, uint32_t *Comp_B, uint32_t *Comp_C);

#endif /* APP_INC_MODULATOR_H_ */
