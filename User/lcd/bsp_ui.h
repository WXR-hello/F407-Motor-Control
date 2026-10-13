/**
  ******************************************************************************
  * @file    bsp_ui.h
  * @brief   屏幕显示：把"要移动的距离"等状态画到 LCD 上
  *
  *  屏幕驱动来自野火 F407 骄阳官方 LCD 例程（FSMC 8080 + 自动识别面板）：
  *    支持 NT35510（4.3/4.5 寸电容屏）和 ILI9806G（5 寸电容屏）
  ******************************************************************************
  */

#ifndef __BSP_UI_H
#define __BSP_UI_H

#include "stm32f4xx.h"

/* 屏幕按钮对应的动作（和物理按键一一对应）*/
#define UI_ACT_NONE     0
#define UI_ACT_UNIT     1   /* = KEY3：切换单位 */
#define UI_ACT_MINUS    2   /* = KEY4：按当前单位 - */
#define UI_ACT_PLUS     3   /* = KEY5：按当前单位 + */
#define UI_ACT_MODE     4   /* = KEY2：切换模式 */
#define UI_ACT_RUN      5   /* = KEY1：走一次 */

/* 触摸调试屏：1 = 上电只显示触摸原始数据（排查触摸用），0 = 正常界面 */
#define UI_TOUCH_DEBUG  0

/**
  * @brief  初始化屏幕并铺好静态界面
  * @retval 0 = 成功；1 = 没认出屏幕（LCDID_UNKNOWN）
  */
uint8_t UI_Init(void);

/* 把整个界面重画一遍（校准时界面被盖掉后调用）*/
void    UI_RedrawAll(void);

/* 触摸点命中按钮时返回对应的动作（UI_ACT_xxx），没命中返回 UI_ACT_NONE */
uint8_t UI_TouchHandle(uint16_t x, uint16_t y);

/* 触摸调试屏：显示 INT 电平、原始值、换算后的坐标 */
void UI_ShowTouchDebug(uint8_t intLevel, int32_t rx, int32_t ry,
                       uint16_t x, uint16_t y, uint8_t calOk);

/**
  * @brief  刷新动态内容（距离、模式、状态、脉冲数）
  * @note   主循环里反复调用即可，内部只在内容变化时才重画
  * @param  mode      当前模式：0 = 电机1、2；1 = 电机3、4
  * @param  dist_mm   当前模式下"要移动的距离"，单位 mm（可为负）
  * @param  running   该轴是否正在运动
  * @param  pulse     该轴上一次实际发出的脉冲数
  * @param  unit_idx  当前单位：0 = 10cm，1 = 1cm，2 = 1mm（KEY3 切换）
  */
void UI_Update(uint8_t mode, int32_t dist_mm, uint8_t running, uint32_t pulse, uint8_t unit_idx);

#endif /* __BSP_UI_H */
