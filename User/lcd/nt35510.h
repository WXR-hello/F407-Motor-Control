#ifndef __NT35510_H
#define	__NT35510_H

#include "stdint.h"

/* 定义 LCD 驱动芯片 ID */
#define      LCDID_NT35510        0x008000

#define 	 NT35510_LESS_PIXEL	  	  480			//液晶屏较短方向的像素宽度
#define 	 NT35510_MORE_PIXEL	 	  800			//液晶屏较长方向的像素宽度

#define      NT35510_CMD_SetCoordinateX		0x2A00	     //设置X坐标
#define      NT35510_CMD_SetCoordinateY		0x2B00	     //设置Y坐标
#define      NT35510_CMD_SetPixel		 	0x2C00	     //填充像素

extern uint16_t NT35510_X_LENGTH;
extern uint16_t NT35510_Y_LENGTH;

uint8_t     NT35510_Init(void);
void        NT35510_REG_Config (void);
void        NT35510_OpenWindow ( uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight );
void        NT35510_Fill ( uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight,uint16_t color );
uint32_t    NT35510_ReadID(void);
void        NT35510_SetPointPixel ( uint16_t usX, uint16_t usY, uint16_t color );
void        NT35510_GramScan ( uint8_t ucOption );

#endif /* __NT35510_H */
