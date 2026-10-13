#ifndef __ILI9806G_H
#define	__ILI9806G_H

#include "stdint.h"

/* 定义 LCD 驱动芯片 ID */
#define     LCDID_ILI9806G        0x9806

#define 	ILI9806G_LESS_PIXEL	  	  480			//液晶屏较短方向的像素宽度
#define 	ILI9806G_MORE_PIXEL	 	  800			//液晶屏较长方向的像素宽度

#define     ILI9806G_CMD_SetCoordinateX		 0x2A	     //设置X坐标
#define     ILI9806G_CMD_SetCoordinateY		 0x2B	     //设置Y坐标
#define     ILI9806G_CMD_SetPixel		 	 0x2C	     //填充像素

extern uint16_t ILI9806G_X_LENGTH;
extern uint16_t ILI9806G_Y_LENGTH;


uint8_t     ILI9806G_Init(void);
void        ILI9806G_REG_Config (void);
void        ILI9806G_OpenWindow ( uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight );
void        ILI9806G_Fill ( uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight,uint16_t color );
uint32_t    ILI9806G_ReadID(void); 
void        ILI9806G_SetPointPixel ( uint16_t usX, uint16_t usY, uint16_t color );
void        ILI9806G_GramScan ( uint8_t ucOption );

#endif /* __ILI9806G_H */
