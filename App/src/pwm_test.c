/*
 * pwm_test.c — 分级输出测试（GPIO 翻转 / EPWM 互补 PWM）
 *
 * 刻意不调用 Board_init()：ADC / DMA / CLA / I2C / SCI 一个都不初始化，
 * 把可能干扰输出的东西全部排除掉，EPWM 和 GPIO 都在本文件里现配。
 *
 * 两级，运行中可以在 CCS 的 Expressions 窗口里改 gPwmTestStage 直接切：
 *
 *   1 = GPIO 翻转    GPIO0~3 每 500 ms 翻一次。示波器/万用表/LED 都能看。
 *                    这一级过了，说明：程序在跑、镜像装进去了、引脚通。
 *   2 = EPWM PWM     GPIO0/1 = EPWM1_A/B 互补 + 死区
 *                    GPIO2/3 = EPWM2_A/B 互补 + 死区
 *                    10 kHz，占空比 10%→90% 来回扫（约 3.2 s 一轮）
 *
 * 恢复原工程：注释掉 main.c 里的 PwmTest_Run(); 那一行即可。
 */

#include "device.h"
#include "driverlib.h"
#include "board.h"          /* 只为拿 myEPWM1_EPWMA_PIN_CONFIG 这几个引脚定义 */
#include "PwmTest.h"

/* ================================================================ */
/*  可调参数                                                         */
/* ================================================================ */

#define PWM_TEST_STAGE_DEFAULT      2U      /* 起始档位：1 = GPIO，2 = EPWM */

#define PWM_TEST_BLINK_MS           500U    /* 第 1 级：翻转半周期 ms */

#define PWM_TEST_SWITCH_FREQ_HZ     10000U  /* 第 2 级：开关频率 Hz */
#define PWM_TEST_DEADBAND_NS        200U    /* 第 2 级：上下桥臂死区 ns */
#define PWM_TEST_DEADBAND_ENABLE    1U      /* 1 = 开死区；0 = 纯互补方波（排查用） */

#define PWM_TEST_SWEEP_ENABLE       1U      /* 1 = 占空比来回扫；0 = 固定不动 */
#define PWM_TEST_DUTY_FIXED_PCT     50U     /* SWEEP_ENABLE=0 时的固定占空比 % */
#define PWM_TEST_DUTY_MIN_PCT       10U     /* 扫描下限 %，别贴 0/100，死区会吃掉窄脉冲 */
#define PWM_TEST_DUTY_MAX_PCT       90U     /* 扫描上限 % */
#define PWM_TEST_SWEEP_STEP_PCT     1U      /* 每步 % */
#define PWM_TEST_SWEEP_STEP_MS      20U     /* 每步停留 ms */

/* 运行中可在 CCS Expressions 里直接改，立即切档 */
volatile uint16_t gPwmTestStage = PWM_TEST_STAGE_DEFAULT;

/* ================================================================ */
/*  由上面参数算出来的东西                                           */
/* ================================================================ */

/*
 * SYSCLK = 100 MHz（Device_init() 里配的 PLL）
 * EPWMCLK = SYSCLK / 2 = 50 MHz（PERCLKDIVSEL.EPWMCLKDIV 复位值就是 /2）
 * 上-下计数时一个 PWM 周期 = 2 * TBPRD 个 TBCLK：
 *     TBPRD = EPWMCLK / (2 * fsw) = 50e6 / 20000 = 2500
 * 1 个 TBCLK = 20 ns
 */
#define PWM_TEST_SYSCLK_HZ          ((uint32_t)DEVICE_SYSCLK_FREQ)
#define PWM_TEST_EPWMCLK_HZ         (PWM_TEST_SYSCLK_HZ / 2U)
#define PWM_TEST_TBPRD              ((uint16_t)(PWM_TEST_EPWMCLK_HZ / (2U * PWM_TEST_SWITCH_FREQ_HZ)))
#define PWM_TEST_DB_COUNTS          ((uint16_t)(((uint64_t)PWM_TEST_EPWMCLK_HZ * \
                                                  (uint64_t)PWM_TEST_DEADBAND_NS) / 1000000000ULL))

/* ================================================================ */
/*  公共小工具                                                       */
/* ================================================================ */

/* 粗略延时，只用来控节奏 */
static void PwmTest_DelayMs(uint16_t ms)
{
    while (ms > 0U)
    {
        DEVICE_DELAY_US(1000U);
        ms--;
    }
}

/* ================================================================ */
/*  第 1 级：GPIO 翻转                                               */
/* ================================================================ */

static void PwmTest_PinMuxGpio(void)
{
    EALLOW;
    GPIO_setPinConfig(GPIO_0_GPIO0);
    GPIO_setPinConfig(GPIO_1_GPIO1);
    GPIO_setPinConfig(GPIO_2_GPIO2);
    GPIO_setPinConfig(GPIO_3_GPIO3);
    EDIS;

    GPIO_setDirectionMode(0U, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(1U, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(2U, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(3U, GPIO_DIR_MODE_OUT);

    GPIO_setPadConfig(0U, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(1U, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(2U, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(3U, GPIO_PIN_TYPE_STD);

    GPIO_writePin(0U, 0U);
    GPIO_writePin(1U, 0U);
    GPIO_writePin(2U, 0U);
    GPIO_writePin(3U, 0U);
}

static void PwmTest_StageGpio(void)
{
    for (;;)
    {
        GPIO_writePin(0U, 1U);          /* GPIO0 高 */
        GPIO_writePin(1U, 0U);          /* GPIO1 低 */
        GPIO_writePin(2U, 1U);          /* GPIO2 高 */
        GPIO_writePin(3U, 0U);          /* GPIO3 低 */
        PwmTest_DelayMs(PWM_TEST_BLINK_MS);

        GPIO_writePin(0U, 0U);          /* GPIO0 低 */
        GPIO_writePin(1U, 1U);          /* GPIO1 高 */
        GPIO_writePin(2U, 0U);          /* GPIO2 低 */
        GPIO_writePin(3U, 1U);          /* GPIO3 高 */
        PwmTest_DelayMs(PWM_TEST_BLINK_MS);
    }
}

/* ================================================================ */
/*  第 2 级：EPWM 互补 PWM                                           */
/* ================================================================ */

static void PwmTest_PinMuxEpwm(void)
{
    EALLOW;
    GPIO_setPinConfig(myEPWM1_EPWMA_PIN_CONFIG);    /* GPIO0 = EPWM1_A */
    GPIO_setPinConfig(myEPWM1_EPWMB_PIN_CONFIG);    /* GPIO1 = EPWM1_B */
    GPIO_setPinConfig(myEPWM2_EPWMA_PIN_CONFIG);    /* GPIO2 = EPWM2_A */
    GPIO_setPinConfig(myEPWM2_EPWMB_PIN_CONFIG);    /* GPIO3 = EPWM2_B */
    EDIS;

    GPIO_setPadConfig(myEPWM1_EPWMA_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(myEPWM1_EPWMB_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(myEPWM2_EPWMA_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(myEPWM2_EPWMB_GPIO, GPIO_PIN_TYPE_STD);

    GPIO_setQualificationMode(myEPWM1_EPWMA_GPIO, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(myEPWM1_EPWMB_GPIO, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(myEPWM2_EPWMA_GPIO, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(myEPWM2_EPWMB_GPIO, GPIO_QUAL_SYNC);
}

/* 占空比百分比 → CMPA */
static uint16_t PwmTest_DutyToCmp(uint16_t dutyPct)
{
    return (uint16_t)(((uint32_t)PWM_TEST_TBPRD * (uint32_t)dutyPct) / 100U);
}

/* 同时改两路 EPWM 的占空比（影子装载，CTR=0 时才真正生效） */
static void PwmTest_SetDuty(uint16_t dutyPct)
{
    uint16_t cmp = PwmTest_DutyToCmp(dutyPct);

    EPWM_setCounterCompareValue(EPWM1_BASE, EPWM_COUNTER_COMPARE_A, cmp);
    EPWM_setCounterCompareValue(EPWM2_BASE, EPWM_COUNTER_COMPARE_A, cmp);
}

/*
 * 把一路 EPWM 配成「A/B 互补 + 死区」的普通 PWM。
 *
 * 时基：上-下计数。A 管在 CTR=0 置高、上数过 CMPA 拉低、下数过 CMPA 再置高、
 *       CTR=PRD 拉低 ⇒ 高电平宽度 = 2*CMPA 个 TBCLK，占空比 = CMPA / TBPRD。
 */
static void PwmTest_ConfigPair(uint32_t base, uint16_t tbprd, uint16_t cmp, uint16_t dbCount)
{
    /* ---------- 时基 ---------- */
    EPWM_setClockPrescaler(base, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);
    EPWM_setTimeBasePeriod(base, tbprd);
    EPWM_setTimeBaseCounter(base, 0U);
    EPWM_setTimeBaseCounterMode(base, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_disablePhaseShiftLoad(base);       /* 测试用：两路各跑各的，不互相同步 */
    EPWM_setPhaseShift(base, 0U);

    /* ---------- 比较值：影子装载，CTR=0 时生效 ---------- */
    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_A, cmp);
    EPWM_setCounterCompareShadowLoadMode(base, EPWM_COUNTER_COMPARE_A, EPWM_COMP_LOAD_ON_CNTR_ZERO);
    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_B, 0U);
    EPWM_setCounterCompareShadowLoadMode(base, EPWM_COUNTER_COMPARE_B, EPWM_COMP_LOAD_ON_CNTR_ZERO);

    /* ---------- 动作限定器 ----------
     * A 管：CTR=0 置高 → 上数过 CMPA 拉低 → 下数过 CMPA 再置高 → CTR=PRD 拉低
     * B 管：和 A 完全反相。
     *       DB 模块全开（DBCTL.OUT_MODE=0b11）时 B 由死区模块产生、B 的 AQ 动作
     *       会被旁路，但这里照样写上，两种 DB 配置下结果都是对的。
     */
    EPWM_setActionQualifierShadowLoadMode(base, EPWM_ACTION_QUALIFIER_A, EPWM_AQ_LOAD_ON_CNTR_ZERO);
    EPWM_setActionQualifierShadowLoadMode(base, EPWM_ACTION_QUALIFIER_B, EPWM_AQ_LOAD_ON_CNTR_ZERO);

    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW,  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW,  EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);

    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW,  EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW,  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);

    /* ---------- 死区 ---------- */
#if PWM_TEST_DEADBAND_ENABLE
    /* 两个 EN 位都置上 ⇒ DBCTL.OUT_MODE = 0b11，即 TI 说的 DB_FULL_ENABLE：
     * RED 作用在 A，FED 作用在 B（FED 极性反转，所以 B 是 A 的互补）。
     * 死区计数单位是 TBCLK，200 ns / 20 ns = 10 拍。 */
    EPWM_setDeadBandDelayMode(base, EPWM_DB_RED, true);
    EPWM_setDeadBandDelayMode(base, EPWM_DB_FED, true);
    EPWM_setRisingEdgeDelayCount(base, dbCount);
    EPWM_setFallingEdgeDelayCount(base, dbCount);
    EPWM_setDeadBandDelayPolarity(base, EPWM_DB_FED, EPWM_DB_POLARITY_ACTIVE_LOW);
#else
    /* 关死区 ⇒ DBCTL.OUT_MODE = 0b00，A/B 直接走 AQ 输出，得到两路纯方波 */
    EPWM_setDeadBandDelayMode(base, EPWM_DB_RED, false);
    EPWM_setDeadBandDelayMode(base, EPWM_DB_FED, false);
    (void)dbCount;
#endif

    /* ---------- 确保 trip zone 不会把输出拉成高阻 ----------
     * TZCTL / TZSEL 是 EALLOW 保护的，这里自己括起来，不依赖 driverlib 内部实现。
     * 其实复位后 TZSEL 本来就是 0（没有选中任何触发源），这里只是写死一遍更放心。 */
    EALLOW;
    EPWM_setTripZoneAction(base, EPWM_TZ_ACTION_EVENT_TZA, EPWM_TZ_ACTION_DISABLE);
    EPWM_setTripZoneAction(base, EPWM_TZ_ACTION_EVENT_TZB, EPWM_TZ_ACTION_DISABLE);
    EDIS;
}

static void PwmTest_StageEpwm(void)
{
    uint16_t cmp = PwmTest_DutyToCmp(PWM_TEST_DUTY_FIXED_PCT);

    /* EPWM 外设时钟（Board_init 不管这事，必须自己开） */
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM2);

    PwmTest_PinMuxEpwm();

    PwmTest_ConfigPair(EPWM1_BASE, PWM_TEST_TBPRD, cmp, PWM_TEST_DB_COUNTS);
    PwmTest_ConfigPair(EPWM2_BASE, PWM_TEST_TBPRD, cmp, PWM_TEST_DB_COUNTS);

    /* ---------- 启动计数 ---------- */
    EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setTimeBaseCounterMode(EPWM2_BASE, EPWM_COUNTER_MODE_UP_DOWN);

    /* ★★★ 关键的一句：EPWM 时基时钟总开关 PCLKCR0.TBCLKSYNC ★★★
     *
     * 复位后 TBCLKSYNC = 0，意思是所有 EPWM 的时基全部停摆 —— TBCTR 根本不
     * 会动，动作限定器也就永远不会触发，引脚上什么都出不来。
     *
     * TI 的 Device_init() 里本来有一句 SysCtl_enablePeripheral(
     * SYSCTL_PERIPH_CLK_TBCLKSYNC)，但本工程 RAM 配置用的是 SysConfig 生成的
     * RAM/syscfg/device.c，那份里没有这句；而 SysConfig 生成的
     * RAM/syscfg/board.c 的 SYSCTL_init() 还会显式把它关掉（它是为了让多路
     * EPWM 能对齐着一起起跑），SYNC_init() 里也没有再打开。
     * 原工程的 main.c 同样缺这一步，所以那套 SPWM 也一直没输出。
     *
     * 放在这里：等 EPWM1/EPWM2 都配置完了再打开，两路计数器同时起跑。 */
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    for (;;)
    {
#if PWM_TEST_SWEEP_ENABLE
        int16_t duty;

        for (duty = (int16_t)PWM_TEST_DUTY_MIN_PCT; duty <= (int16_t)PWM_TEST_DUTY_MAX_PCT;
             duty += (int16_t)PWM_TEST_SWEEP_STEP_PCT)
        {
            PwmTest_SetDuty((uint16_t)duty);
            PwmTest_DelayMs(PWM_TEST_SWEEP_STEP_MS);
        }

        for (duty = (int16_t)PWM_TEST_DUTY_MAX_PCT; duty >= (int16_t)PWM_TEST_DUTY_MIN_PCT;
             duty -= (int16_t)PWM_TEST_SWEEP_STEP_PCT)
        {
            PwmTest_SetDuty((uint16_t)duty);
            PwmTest_DelayMs(PWM_TEST_SWEEP_STEP_MS);
        }
#else
        PwmTest_DelayMs(1000U);     /* 固定占空比，什么都不用做 */
#endif
    }
}

/* ================================================================ */
/*  对外入口                                                         */
/* ================================================================ */

void PwmTest_Run(void)
{
    /* ---------- 只做最小初始化 ---------- */
    Device_init();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    /* 注意：这里故意不调 Board_init() —— 不碰 ADC/DMA/CLA/I2C/SCI */

    if (gPwmTestStage == 1U)
    {
        PwmTest_PinMuxGpio();
        PwmTest_StageGpio();        /* 不返回 */
    }
    else
    {
        PwmTest_StageEpwm();        /* 不返回 */
    }
}
