/**
  ******************************************************************************
  * @file    lcd_draw.c
  * @author  embedfire
  * @brief   
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

#include "bsp_lcd_display.h"
#include "nt35510.h"
#include "ili9806g.h"
#include "bsp_ili9341_lcd.h"
#include "lcd_draw.h"

//根据液晶屏幕决定像素宽度
uint16_t LCD_X_LENGTH = 0;
uint16_t LCD_Y_LENGTH = 0;

//液晶屏扫描模式，本变量主要用于方便选择触摸屏的计算参数
//参数可选值为0-7
//调用 芯片_GramScan函数设置方向时会自动更改
//LCD刚初始化完成时会使用本默认值
uint8_t LCD_SCAN_MODE = 6;

/*当前识别到的面板 ID，0 = 面板没回应（排线/座子/型号问题）
  这个值同时用来给板上的 LED 报信，见 bsp_ui.c*/
uint16_t LCD_PANEL_ID = 0;

/*认屏兜底开关：
    0 = 正常流程，自动识别 NT35510 / ILI9806G / ILI9341；
    1 = 跳过识别，直接按 ILI9341（野火 3.2 / 2.8 寸电阻屏）初始化。
  现在设成 1：你这块是 3.2 寸 ILI9341，第一次烧写时认屏白屏，
  直接指定面板更稳。以后换回别的屏（4.3/4.5/5 寸）把这里改回 0 即可。*/
#define LCD_FORCE_ILI9341   1


void (*lcd_gram_scan)(uint8_t ucOption);
void (*lcd_open_window)(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void (*lcd_set_pixel)(uint16_t sx, uint16_t sy,uint16_t color);
void (*lcd_fill_color)(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,uint16_t color);      

uint8_t LCD_Draw_Config(void)
{     
    uint8_t ret = 0;
    
    LCD_Display_FSMC_Init();
    LCD_Display_Pin_Init();
    LCD_Display_Rest();

#if (LCD_FORCE_ILI9341 == 1)
    /*兜底：跳过认屏，按 ILI9341 的时序直接初始化*/
    LCD_Display_FSMC_Init_ILI9341();
    ILI9341_Init();
    LCD_PANEL_ID = ILI9341_GetID();
    lcd_gram_scan = ILI9341_GramScan;
    lcd_open_window = ILI9341_OpenWindow;
    lcd_set_pixel = ILI9341_SetPointPixel_Color;
    lcd_fill_color = ILI9341_Fill;
#else
    if(NT35510_ReadID() == LCDID_NT35510)
    {
        NT35510_Init();
        LCD_PANEL_ID = LCDID_NT35510;
        lcd_gram_scan = NT35510_GramScan;
        lcd_open_window = NT35510_OpenWindow;
        lcd_set_pixel = NT35510_SetPointPixel;
        lcd_fill_color = NT35510_Fill;

    }
    else if(ILI9806G_ReadID() == LCDID_ILI9806G)
    {
        ILI9806G_Init();
        LCD_PANEL_ID = LCDID_ILI9806G;
        lcd_gram_scan = ILI9806G_GramScan;
        lcd_open_window = ILI9806G_OpenWindow;
        lcd_set_pixel = ILI9806G_SetPointPixel;
        lcd_fill_color = ILI9806G_Fill;
    }
    else
    {
        /*野火 3.2 寸（240x320）和 2.8 寸（240x320）电阻触摸屏：ILI9341 面板，
          ID 读回 0x8552 的是 ST7789V 面板，两种面板共用这一套初始化。
          注意：ILI9341 的 FSMC 时序和 NT35510/ILI9806G 不一样
          （官方例程是"模式 B + 数据保持 15 个 HCLK"），所以先换时序再读 ID。*/
        LCD_Display_FSMC_Init_ILI9341();

        if(ILI9341_ReadID() == LCDID_ILI9341)
        {
            ILI9341_Init();
            LCD_PANEL_ID = ILI9341_GetID();
            lcd_gram_scan = ILI9341_GramScan;
            lcd_open_window = ILI9341_OpenWindow;
            lcd_set_pixel = ILI9341_SetPointPixel_Color;
            lcd_fill_color = ILI9341_Fill;
        }
        else
        {
            ret = 1;
            LCD_PANEL_ID = 0;
        }
    }
#endif /* LCD_FORCE_ILI9341 */

    lcd_gram_scan(LCD_SCAN_MODE);
    
    return ret;
}

/**
 * @brief  在 LCD 显示器上使用 Bresenham 算法画线段 
 * @param  usX1 ：在特定扫描方向下线段的一个端点X坐标
 * @param  usY1 ：在特定扫描方向下线段的一个端点Y坐标
 * @param  usX2 ：在特定扫描方向下线段的另一个端点X坐标
 * @param  usY2 ：在特定扫描方向下线段的另一个端点Y坐标
 * @param  color ：设置颜色
 * @retval 无
 */
void LCD_Draw_Line ( uint16_t usX1, uint16_t usY1, uint16_t usX2, uint16_t usY2, uint16_t color )
{
	uint16_t us; 
	uint16_t usX_Current, usY_Current;
	
	int32_t lError_X = 0, lError_Y = 0, lDelta_X, lDelta_Y, lDistance; 
	int32_t lIncrease_X, lIncrease_Y; 	
	
	
	lDelta_X = usX2 - usX1; //计算坐标增量 
	lDelta_Y = usY2 - usY1; 
	
	usX_Current = usX1; 
	usY_Current = usY1; 
	
	
	if ( lDelta_X > 0 ) 
		lIncrease_X = 1; //设置单步方向 
	
	else if ( lDelta_X == 0 ) 
		lIncrease_X = 0;//垂直线 
	
	else 
  { 
    lIncrease_X = -1;
    lDelta_X = - lDelta_X;
  } 

	
	if ( lDelta_Y > 0 )
		lIncrease_Y = 1; 
	
	else if ( lDelta_Y == 0 )
		lIncrease_Y = 0;//水平线 
	
	else 
  {
    lIncrease_Y = -1;
    lDelta_Y = - lDelta_Y;
  } 

	
	if (  lDelta_X > lDelta_Y )
		lDistance = lDelta_X; //选取基本增量坐标轴 
	
	else 
		lDistance = lDelta_Y; 

	
	for ( us = 0; us <= lDistance + 1; us ++ )//画线输出 
	{  
		lcd_set_pixel ( usX_Current, usY_Current, color);//画点 
		
		lError_X += lDelta_X ; 
		lError_Y += lDelta_Y ; 
		
		if ( lError_X > lDistance ) 
		{ 
			lError_X -= lDistance; 
			usX_Current += lIncrease_X; 
		}  
		
		if ( lError_Y > lDistance ) 
		{ 
			lError_Y -= lDistance; 
			usY_Current += lIncrease_Y; 
		} 
		
	}  
}

/**
 * @brief  在 LCD 显示器上显示一个英文字符
 * @param  usX ：在特定扫描方向下字符的起始X坐标
 * @param  usY ：在特定扫描方向下该点的起始Y坐标
 * @param  cChar ：要显示的英文字符
 * @param  TextColor ：字体颜色
 * @param  BackColor ：背景颜色
 * @param  LCD_Currentfonts ：英文字体大小结构体指针
 * @retval 无
 */
void LCD_DispChar_EN ( uint16_t usX, uint16_t usY, const char cChar, uint16_t TextColor, uint16_t BackColor, sFONT *LCD_Currentfonts)
{
	uint8_t  byteCount, bitCount,fontLength;	
	uint16_t ucRelativePositon;
	uint8_t *Pfont;
	
	//对ascii码表偏移（字模表不包含ASCII表的前32个非图形符号）
	ucRelativePositon = cChar - ' ';
	
	//每个字模的字节数
	fontLength = (LCD_Currentfonts->Width*LCD_Currentfonts->Height)/8;
		
	//字模首地址
	/*ascii码表偏移值乘以每个字模的字节数，求出字模的偏移位置*/
	Pfont = (uint8_t *)&LCD_Currentfonts->table[ucRelativePositon * fontLength];
	
	//设置显示窗口
	lcd_open_window ( usX, usY, LCD_Currentfonts->Width, LCD_Currentfonts->Height);
	
	//按字节读取字模数据
	//由于前面直接设置了显示窗口，显示数据会自动换行
	for ( byteCount = 0; byteCount < fontLength; byteCount++ )
	{
			//一位一位处理要显示的颜色
			for ( bitCount = 0; bitCount < 8; bitCount++ )
			{
					if ( Pfont[byteCount] & (0x80>>bitCount) )
						LCD_WRITE_DATA ( TextColor );			
					else
						LCD_WRITE_DATA ( BackColor );
			}	
	}	
}
/**
 * @brief  在 LCD 显示器上显示英文字符串
 * @param  usX ：在特定扫描方向下字符的起始X坐标
 * @param  usY ：在特定扫描方向下字符的起始Y坐标
 * @param  pStr ：要显示的英文字符串的首地址
 * @param  TextColor ：字体颜色
 * @param  BackColor ：背景颜色
 * @param  LCD_Currentfonts ：英文字体大小结构体指针
 * @retval 无
 */
void LCD_DispString_EN ( uint16_t usX ,uint16_t usY, char * pStr, uint16_t TextColor, uint16_t BackColor, sFONT *LCD_Currentfonts)
{
	while ( * pStr != '\0' )
	{
		//屏幕边界检查
        if ( ( usX - 0 + LCD_Currentfonts->Width ) > LCD_X_LENGTH )
		{
			usX = 0;
			usY += LCD_Currentfonts->Height;
		}
		
		if ( ( usY - 0 + LCD_Currentfonts->Height ) > LCD_Y_LENGTH )
		{
			usX = 0;
			usY = 0;
		}
		
		LCD_DispChar_EN ( usX, usY, * pStr, TextColor, BackColor, LCD_Currentfonts);
		
		pStr ++;
		
		usX += LCD_Currentfonts->Width;
		
	}
	
}

/**
 * @brief  通用点阵字符显示函数
 * @param  start_x ：起始X坐标
 * @param  start_y ：起始Y坐标
 * @param  index ：字符在字模数组中的索引
 * @param  TextColor ：字体颜色
 * @param  BackColor ：背景颜色
 * @param  font ：自定义点阵字模描述结构体指针
 * @note  可通过该函数显示自定义的中文字体
 */
void LCD_DispChar_Bitmap(uint16_t start_x, uint16_t start_y, 
                              uint16_t index, uint16_t TextColor, 
                              uint16_t BackColor, FontBitmap_t *font)
{
    // 计算每个字符占用的总字节数
    uint32_t bytes_per_char = font->height * font->bytes_per_row;
    
    // 定位到指定字符的数据起始位置
    const uint8_t *pCharData = &font->table[index * bytes_per_char];
    
    // 遍历所有行
    for (uint8_t row = 0; row < font->height; row++)
    {
        // 获取当前行的数据
        uint32_t rowData = 0;
        const uint8_t *pRowData = &pCharData[row * font->bytes_per_row];
        
        // 将多个字节组合成一个行数据（假设高位在前）
        for (uint8_t b = 0; b < font->bytes_per_row; b++)
        {
            rowData = (rowData << 8) | pRowData[b];
        }
        
        // 调整行数据的位对齐（向右对齐到宽度）
        rowData >>= (font->bytes_per_row * 8 - font->width);
        
        // 遍历该行的所有像素（列）
        for (uint8_t col = 0; col < font->width; col++)
        {
            // 根据扫描方向获取像素位
            uint8_t pixelBit;
            if (font->scan_direction == 0)  // 逐行顺向（MSB对应最左像素）
            {
                pixelBit = (rowData >> (font->width - 1 - col)) & 0x01;
            }
            else if (font->scan_direction == 1)  // 逐行逆向（LSB对应最左像素）
            {
                pixelBit = (rowData >> col) & 0x01;
            }
            else  // 默认使用顺向
            {
                pixelBit = (rowData >> (font->width - 1 - col)) & 0x01;
            }
            
            // 阴码/阳码转换
            if (!font->is_yin_code)  // 如果是阳码，取反
            {
                pixelBit = !pixelBit;
            }
            
            // 计算像素坐标并绘制
            uint16_t pixelX = start_x + col;
            uint16_t pixelY = start_y + row;
            
            uint16_t color = pixelBit ? TextColor : BackColor;
            lcd_set_pixel(pixelX, pixelY, color);
        }
    }
}

/**
 * @brief  连续显示字符串（根据索引数组）
 * @param  start_x ：起始X坐标
 * @param  start_y ：起始Y坐标
 * @param  indices ：字符索引数组
 * @param  count ：要显示的字符数量
 * @param  TextColor ：字体颜色
 * @param  BackColor ：背景颜色
 * @param  font ：自定义点阵字模描述结构体指针
 * @note  可通过该函数显示自定义的中文字体
 */
void LCD_DispString_Bitmap(uint16_t start_x, uint16_t start_y, 
                              const uint16_t *indices, uint8_t count,
                              uint16_t TextColor, uint16_t BackColor, 
                              FontBitmap_t *font)
{
    
    for (uint8_t i = 0; i < count; i++)
    {
		//屏幕边界检查
        if ( ( start_x - 0 + font->width ) > LCD_X_LENGTH )
		{
			start_x = 0;
			start_y += font->height;
		}
		
		if ( ( start_y - 0 + font->height ) > LCD_Y_LENGTH )
		{
			start_x = 0;
			start_y = 0;
		}

        LCD_DispChar_Bitmap(start_x, start_y, indices[i], 
                                TextColor, BackColor, font);
        
        // 移动到下一个字符位置
        start_x += font->width;
    }
 
}

/*********************************************END OF FILE**********************/
