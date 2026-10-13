/**
  ******************************************************************************
  * @file    bsp_ili9341_lcd.h
  * @brief   ILI9341 / ST7789V（野火 3.2 寸、2.8 寸电阻触摸屏）驱动
  *
  *  FSMC 8080 接口、16 位数据，地址和 NT35510/ILI9806G 用同一套：
  *     NE3(Bank3) = 片选 CS，A0 = 寄存器/数据选择 RS
  *  引脚、复位(PF11)、背光(PE5) 都由 bsp_lcd_display.c 配好，本文件只管面板。
  ******************************************************************************
  */

#ifndef __BSP_ILI9341_LCD_H
#define __BSP_ILI9341_LCD_H

#include "stm32f4xx.h"
#include "lcd_draw.h"
#include "bsp_lcd_display.h"

/* FSMC 命令/数据地址 */
#define FSMC_Addr_ILI9341_CMD               ( ( uint32_t ) 0x68000000 )
#define FSMC_Addr_ILI9341_DATA              ( ( uint32_t ) 0x68000002 )

/* 屏幕像素尺寸（较短/较长方向）*/
#define ILI9341_LESS_PIXEL                  240
#define ILI9341_MORE_PIXEL                  320

/* 常用命令 */
#define CMD_SetCoordinateX                  0x2A
#define CMD_SetCoordinateY                  0x2B
#define CMD_SetPixel                        0x2C

/* 面板 ID */
#define LCDID_ILI9341                       0x9341
#define LCDID_ST7789V                       0x8552

#define DEBUG_DELAY()

/*强制面板型号（本工程认屏读不到 ID 时用）：
    0             = 先读 ID，读不到按 ILI9341 走
    LCDID_ILI9341 = 直接按 ILI9341 初始化（野火 2.8 寸；3.2 寸老版本已停产）
    LCDID_ST7789V = 直接按 ST7789V 初始化（野火 3.2 寸现产版本）
  野火官方文档：3.2 寸现产用 ST7789V（ILI9341 停产），2.8 寸用 ILI9341；
  两个模块接线完全一样，只有面板 IC 不同，所以按屏的寸数选这里。*/
#define ILI9341_FORCE_ID                   LCDID_ST7789V

/*背光极性：野火 3.2 / 2.8 寸模块是"低电平点亮"，
            1 = 低电平点亮（默认，3.2/2.8 寸都用这个）
            0 = 高电平点亮（骄阳自带 4.3/5 寸那种）
  现象对照：如果烧完屏幕"先白后黑"，就是这个极性反了。*/
#define ILI9341_BL_ACTIVE_LOW              1

/*总线方式：
    0 = 用硬件 FSMC（正常、快）
    1 = 用 GPIO 软件模拟 8080 总线（对屏来说还是 8080 模式、同一批引脚，
        只是 WR/CS 由 CPU 手动翻转，用来排除"FSMC 外设没配好"这个可能）
  现在设回 0（用硬件 FSMC，正常速度）。*/
#define ILI9341_USE_BITBANG                0

void     ILI9341_Init(void);
void     ILI9341_InitAs(uint16_t which);
uint16_t ILI9341_ReadID(void);
uint16_t ILI9341_GetID(void);
void     ILI9341_Rst(void);
void     ILI9341_GramScan(uint8_t ucOption);
void     ILI9341_OpenWindow(uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight);
void     ILI9341_SetPointPixel_Color(uint16_t usX, uint16_t usY, uint16_t usColor);
void     ILI9341_Fill(uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight, uint16_t usColor);

#endif /* __BSP_ILI9341_LCD_H */
