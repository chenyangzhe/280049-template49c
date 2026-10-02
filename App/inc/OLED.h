/**
 * @file    OLED.h
 * @brief   0.96寸 OLED 驱动头文件 (SSD1306, 硬件I2C, 128x64)
 * @note    引脚: GPIO26=SDA, GPIO27=SCL, I2CA
 *          硬件 I2C 由 SysConfig 配置，Board_init() 初始化
 */

#ifndef __OLED_H__
#define __OLED_H__

#include "driverlib.h"

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowFloat(uint8_t Line, uint8_t Column, float Number, uint8_t IntegerLength, uint8_t DecimalLength);

#endif
