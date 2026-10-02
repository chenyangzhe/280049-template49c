/**
 * @file    OLED.c
 * @brief   0.96寸 OLED 驱动 (SSD1306, 硬件I2C)
 * @note    I2CA, GPIO26=SDA, GPIO27=SCL, 地址 0x3C
 *          硬件 I2C 由 SysConfig 配置，Board_init() 初始化
 */

#include "OLED.h"
#include "OLED_Font.h"
#include "board.h"
#include "driverlib.h"
#include "device.h"

#define OLED_POWER_ON_DELAY_US 100000UL
#define OLED_I2C_TIMEOUT       100000UL

/* ---- 硬件 I2C 写入（带控制字节） ---- */

/**
 * @brief  通过硬件 I2C 向 SSD1306 发送 1 个控制字节 + 1 个数据字节
 * @note   SSD1306 的 I2C 协议要求先发控制字节（区分命令/数据），再发内容
 *         每次传输都是: START → 地址 → 控制字节 → 数据字节 → STOP
 * @param  ctrlByte  控制字节: 0x00=下一字节是命令, 0x40=下一字节是数据
 * @param  data      要发送的命令或数据
 */
static void OLED_HW_Write(uint8_t ctrlByte, uint8_t data)
{
    uint32_t timeout;

    //
    // 设置从机地址（0x3C, 7位）
    //
    I2C_setTargetAddress(oled_BASE, oled_TARGET_ADDRESS);

    //
    // 配置发送模式，数据计数 = 2（控制字节 + 数据字节）
    //
    I2C_setDataCount(oled_BASE, 2);
    I2C_setConfig(oled_BASE, I2C_CONTROLLER_SEND_MODE);
    I2C_sendStartCondition(oled_BASE);

    //
    // 写第1字节: 控制字节（0x00 或 0x40）
    //
    I2C_putData(oled_BASE, ctrlByte);

    //
    // 等待 TX FIFO 有空位（上一字节已发出）
    //
    timeout = OLED_I2C_TIMEOUT;
    while ((I2C_getStatus(oled_BASE) & I2C_STS_REG_ACCESS_RDY) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        I2C_sendStopCondition(oled_BASE);
        return;
    }

    //
    // 写第2字节: 命令或显示数据
    //
    I2C_putData(oled_BASE, data);

    //
    // 发送 STOP 并等待完成
    //
    I2C_sendStopCondition(oled_BASE);
    timeout = OLED_I2C_TIMEOUT;
    while (I2C_getStopConditionStatus(oled_BASE) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        return;
    }

    //
    // 等待总线释放，确保下次传输安全
    //
    timeout = OLED_I2C_TIMEOUT;
    while (I2C_isBusBusy(oled_BASE) && (timeout > 0U)) {
        timeout--;
    }
}

static void OLED_WriteCommand(uint8_t Command)
{
    OLED_HW_Write(0x00, Command);   // 控制字节 0x00 = 命令
}

static void OLED_WriteData(uint8_t Data)
{
    OLED_HW_Write(0x40, Data);      // 控制字节 0x40 = 数据
}

/* ---- OLED 初始化 ---- */

void OLED_Init(void)
{
    // 等 OLED 内部电源和控制器稳定。空 for 延时在 O2 下可能被优化掉。
    DEVICE_DELAY_US(OLED_POWER_ON_DELAY_US);

    //
    // GPIO 和 I2C 外设已由 Board_init() 完成初始化：
    //   PinMux:  GPIO26 → I2CA_SDA, GPIO27 → I2CA_SCL（开漏+上拉）
    //   I2CA:    控制器模式, 400kHz, 7位从机地址 0x3C
    // 此处不再重复配置引脚
    //

    //
    // SSD1306 初始化序列（参考 SSD1306 Rev2.2 数据手册）
    // 完整列表: 关显示 → 配时钟/复用/偏移 → 使能电荷泵 → 设地址模式/扫描方向 → 调对比度 → 开显示
    //
    OLED_WriteCommand(0xAE);    //  1. 关显示 (Display OFF)
    OLED_WriteCommand(0xD5);    //  2. 显示时钟分频 / 振荡器频率
    OLED_WriteCommand(0x80);    //     DCLK = Fosc, 分频 = 1
    OLED_WriteCommand(0xA8);    //  3. 多路复用比
    OLED_WriteCommand(0x3F);    //     64 行 (MUX = 63)
    OLED_WriteCommand(0xD3);    //  4. 显示垂直偏移
    OLED_WriteCommand(0x00);    //     偏移 = 0
    OLED_WriteCommand(0x40);    //  5. 显示起始行 = 0
    OLED_WriteCommand(0x8D);    //  6. 电荷泵设置
    OLED_WriteCommand(0x14);    //     使能电荷泵 (3.3V → 内部升压)
    OLED_WriteCommand(0x20);    //  7. 地址模式
    OLED_WriteCommand(0x02);    //     页寻址模式 (Page Addressing)
    OLED_WriteCommand(0xA1);    //  8. 段重映射: 列 127 → SEG0（水平镜像）
    OLED_WriteCommand(0xC8);    //  9. COM 扫描方向: COM63 → COM0（垂直翻转）
    OLED_WriteCommand(0xDA);    // 10. COM 引脚硬件配置
    OLED_WriteCommand(0x12);    //     顺序模式, 不禁用左右重映射
    OLED_WriteCommand(0x81);    // 11. 对比度
    OLED_WriteCommand(0xCF);    //     对比度 = 207 (0xCF)
    OLED_WriteCommand(0xD9);    // 12. 预充电周期
    OLED_WriteCommand(0xF1);    //     Phase1=1, Phase2=15
    OLED_WriteCommand(0xDB);    // 13. VCOMH 电压
    OLED_WriteCommand(0x40);    //     ~0.77 × VCC
    OLED_WriteCommand(0xA4);    // 14. 正常显示（不反白）
    OLED_WriteCommand(0xA6);    // 15. 正常模式（0=灭, 1=亮）
    OLED_WriteCommand(0x2E);    // 16. 关闭水平滚动
    OLED_WriteCommand(0xAF);    // 17. 开显示 (Display ON)

    OLED_Clear();               // 清空 GDRAM，初始化所有像素为灭
}

/* ---- 清屏 ---- */

/**
 * @brief  清空整个屏幕（全部像素写入 0）
 * @note   128x64 = 8 页 × 128 列，逐页逐列写入 0x00
 *         每次设置页地址和列地址后连续写入 128 字节
 */
void OLED_Clear(void)
{
    uint8_t i, j;
    for (i = 0; i < 8; i++)                     // 8 页 (Page 0~7)
    {
        OLED_WriteCommand(0xB0 + i);            // 设置页地址
        OLED_WriteCommand(0x10);                // 列地址高4位 = 0
        OLED_WriteCommand(0x00);                // 列地址低4位 = 0（从第0列开始）
        for (j = 0; j < 128; j++)
            OLED_WriteData(0x00);               // 该列 8 个像素全部熄灭
    }
}

/* ---- 字符显示 ---- */

/**
 * @brief  在指定位置显示一个 8x16 大小字符
 * @note   使用 OLED_F8x16 字库，每个字符占 2 页（8 像素/页 × 2 = 16 像素高）
 *         Line: 1~4 行, Column: 1~16 列
 *         字体为 ASCII，从空格(32)开始索引
 * @param  Line   行号 (1~4)
 * @param  Column 列号 (1~16)
 * @param  Char   要显示的 ASCII 字符
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    uint8_t page;
    // 设置页地址（每行占 2 页，8x16 字体）
    page = (Line - 1) * 2;
    OLED_WriteCommand(0xB0 + page);                                // 设置上半页
    OLED_WriteCommand(0x10 | (((Column - 1) * 8) >> 4));          // 列地址高4位
    OLED_WriteCommand(((Column - 1) * 8) & 0x0F);                  // 列地址低4位

    for (i = 0; i < 8; i++)
        OLED_WriteData(OLED_F8x16[(uint8_t)Char - 32][i]);        // 字库从空格(ASCII 32)开始，写入上8行

    OLED_WriteCommand(0xB0 + page + 1);                            // 设置下半页
    OLED_WriteCommand(0x10 | (((Column - 1) * 8) >> 4));
    OLED_WriteCommand(((Column - 1) * 8) & 0x0F);

    for (i = 0; i < 8; i++)
        OLED_WriteData(OLED_F8x16[(uint8_t)Char - 32][i + 8]);    // 写入下8行
}

/**
 * @brief  在指定位置显示字符串（8x16 字体）
 * @param  Line   行号 (1~4)
 * @param  Column 起始列号 (1~16)
 * @param  String 要显示的字符串（以 '\0' 结尾）
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    while (*String)                    // 逐字符显示直到 '\0'
    {
        OLED_ShowChar(Line, Column, *String);
        Column++;
        String++;
    }
}

/* ---- 数字显示 ---- */

/**
 * @brief  显示无符号整数（右对齐，前导空格）
 * @param  Line   行号 (1~4)
 * @param  Column 起始列号
 * @param  Number 要显示的数值
 * @param  Length 显示位数
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    char str[12];
    uint8_t i;
    for (i = Length; i > 0; i--)          // 从低位到高位逐位取模转字符
    {
        str[i - 1] = Number % 10 + '0';
        Number /= 10;
    }
    str[Length] = '\0';
    OLED_ShowString(Line, Column, str);
}

/**
 * @brief  显示有符号整数（带正负号）
 * @param  Line   行号 (1~4)
 * @param  Column 起始列号
 * @param  Number 有符号数值
 * @param  Length 数值部分位数（不含符号位）
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    if (Number < 0)
    {
        OLED_ShowChar(Line, Column++, '-');
        Number = -Number;
    }
    else
    {
        OLED_ShowChar(Line, Column++, '+');
    }
    OLED_ShowNum(Line, Column, (uint32_t)Number, Length);
}

/**
 * @brief  显示十六进制数
 * @param  Line   行号
 * @param  Column 起始列号
 * @param  Number 要显示的数值
 * @param  Length 显示位数（4 bits/位）
 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    char str[12];
    uint8_t i;
    for (i = Length; i > 0; i--)
    {
        uint8_t nibble = Number & 0x0F;                           // 取低4位
        str[i - 1] = nibble < 10 ? nibble + '0' : nibble - 10 + 'A';
        Number >>= 4;
    }
    str[Length] = '\0';
    OLED_ShowString(Line, Column, str);
}

/**
 * @brief  显示二进制数
 * @param  Line   行号
 * @param  Column 起始列号
 * @param  Number 要显示的数值
 * @param  Length 显示位数
 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    char str[33];                           // 最多 32 位 + '\0'
    uint8_t i;
    for (i = Length; i > 0; i--)
    {
        str[i - 1] = (Number & 0x01) + '0';
        Number >>= 1;
    }
    str[Length] = '\0';
    OLED_ShowString(Line, Column, str);
}

/**
 * @brief  显示浮点数
 * @param  Line          行号
 * @param  Column        起始列号
 * @param  Number        要显示的浮点数
 * @param  IntegerLength 整数部分位数（含符号位）
 * @param  DecimalLength 小数部分位数
 */
void OLED_ShowFloat(uint8_t Line, uint8_t Column, float Number, uint8_t IntegerLength, uint8_t DecimalLength)
{
    /* 先显示整数部分（含符号位），并计算实际消耗列数 */
    uint8_t col = Column;
    if (Number < 0)
    {
        OLED_ShowChar(Line, col++, '-');
        Number = -Number;
    }
    else
    {
        OLED_ShowChar(Line, col++, '+');
    }
    OLED_ShowNum(Line, col, (uint32_t)Number, IntegerLength);
    col += IntegerLength;

    /* 小数点 */
    OLED_ShowChar(Line, col++, '.');

    /* 小数部分 */
    uint8_t decLen = DecimalLength;
    Number -= (uint32_t)Number;
    while (decLen--)
        Number *= 10;
    OLED_ShowNum(Line, col, (uint32_t)Number, DecimalLength);
}
