/*
 * PwmTest.h — 独立 PWM 输出测试（不跑控制环 / CLA / 中断）
 *
 * 只在调试时用：注释掉 main.c 里的 PwmTest_Run(); 就恢复原来的 SPWM 控制环。
 */

#ifndef PWMTEST_H
#define PWMTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* 配置并启动 EPWM1/EPWM2，然后在死循环里刷占空比。本函数不返回。 */
void PwmTest_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* PWMTEST_H */
