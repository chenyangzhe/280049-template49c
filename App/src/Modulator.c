/*
 * Modulator.c — 逆变器调制算法：SPWM / SVPWM / DPWMA
 *
 *  Created on: 2026年6月13日
 *      Author: template49c
 *
 *  统一接口:
 *    Z()       → 零序分量计算
 *    Control() → 调制波 + 零序 → CMPA 占空比
 *    Update()  → 一步到位输出
 */

#include "Modulator.h"

/* ================================================================ */
/*  SPWM — 单极性倍频 SPWM（单相 H 桥）                              */
/* ================================================================ */

float SPWM_Z(float Ua, float Ub, float Uc)
{
    (void)Ub; (void)Uc;
    return 0.0f;  /* 无零序注入 */
}

uint32_t SPWM_Control(float Ux, float U0, float TBPRD)
{
    float duty = (Ux + U0 + 1.0f) * TBPRD * 0.5f;
    return (uint32_t)(duty + 0.5f);
}

void SPWM_Update(float Ua, uint16_t TBPRD, uint32_t *CmpA, uint32_t *CmpB)
{
    float u0 = SPWM_Z(Ua, 0.0f, 0.0f);

    *CmpA = SPWM_Control( Ua, u0, (float)TBPRD);   /* 正向 */
    *CmpB = SPWM_Control(-Ua, u0, (float)TBPRD);   /* 反向（互补） */
}

/* ================================================================ */
/*  SVPWM_Z — 零序分量 u0 = -(max+min)/2                            */
/* ================================================================ */
float SVPWM_Z(float Ua, float Ub, float Uc)
{
    float umax, umin;

    (Ua > Ub) ? (umax = Ua, umin = Ub) : (umax = Ub, umin = Ua);
    umax = (Uc > umax) ? Uc : umax;
    umin = (Uc < umin) ? Uc : umin;

    return -(umax + umin) * 0.5f;
}

/* ================================================================ */
/*  SVPWM_Control — 注入零序 + 标幺 → CMPA (四舍五入)               */
/* ================================================================ */
uint32_t SVPWM_Control(float Ux, float U0, float TBPRD)
{
    return (uint32_t)((Ux + U0 + 1.0f) * TBPRD * 0.5f + 0.5f);
}

/* ================================================================ */
/*  SVPWM_Update — 一步到位: 三相 → 三个 CMPA 值                     */
/* ================================================================ */
void SVPWM_Update(float Va, float Vb, float Vc, uint16_t TBPRD,
                  uint32_t *Comp_A, uint32_t *Comp_B, uint32_t *Comp_C)
{
    float u0 = SVPWM_Z(Va, Vb, Vc);

    *Comp_A = SVPWM_Control(Va, u0, (float)TBPRD);
    *Comp_B = SVPWM_Control(Vb, u0, (float)TBPRD);
    *Comp_C = SVPWM_Control(Vc, u0, (float)TBPRD);
}

/* ================================================================ */
/*  DPWMA_Z — 不连续 PWM 零序分量 (zyj 原版)                         */
/* ================================================================ */
float DPWMA_Z(float Ua, float Ub, float Uc)
{
    float a1, a2, b1, b2, c1, c2, z1, z2;

    /* A 相分解 */
    if (Ua > 0.0f) {
        a1 = 1.0001f - Ua;
        a2 = Ua;
    } else {
        a1 = -Ua;
        a2 = 1.0001f + Ua;
    }

    /* B 相分解 */
    if (Ub > 0.0f) {
        b1 = 1.0001f - Ub;
        b2 = Ub;
    } else {
        b1 = -Ub;
        b2 = 1.0001f + Ub;
    }

    /* C 相分解 */
    if (Uc > 0.0f) {
        c1 = 1.0001f - Uc;
        c2 = Uc;
    } else {
        c1 = -Uc;
        c2 = 1.0001f + Uc;
    }

    /* min(a1, b1, c1) */
    z1 = a1;
    if (b1 < z1) z1 = b1;
    if (c1 < z1) z1 = c1;

    /* min(a2, b2, c2) */
    z2 = a2;
    if (b2 < z2) z2 = b2;
    if (c2 < z2) z2 = c2;

    /* 扇区判断: z1 > z2 → 底部钳位, 否则顶部钳位 */
    return (z1 > z2) ? -z2 : z1;
}

/* ================================================================ */
/*  DPWMA_Control — 占空比映射 + 钳位保护                            */
/* ================================================================ */
uint32_t DPWMA_Control(float Ux, float U0, float TBPRD)
{
    float upper = TBPRD + 1.0f;
    float lower = 0.0f;

    float duty = (Ux + 1.0f + U0) * TBPRD * 0.5f;

    if (duty >= upper) {
        duty = upper + 1.0f;        /* 上钳: 强制 100% 占空比 */
    } else if (duty <= lower) {
        duty = lower;               /* 下钳: 强制 0% 占空比   */
    }

    return (uint32_t)(duty + 0.5f); /* 四舍五入 */
}

/* ================================================================ */
/*  DPWMA_Update — 一步到位: 三相 → 三个 CMPA 值                     */
/* ================================================================ */
void DPWMA_Update(float Va, float Vb, float Vc, uint16_t TBPRD,
                  uint32_t *Comp_A, uint32_t *Comp_B, uint32_t *Comp_C)
{
    float u0 = DPWMA_Z(Va, Vb, Vc);

    *Comp_A = DPWMA_Control(Va, u0, (float)TBPRD);
    *Comp_B = DPWMA_Control(Vb, u0, (float)TBPRD);
    *Comp_C = DPWMA_Control(Vc, u0, (float)TBPRD);
}
