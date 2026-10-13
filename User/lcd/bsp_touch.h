/**
  ******************************************************************************
  * @file    bsp_touch.h
  * @brief   3.2 寸模块的电阻触摸（XPT2046，软件模拟 SPI）
  *
  *  引脚（野火 3.2/2.8 寸模块的触摸信号，走液晶排线）：
  *     SPI_CLK  = PG9      T_CS = PH4
  *     SPI_MOSI = PD6      SPI_MISO = PH5      PENIRQ = PG8（按下为低）
  ******************************************************************************
  */

#ifndef __BSP_TOUCH_H
#define __BSP_TOUCH_H

#include "stm32f4xx.h"

void    Touch_Init(void);

/* 快速判断有没有按下（读 PENIRQ，不发起 SPI 传输）*/
uint8_t Touch_IsPressed(void);

/* 取一次触摸点：有按下返回 1，并把屏幕坐标写到 *x / *y */
uint8_t Touch_GetPoint(uint16_t *x, uint16_t *y);

/* 两点校准：按提示点左上角、右下角各一次。成功返回 1 */
uint8_t Touch_Calibrate(void);

/* 有没有校准过 */
uint8_t Touch_IsCalibrated(void);

/* 调试用：不管有没有按下，都读一次原始值（12 位）*/
void    Touch_ReadRawXY(int32_t *rx, int32_t *ry);

/* 调试用：把原始值按当前校准换算成屏幕坐标（没校准时按"不交换、正方向"粗略换算）*/
void    Touch_RawToScreen(int32_t rx, int32_t ry, uint16_t *x, uint16_t *y);

#endif /* __BSP_TOUCH_H */
