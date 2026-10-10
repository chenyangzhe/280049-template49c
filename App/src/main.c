/*
 * main.c — 单极性倍频 SPWM 控制环
 *
 *  ISR: EPWM1 SOCA 触发 ADC → ADCINT1 → DDS 正弦 + SPWM 更新 CMPA
 *  后台: 死循环
 */

#include "device.h"
#include "driverlib.h"
#include "board.h"
#include "Control.h"
#include "cla_control_shared.h"
#include "interrupt.h"
#include "vofa.h"
#include "vofa_drv.h"
#include "vofa_qt.h"
#include "PwmTest.h"        /* PWM 测试，见 App/src/pwm_test.c */

/* ================================================================ */
/*  主函数                                                           */
/* ================================================================ */

int main(void)

{
    uint16_t vofaSeqSeen;

    /* ↓↓↓ 【PWM 测试入口】不返回。
     *     要恢复原来的 SPWM 控制环，把下面这一行注释掉即可。
     *     想调频率/占空比/死区，改 App/src/pwm_test.c 顶部那几个宏。 */
    // PwmTest_Run();

    /* ---------- 基础初始化 ---------- */
    Device_init();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Board_init();
    // /* ---------- GPIO7（丝印 04B）拉高 ----------
    //  * 注意：GPIO_writePin 第一个参数是引脚号，写 led（=7）或直接写 7。
    //  *       写 GPIO_7_GPIO7 是错的 —— 那是给 GPIO_setPinConfig 用的复用配置值
    //  *       (0x00060E00)，当引脚号传进去会算到一个乱地址上，引脚不会动。
    //  *
    //  * 下面把复用/方向/焊盘显式再写一遍（绕开 led_init 那条间接路径），
    //  * 排查完可以只留最后一句 GPIO_writePin(led, 1); */
    // EALLOW;
    // GPIO_setPinConfig(GPIO_7_GPIO7);                 /* 复用回普通 GPIO */
    // GPIO_setPadConfig(led, GPIO_PIN_TYPE_STD);       /* 推挽，关上下拉 */
    // GPIO_setQualificationMode(led, GPIO_QUAL_SYNC);
    // GPIO_setDirectionMode(led, GPIO_DIR_MODE_OUT);   /* 方向：输出 */
    // GPIO_setControllerCore(led, GPIO_CORE_CPU1);
    // EDIS;
    GPIO_writePin(led, 1);                           /* 拉高 */
    /* ---------- 控制环初始化（PR） ---------- */
    Control_Init();

    /* ---------- Vofa 初始化 ---------- */
    VOFA_Drv_Init();
    VOFA_QT_Init();
    VOFA_Init();
    VOFA_Set_Channel_Input(0U, &cla_sin_ref);     /* ch0: CLA DDS 50Hz 正弦 */
    Interrupt_register(INT_SCIA_RX, &VOFA_Drv_SCI_RxISR);
    Interrupt_register(INT_SCIA_TX, &VOFA_Drv_SCI_TxISR);
    Interrupt_enable(INT_SCIA_RX);
    Interrupt_enable(INT_SCIA_TX);
    vofaSeqSeen = cla_vofa_seq;

    /* ---------- 使能 EPWM1/EPWM2 时钟 ---------- */
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM2);

    /* ADC 中断由 Board_init() 里的 INTERRUPT_init() 自动注册 */
    Interrupt_enableGlobal();

    /* ---------- 启动 EPWM1/EPWM2 UP-DOWN 计数 ---------- */
    /* ★ EPWM 时基时钟总开关。SysConfig 生成的 SYSCTL_init() 会主动把
     *   PCLKCR0.TBCLKSYNC 清零（为了让多路 EPWM 对齐着一起起跑），
     *   而 TI 的 Device_init() 里那句 enable 在 SysConfig 生成的
     *   RAM/syscfg/device.c 里又没有 —— 结果就是 TBCTR 一直是 0，
     *   引脚上什么都出不来。必须在启动计数之前把它打开。 */
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setTimeBaseCounterMode(EPWM2_BASE, EPWM_COUNTER_MODE_UP_DOWN);


    /* ---------- 主循环 ---------- */
    for (;;)
    {
        uint16_t vofaSeqNow = cla_vofa_seq;
        if (vofaSeqNow != vofaSeqSeen)
        {
            vofaSeqSeen = vofaSeqNow;
            VOFA_Request_Send();
        }

        VOFA_Run();
        VOFA_QT_Service();
    }
}
