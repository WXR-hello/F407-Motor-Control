/**
  ******************************************************************************
  * @file    ili9806g.c
  * @author  embedfire
  * @brief   
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

#include "bsp_lcd_display.h"
#include "ili9806g.h"
#include "lcd_draw.h"  

//根据液晶扫描方向而变化的XY像素宽度
//调用ILI9806G_GramScan函数设置方向时会自动更改
uint16_t ILI9806G_X_LENGTH = ILI9806G_LESS_PIXEL;
uint16_t ILI9806G_Y_LENGTH = ILI9806G_MORE_PIXEL;

void ILI9806G_REG_Config (void)
{	
	//************* Start Initial Sequence **********//
	LCD_WRITE_CMD(0xFF); // EXTC Command Set enable register 
	LCD_WRITE_DATA(0xFF); 
	LCD_WRITE_DATA(0x98); 
	LCD_WRITE_DATA(0x06); 

	LCD_WRITE_CMD(0xBA); // SPI Interface Setting 
	LCD_WRITE_DATA(0x60); 

	LCD_WRITE_CMD(0xBC); // GIP 1 
	LCD_WRITE_DATA(0x01); 
	LCD_WRITE_DATA(0x0E); 
	LCD_WRITE_DATA(0x61); 
	LCD_WRITE_DATA(0xFB); 
	LCD_WRITE_DATA(0x10); 
	LCD_WRITE_DATA(0x10); 
	LCD_WRITE_DATA(0x0B); 
	LCD_WRITE_DATA(0x0F); 
	LCD_WRITE_DATA(0x2E); 
	LCD_WRITE_DATA(0x73); 
	LCD_WRITE_DATA(0xFF); 
	LCD_WRITE_DATA(0xFF); 
	LCD_WRITE_DATA(0x0E); 
	LCD_WRITE_DATA(0x0E); 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x03); 
	LCD_WRITE_DATA(0x66); 
	LCD_WRITE_DATA(0x63); 
	LCD_WRITE_DATA(0x01); 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x00); 

	LCD_WRITE_CMD(0xBD); // GIP 2 
	LCD_WRITE_DATA(0x01); 
	LCD_WRITE_DATA(0x23); 
	LCD_WRITE_DATA(0x45); 
	LCD_WRITE_DATA(0x67); 
	LCD_WRITE_DATA(0x01); 
	LCD_WRITE_DATA(0x23); 
	LCD_WRITE_DATA(0x45); 
	LCD_WRITE_DATA(0x67); 

	LCD_WRITE_CMD(0xBE); // GIP 3 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x21); 
	LCD_WRITE_DATA(0xAB); 
	LCD_WRITE_DATA(0x60); 
	LCD_WRITE_DATA(0x22); 
	LCD_WRITE_DATA(0x22); 
	LCD_WRITE_DATA(0x22); 
	LCD_WRITE_DATA(0x22); 
	LCD_WRITE_DATA(0x22); 

	LCD_WRITE_CMD(0xC7); // Vcom 
	LCD_WRITE_DATA(0x6F); 

	LCD_WRITE_CMD(0xED); // EN_volt_reg 
	LCD_WRITE_DATA(0x7F); 
	LCD_WRITE_DATA(0x0F); 
	LCD_WRITE_DATA(0x00); 

	LCD_WRITE_CMD(0xC0); // Power Control 1
	LCD_WRITE_DATA(0x37); 
	LCD_WRITE_DATA(0x0B); 
	LCD_WRITE_DATA(0x0A); 

	LCD_WRITE_CMD(0xFC); // LVGL 
	LCD_WRITE_DATA(0x0A); 

	LCD_WRITE_CMD(0xDF); // Engineering Setting 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x20); 

	LCD_WRITE_CMD(0xF3); // DVDD Voltage Setting 
	LCD_WRITE_DATA(0x74); 

	LCD_WRITE_CMD(0xB4); // Display Inversion Control 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x00); 

	LCD_WRITE_CMD(0xF7); // 480x800
	LCD_WRITE_DATA(0x8A); 

	LCD_WRITE_CMD(0xB1); // Frame Rate 
	LCD_WRITE_DATA(0x00); 
	LCD_WRITE_DATA(0x12); 
	LCD_WRITE_DATA(0x13); 

	LCD_WRITE_CMD(0xF2); //Panel Timing Control 
	LCD_WRITE_DATA(0x80); 
	LCD_WRITE_DATA(0x5B); 
	LCD_WRITE_DATA(0x40); 
	LCD_WRITE_DATA(0x28); 

	LCD_WRITE_CMD(0xC1); // Power Control 2 
	LCD_WRITE_DATA(0x17); 
	LCD_WRITE_DATA(0x7D); 
	LCD_WRITE_DATA(0x7A); 
	LCD_WRITE_DATA(0x20); 

	LCD_WRITE_CMD(0xE0); 
	LCD_WRITE_DATA(0x00); //P1 
	LCD_WRITE_DATA(0x11); //P2 
	LCD_WRITE_DATA(0x1C); //P3 
	LCD_WRITE_DATA(0x0E); //P4 
	LCD_WRITE_DATA(0x0F); //P5 
	LCD_WRITE_DATA(0x0C); //P6 
	LCD_WRITE_DATA(0xC7); //P7 
	LCD_WRITE_DATA(0x06); //P8 
	LCD_WRITE_DATA(0x06); //P9 
	LCD_WRITE_DATA(0x0A); //P10 
	LCD_WRITE_DATA(0x10); //P11 
	LCD_WRITE_DATA(0x12); //P12 
	LCD_WRITE_DATA(0x0A); //P13 
	LCD_WRITE_DATA(0x10); //P14 
	LCD_WRITE_DATA(0x02); //P15 
	LCD_WRITE_DATA(0x00); //P16 

	LCD_WRITE_CMD(0xE1); 
	LCD_WRITE_DATA(0x00); //P1 
	LCD_WRITE_DATA(0x12); //P2 
	LCD_WRITE_DATA(0x18); //P3 
	LCD_WRITE_DATA(0x0C); //P4 
	LCD_WRITE_DATA(0x0F); //P5 
	LCD_WRITE_DATA(0x0A); //P6 
	LCD_WRITE_DATA(0x77); //P7 
	LCD_WRITE_DATA(0x06); //P8 
	LCD_WRITE_DATA(0x07); //P9 
	LCD_WRITE_DATA(0x0A); //P10 
	LCD_WRITE_DATA(0x0E); //P11 
	LCD_WRITE_DATA(0x0B); //P12 
	LCD_WRITE_DATA(0x10); //P13 
	LCD_WRITE_DATA(0x1D); //P14 
	LCD_WRITE_DATA(0x17); //P15 
	LCD_WRITE_DATA(0x00); //P16 

	LCD_WRITE_CMD(0x35); //Tearing Effect ON 
	LCD_WRITE_DATA(0x00); 

	LCD_WRITE_CMD(0x3A);
	LCD_WRITE_DATA(0x55);


	LCD_WRITE_CMD(0x11); //Exit Sleep 
	HAL_Delay (200);
	LCD_WRITE_CMD(0x29); // Display On 

}


/**
 * @brief  在显示器上开辟一个窗口，并准备填充像素
 * @param  usX ：在特定扫描方向下窗口的起点X坐标
 * @param  usY ：在特定扫描方向下窗口的起点Y坐标
 * @param  usWidth ：窗口的宽度
 * @param  usHeight ：窗口的高度
 * @retval 无
 */
void ILI9806G_OpenWindow ( uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight )
{
	LCD_WRITE_CMD ( ILI9806G_CMD_SetCoordinateX ); 				 /* 设置X坐标 */
	LCD_WRITE_DATA ( usX >> 8  );	 /* 先高8位，然后低8位 */
	LCD_WRITE_DATA ( usX & 0xff  );	 /* 设置起始点和结束点*/
	LCD_WRITE_DATA ( ( usX + usWidth - 1 ) >> 8  );
	LCD_WRITE_DATA ( ( usX + usWidth - 1 ) & 0xff  );

	LCD_WRITE_CMD ( ILI9806G_CMD_SetCoordinateY ); 			     /* 设置Y坐标*/
	LCD_WRITE_DATA ( usY >> 8  );
	LCD_WRITE_DATA ( usY & 0xff  );
	LCD_WRITE_DATA ( ( usY + usHeight - 1 ) >> 8 );
	LCD_WRITE_DATA ( ( usY + usHeight - 1) & 0xff );

    LCD_WRITE_CMD ( ILI9806G_CMD_SetPixel );
}



void ILI9806G_Fill( uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight,uint16_t color )
{
	ILI9806G_OpenWindow ( usX, usY, usWidth, usHeight );   
    
    int temp =  usWidth * usHeight;
    		
	for (int  i = 0; i < temp; i ++ )
		LCD_WRITE_DATA ( color );	
	
}

uint32_t ILI9806G_ReadID(void)
{
	uint16_t ID[3];
    uint32_t id = 0;
	
    LCD_WRITE_CMD(0xD3);
    LCD_READ_DATA();
    ID[0] = LCD_READ_DATA();
    ID[1] = LCD_READ_DATA();
    ID[2] = LCD_READ_DATA();
    id =  (ID[0]& 0xff) << 16 | (ID[1]& 0xff) << 8 | (ID[2]& 0xff);
	
    if(id == LCDID_ILI9806G)
    {
        return id;
    }
    else
    {
        return 0;
    }
}

uint8_t ILI9806G_Init(void)
{

    ILI9806G_REG_Config();
    LCD_Display_BL_ENABLE(1);
    return 0;
}

/**
 * @brief  对显示器的某一点以某种颜色进行填充
 * @param  usX ：在特定扫描方向下该点的X坐标
 * @param  usY ：在特定扫描方向下该点的Y坐标
 * @param  color ：设置颜色
 * @retval 无
 */
void ILI9806G_SetPointPixel ( uint16_t usX, uint16_t usY, uint16_t color )	
{	
	if ( ( usX < ILI9806G_X_LENGTH ) && ( usY < ILI9806G_Y_LENGTH ) )
  {
		ILI9806G_OpenWindow ( usX, usY, 1, 1 );
	
        LCD_WRITE_DATA ( color );
	}
	
}

/**
 * @brief  设置显示器的GRAM的扫描方向 
 * @param  ucOption ：选择GRAM的扫描方向 
 *     @arg 0-7 :参数可选值为0-7这八个方向
 *
 *	！！！其中0、3、5、6 模式适合从左至右显示文字，
 *				不推荐使用其它模式显示文字	其它模式显示文字会有镜像效果			
 *		
 *	其中0、2、4、6 模式的X方向像素为480，Y方向像素为800
 *	其中1、3、5、7 模式下X方向像素为800，Y方向像素为480
 *
 *	其中 6 模式为大部分液晶例程的默认显示方向
 *	其中 3 模式为摄像头例程使用的方向
 *	其中 0 模式为BMP图片显示例程使用的方向
 *
 * @retval 无
 * @note  坐标图例：A表示向上，V表示向下，<表示向左，>表示向右
					X表示X轴，Y表示Y轴

							LCDID_ILI9806G
----------------------------------------------------------------------------
模式0：			.		模式1：		.	模式2：			.	模式3：					
			A	.			A		.		A			.		A									
			|	.			|		.		|			.		|							
			Y	.			X		.		Y			.		X					
			0	.			1		.		2			.		3					
	<--- X0 o	.	<----Y1	o		.		o 2X--->    .		o 3Y--->	
----------------------------------------------------------------------------	
模式4：			.	模式5：			.	模式6：			.	模式7：					
	<--- X4 o	.	<--- Y5 o		.		o 6X--->    .		o 7Y--->	
			4	.			5		.		6			.		7	
			Y	.			X		.		Y			.		X						
			|	.			|		.		|			.		|							
			V	.			V		.		V			.		V		
----------------------------------------------------------------------------				
						     LCD屏示例
						|-----------------|
						|	  野火Logo	  |
						|				  |
						|				  |
						|				  |
						|				  |
						|				  |
						|				  |
						|				  |
						|				  |
						|-----------------|
						屏幕正面（宽480，高800）
																			
 *******************************************************/
void ILI9806G_GramScan ( uint8_t ucOption )
{	
	//参数检查，只可输入0-7
	if(ucOption >7 )
		return;
	
	//根据模式更新LCD_SCAN_MODE的值，主要用于触摸屏选择计算参数
	LCD_SCAN_MODE = ucOption;
	
	//根据模式更新XY方向的像素宽度
	if(ucOption%2 == 0)	
	{
		//0 2 4 6模式下X方向像素宽度为480，Y方向为800
		ILI9806G_X_LENGTH = ILI9806G_LESS_PIXEL;
		ILI9806G_Y_LENGTH =	ILI9806G_MORE_PIXEL;
	}
	else				
	{
		//1 3 5 7模式下X方向像素宽度为800，Y方向为480
		ILI9806G_X_LENGTH = ILI9806G_MORE_PIXEL;
		ILI9806G_Y_LENGTH =	ILI9806G_LESS_PIXEL; 
	}

	//0x36命令参数的高3位可用于设置GRAM扫描方向	
	LCD_WRITE_CMD ( 0x36 ); 

    LCD_WRITE_DATA ( 0x00 |(ucOption<<5));//根据ucOption的值设置LCD参数，共0-7种模式

	LCD_WRITE_CMD ( ILI9806G_CMD_SetCoordinateX ); 
	LCD_WRITE_DATA ( 0x00 );		/* x 起始坐标高8位 */
	LCD_WRITE_DATA ( 0x00 );		/* x 起始坐标低8位 */
	LCD_WRITE_DATA ( ((ILI9806G_X_LENGTH-1)>>8)&0xFF ); /* x 结束坐标高8位 */	
	LCD_WRITE_DATA ( (ILI9806G_X_LENGTH-1)&0xFF );				/* x 结束坐标低8位 */

	LCD_WRITE_CMD ( ILI9806G_CMD_SetCoordinateY ); 
	LCD_WRITE_DATA ( 0x00 );		/* y 起始坐标高8位 */
	LCD_WRITE_DATA ( 0x00 );		/* y 起始坐标低8位 */
	LCD_WRITE_DATA ( ((ILI9806G_Y_LENGTH-1)>>8)&0xFF );	/* y 结束坐标高8位 */	 
	LCD_WRITE_DATA ( (ILI9806G_Y_LENGTH-1)&0xFF );				/* y 结束坐标低8位 */

	/* write gram start */
	LCD_WRITE_CMD ( ILI9806G_CMD_SetPixel );	
    
    //根据模式更新LCD X Y 的值
    LCD_X_LENGTH = ILI9806G_X_LENGTH;
    LCD_Y_LENGTH = ILI9806G_Y_LENGTH;
}

/*********************************************END OF FILE**********************/
