/**
  ******************************************************************************
  * @file    bsp_ili9341_lcd.c
  * @brief   ILI9341 / ST7789V（野火 3.2 寸、2.8 寸电阻触摸屏）驱动，HAL 版
  *
  *  取自野火 3.2/2.8 寸屏模块资料的官方例程，改动见文件头的说明：
  *    - 去掉 StdPeriph 的 GPIO/FSMC/背光/复位代码，复用本工程的 bsp_lcd_display.c
  *    - 去掉自带的文字/画线函数（本工程用 lcd_draw.c 那一套）
  *    - 填充/画点改成带颜色参数的版本
  ******************************************************************************
  */

#include "bsp_ili9341_lcd.h"

#if (ILI9341_USE_BITBANG == 1)
/* 软件模拟 8080 总线用的引脚（和 FSMC 用的是同一批脚）*/
typedef struct { GPIO_TypeDef *port; uint16_t pin; } bbpin_t;
static const bbpin_t s_bb_d[16] = {
	{GPIOD, GPIO_PIN_14}, {GPIOD, GPIO_PIN_15}, {GPIOD, GPIO_PIN_0},  {GPIOD, GPIO_PIN_1},
	{GPIOE, GPIO_PIN_7},  {GPIOE, GPIO_PIN_8},  {GPIOE, GPIO_PIN_9},  {GPIOE, GPIO_PIN_10},
	{GPIOE, GPIO_PIN_11}, {GPIOE, GPIO_PIN_12}, {GPIOE, GPIO_PIN_13}, {GPIOE, GPIO_PIN_14},
	{GPIOE, GPIO_PIN_15}, {GPIOD, GPIO_PIN_8},  {GPIOD, GPIO_PIN_9},  {GPIOD, GPIO_PIN_10}
};
#define BB_RS_PORT   GPIOF
#define BB_RS_PIN    GPIO_PIN_0
#define BB_CS_PORT   GPIOG
#define BB_CS_PIN    GPIO_PIN_10
#define BB_WR_PORT   GPIOD
#define BB_WR_PIN    GPIO_PIN_5
#define BB_RD_PORT   GPIOD
#define BB_RD_PIN    GPIO_PIN_4
void bb_bus_init(void);
void BB_Write(uint16_t v, uint8_t rs);
#endif

/* 当前面板 ID（REG_Config / GramScan 里要用）*/
static uint16_t s_lcdid = 0;
/* 面板的真实应答（0 = 读 ID 时面板没有任何回应）*/
static uint16_t s_rawid = 0;
/* 运行时指定的型号（0 = 按编译时的 ILI9341_FORCE_ID 或读回的 ID 走）*/
static uint16_t s_forceid = ILI9341_FORCE_ID;

/* 函数声明 */
void ILI9341_Write_Cmd(uint16_t usCmd);
void ILI9341_Write_Data(uint16_t usData);
uint16_t ILI9341_Read_Data(void);
static void ILI9341_Delay(__IO uint32_t nCount);
static void ILI9341_SetCursor(uint16_t usX, uint16_t usY);
static __inline void ILI9341_FillColor(uint32_t ulAmout_Point, uint16_t usColor);

__inline void ILI9341_Write_Cmd ( uint16_t usCmd )
{
#if (ILI9341_USE_BITBANG == 1)
	BB_Write(usCmd, 0U);
#else
	* ( __IO uint16_t * ) ( FSMC_Addr_ILI9341_CMD ) = usCmd;
#endif
	
}

__inline void ILI9341_Write_Data ( uint16_t usData )
{
#if (ILI9341_USE_BITBANG == 1)
	BB_Write(usData, 1U);
#else
	* ( __IO uint16_t * ) ( FSMC_Addr_ILI9341_DATA ) = usData;
#endif
	
}

__inline uint16_t ILI9341_Read_Data ( void )
{
	return ( * ( __IO uint16_t * ) ( FSMC_Addr_ILI9341_DATA ) );
	
}

static void ILI9341_Delay ( __IO uint32_t nCount )
{
  for ( ; nCount != 0; nCount -- );
	
}

static void ILI9341_REG_Config ( void )
{
  /*先把面板的真实应答记下来（0 = 面板一点没回应，多半是排线/座子/型号问题）*/
  s_rawid = ILI9341_ReadID();
  if (s_forceid != 0U)
  {
    /*运行时指定了型号（盲测用）：直接按它走*/
    s_lcdid = s_forceid;
  }
  else if (s_rawid != 0U)
  {
    /*面板有应答：按应答的型号走*/
    s_lcdid = s_rawid;
  }
  else
  {
    /*读不到 ID：按 ILI9341 处理，保证初始化序列照发，避免白屏*/
    s_lcdid = LCDID_ILI9341;
  }
  
  if(s_lcdid == LCDID_ILI9341)
  {
    /*  Power control B (CFh)  */
    DEBUG_DELAY  ();
    ILI9341_Write_Cmd ( 0xCF  );
    ILI9341_Write_Data ( 0x00  );
    ILI9341_Write_Data ( 0x81  );
    ILI9341_Write_Data ( 0x30  );
    
    /*  Power on sequence control (EDh) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xED );
    ILI9341_Write_Data ( 0x64 );
    ILI9341_Write_Data ( 0x03 );
    ILI9341_Write_Data ( 0x12 );
    ILI9341_Write_Data ( 0x81 );
    
    /*  Driver timing control A (E8h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xE8 );
    ILI9341_Write_Data ( 0x85 );
    ILI9341_Write_Data ( 0x10 );
    ILI9341_Write_Data ( 0x78 );
    
    /*  Power control A (CBh) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xCB );
    ILI9341_Write_Data ( 0x39 );
    ILI9341_Write_Data ( 0x2C );
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x34 );
    ILI9341_Write_Data ( 0x02 );
    
    /* Pump ratio control (F7h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xF7 );
    ILI9341_Write_Data ( 0x20 );
    
    /* Driver timing control B */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xEA );
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x00 );
    
    /* Frame Rate Control (In Normal Mode/Full Colors) (B1h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xB1 );
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x1B );
    
    /*  Display Function Control (B6h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xB6 );
    ILI9341_Write_Data ( 0x0A );
    ILI9341_Write_Data ( 0xA2 );
    
    /* Power Control 1 (C0h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xC0 );
    ILI9341_Write_Data ( 0x35 );
    
    /* Power Control 2 (C1h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xC1 );
    ILI9341_Write_Data ( 0x11 );
    
    /* VCOM Control 1 (C5h) */
    ILI9341_Write_Cmd ( 0xC5 );
    ILI9341_Write_Data ( 0x45 );
    ILI9341_Write_Data ( 0x45 );
    
    /*  VCOM Control 2 (C7h)  */
    ILI9341_Write_Cmd ( 0xC7 );
    ILI9341_Write_Data ( 0xA2 );
    
    /* Enable 3G (F2h) */
    ILI9341_Write_Cmd ( 0xF2 );
    ILI9341_Write_Data ( 0x00 );
    
    /* Gamma Set (26h) */
    ILI9341_Write_Cmd ( 0x26 );
    ILI9341_Write_Data ( 0x01 );
    DEBUG_DELAY ();
    
    /* Positive Gamma Correction */
    ILI9341_Write_Cmd ( 0xE0 ); //Set Gamma
    ILI9341_Write_Data ( 0x0F );
    ILI9341_Write_Data ( 0x26 );
    ILI9341_Write_Data ( 0x24 );
    ILI9341_Write_Data ( 0x0B );
    ILI9341_Write_Data ( 0x0E );
    ILI9341_Write_Data ( 0x09 );
    ILI9341_Write_Data ( 0x54 );
    ILI9341_Write_Data ( 0xA8 );
    ILI9341_Write_Data ( 0x46 );
    ILI9341_Write_Data ( 0x0C );
    ILI9341_Write_Data ( 0x17 );
    ILI9341_Write_Data ( 0x09 );
    ILI9341_Write_Data ( 0x0F );
    ILI9341_Write_Data ( 0x07 );
    ILI9341_Write_Data ( 0x00 );
    
    /* Negative Gamma Correction (E1h) */
    ILI9341_Write_Cmd ( 0XE1 ); //Set Gamma
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x19 );
    ILI9341_Write_Data ( 0x1B );
    ILI9341_Write_Data ( 0x04 );
    ILI9341_Write_Data ( 0x10 );
    ILI9341_Write_Data ( 0x07 );
    ILI9341_Write_Data ( 0x2A );
    ILI9341_Write_Data ( 0x47 );
    ILI9341_Write_Data ( 0x39 );
    ILI9341_Write_Data ( 0x03 );
    ILI9341_Write_Data ( 0x06 );
    ILI9341_Write_Data ( 0x06 );
    ILI9341_Write_Data ( 0x30 );
    ILI9341_Write_Data ( 0x38 );
    ILI9341_Write_Data ( 0x0F );
    
    /* memory access control set */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0x36 ); 	
    ILI9341_Write_Data ( 0xC8 );    /*竖屏  左上角到 (起点)到右下角 (终点)扫描方式*/
    DEBUG_DELAY ();
    
    /* column address control set */
    ILI9341_Write_Cmd ( CMD_SetCoordinateX ); 
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0xEF );
    
    /* page address control set */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( CMD_SetCoordinateY ); 
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x01 );
    ILI9341_Write_Data ( 0x3F );
    
    /*  Pixel Format Set (3Ah)  */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0x3a ); 
    ILI9341_Write_Data ( 0x55 );
    
    /* Sleep Out (11h)  */
    ILI9341_Write_Cmd ( 0x11 );	
    ILI9341_Delay ( 0xAFFf<<2 );
    DEBUG_DELAY ();
    
    /* Display ON (29h) */
    ILI9341_Write_Cmd ( 0x29 ); 
	}
  
  else if(s_lcdid == LCDID_ST7789V)
  {
    /*  Power control B (CFh)  */
    DEBUG_DELAY  ();
    ILI9341_Write_Cmd ( 0xCF  );
    ILI9341_Write_Data ( 0x00  );
    ILI9341_Write_Data ( 0xC1  );
    ILI9341_Write_Data ( 0x30  );
    
    /*  Power on sequence control (EDh) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xED );
    ILI9341_Write_Data ( 0x64 );
    ILI9341_Write_Data ( 0x03 );
    ILI9341_Write_Data ( 0x12 );
    ILI9341_Write_Data ( 0x81 );
    
    /*  Driver timing control A (E8h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xE8 );
    ILI9341_Write_Data ( 0x85 );
    ILI9341_Write_Data ( 0x10 );
    ILI9341_Write_Data ( 0x78 );
    
    /*  Power control A (CBh) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xCB );
    ILI9341_Write_Data ( 0x39 );
    ILI9341_Write_Data ( 0x2C );
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x34 );
    ILI9341_Write_Data ( 0x02 );
    
    /* Pump ratio control (F7h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xF7 );
    ILI9341_Write_Data ( 0x20 );
    
    /* Driver timing control B */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xEA );
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x00 );
    
    
    /* Power Control 1 (C0h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xC0 );   //Power control
    ILI9341_Write_Data ( 0x21 );  //VRH[5:0]
    
    /* Power Control 2 (C1h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xC1 );   //Power control
    ILI9341_Write_Data ( 0x11 );  //SAP[2:0];BT[3:0]
    
    /* VCOM Control 1 (C5h) */
    ILI9341_Write_Cmd ( 0xC5 );
    ILI9341_Write_Data ( 0x2D );
    ILI9341_Write_Data ( 0x33 );
    
    /*  VCOM Control 2 (C7h)  */
//	ILI9341_Write_Cmd ( 0xC7 );
//	ILI9341_Write_Data ( 0XC0 );
    
    /* memory access control set */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0x36 );   //Memory Access Control
    ILI9341_Write_Data ( 0x00 );  /*竖屏  左上角到 (起点)到右下角 (终点)扫描方式*/
    DEBUG_DELAY ();
    
    ILI9341_Write_Cmd(0x3A);   
    ILI9341_Write_Data(0x55); 
    
      /* Frame Rate Control (In Normal Mode/Full Colors) (B1h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xB1 );
    ILI9341_Write_Data ( 0x00 );
    ILI9341_Write_Data ( 0x17 );
    
    /*  Display Function Control (B6h) */
    DEBUG_DELAY ();
    ILI9341_Write_Cmd ( 0xB6 );
    ILI9341_Write_Data ( 0x0A );
    ILI9341_Write_Data ( 0xA2 );
    
    ILI9341_Write_Cmd(0xF6);    			
    ILI9341_Write_Data(0x01); 
    ILI9341_Write_Data(0x30); 
    
    /* Enable 3G (F2h) */
    ILI9341_Write_Cmd ( 0xF2 );
    ILI9341_Write_Data ( 0x00 );
    
    /* Gamma Set (26h) */
    ILI9341_Write_Cmd ( 0x26 );
    ILI9341_Write_Data ( 0x01 );
    DEBUG_DELAY ();
    
    /* Positive Gamma Correction */
    ILI9341_Write_Cmd(0xe0); //Positive gamma
    ILI9341_Write_Data(0xd0);         
    ILI9341_Write_Data(0x00); 
    ILI9341_Write_Data(0x02); 
    ILI9341_Write_Data(0x07); 
    ILI9341_Write_Data(0x0b); 
    ILI9341_Write_Data(0x1a); 
    ILI9341_Write_Data(0x31); 
    ILI9341_Write_Data(0x54); 
    ILI9341_Write_Data(0x40); 
    ILI9341_Write_Data(0x29); 
    ILI9341_Write_Data(0x12); 
    ILI9341_Write_Data(0x12); 
    ILI9341_Write_Data(0x12); 
    ILI9341_Write_Data(0x17);

    /* Negative Gamma Correction (E1h) */
    ILI9341_Write_Cmd(0xe1); //Negative gamma
    ILI9341_Write_Data(0xd0); 
    ILI9341_Write_Data(0x00); 
    ILI9341_Write_Data(0x02); 
    ILI9341_Write_Data(0x07); 
    ILI9341_Write_Data(0x05); 
    ILI9341_Write_Data(0x25); 
    ILI9341_Write_Data(0x2d); 
    ILI9341_Write_Data(0x44); 
    ILI9341_Write_Data(0x45); 
    ILI9341_Write_Data(0x1c); 
    ILI9341_Write_Data(0x18); 
    ILI9341_Write_Data(0x16); 
    ILI9341_Write_Data(0x1c); 
    ILI9341_Write_Data(0x1d); 
  
	
//	/* column address control set */
//	ILI9341_Write_Cmd ( CMD_SetCoordinateX ); 
//	ILI9341_Write_Data ( 0x00 );
//	ILI9341_Write_Data ( 0x00 );
//	ILI9341_Write_Data ( 0x00 );
//	ILI9341_Write_Data ( 0xEF );
//	
//	/* page address control set */
//	DEBUG_DELAY ();
//	ILI9341_Write_Cmd ( CMD_SetCoordinateY ); 
//	ILI9341_Write_Data ( 0x00 );
//	ILI9341_Write_Data ( 0x00 );
//	ILI9341_Write_Data ( 0x01 );
//	ILI9341_Write_Data ( 0x3F );
	
	
    /* Sleep Out (11h)  */
    ILI9341_Write_Cmd ( 0x11 );	  //Exit Sleep
    ILI9341_Delay ( 0xAFFf<<2 );
    DEBUG_DELAY ();
    
    /* Display ON (29h) */
    ILI9341_Write_Cmd ( 0x29 );   //Display on
    
    ILI9341_Write_Cmd(0x2c);
  }
	
}

uint16_t ILI9341_ReadID(void)
{
	uint16_t id = 0;
	
	ILI9341_Write_Cmd(0x04);
	ILI9341_Read_Data();
	ILI9341_Read_Data();
	id = ILI9341_Read_Data();
	id <<= 8;
	id |= ILI9341_Read_Data();
	
  if(id == LCDID_ST7789V)
  {
    return id;
  }
  else
  {
    ILI9341_Write_Cmd(0xD3);
    ILI9341_Read_Data();
    ILI9341_Read_Data();
    id = ILI9341_Read_Data();
    id <<= 8;
    id |= ILI9341_Read_Data();
    
    if(id)
    {
      return LCDID_ILI9341;
    }
  }
  
	return LCDID_UNKNOWN;
}

void ILI9341_GramScan ( uint8_t ucOption )
{	
	//参数检查，只可输入0-7
	if(ucOption >7 )
		return;
	
	//根据模式更新LCD_SCAN_MODE的值，主要用于触摸屏选择计算参数
	LCD_SCAN_MODE = ucOption;
	
	//根据模式更新XY方向的像素宽度
	if(ucOption%2 == 0)	
	{
		//0 2 4 6模式下X方向像素宽度为240，Y方向为320
		LCD_X_LENGTH = ILI9341_LESS_PIXEL;
		LCD_Y_LENGTH =	ILI9341_MORE_PIXEL;
	}
	else				
	{
		//1 3 5 7模式下X方向像素宽度为320，Y方向为240
		LCD_X_LENGTH = ILI9341_MORE_PIXEL;
		LCD_Y_LENGTH =	ILI9341_LESS_PIXEL; 
	}

	//0x36命令参数的高3位可用于设置GRAM扫描方向	
	ILI9341_Write_Cmd ( 0x36 );
  if(s_lcdid == LCDID_ILI9341)
  {
    ILI9341_Write_Data ( 0x08 |(ucOption<<5));//根据ucOption的值设置LCD参数，共0-7种模式
  }
  else if(s_lcdid == LCDID_ST7789V)
  {
    ILI9341_Write_Data ( 0x00 |(ucOption<<5));//根据ucOption的值设置LCD参数，共0-7种模式
  }
	ILI9341_Write_Cmd ( CMD_SetCoordinateX ); 
	ILI9341_Write_Data ( 0x00 );		/* x 起始坐标高8位 */
	ILI9341_Write_Data ( 0x00 );		/* x 起始坐标低8位 */
	ILI9341_Write_Data ( ((LCD_X_LENGTH-1)>>8)&0xFF ); /* x 结束坐标高8位 */	
	ILI9341_Write_Data ( (LCD_X_LENGTH-1)&0xFF );				/* x 结束坐标低8位 */

	ILI9341_Write_Cmd ( CMD_SetCoordinateY ); 
	ILI9341_Write_Data ( 0x00 );		/* y 起始坐标高8位 */
	ILI9341_Write_Data ( 0x00 );		/* y 起始坐标低8位 */
	ILI9341_Write_Data ( ((LCD_Y_LENGTH-1)>>8)&0xFF );	/* y 结束坐标高8位 */	 
	ILI9341_Write_Data ( (LCD_Y_LENGTH-1)&0xFF );				/* y 结束坐标低8位 */

	/* write gram start */
	ILI9341_Write_Cmd ( CMD_SetPixel );	
}

void ILI9341_OpenWindow ( uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight )
{	
	ILI9341_Write_Cmd ( CMD_SetCoordinateX ); 				 /* 设置X坐标 */
	ILI9341_Write_Data ( usX >> 8  );	 /* 先高8位，然后低8位 */
	ILI9341_Write_Data ( usX & 0xff  );	 /* 设置起始点和结束点*/
	ILI9341_Write_Data ( ( usX + usWidth - 1 ) >> 8  );
	ILI9341_Write_Data ( ( usX + usWidth - 1 ) & 0xff  );

	ILI9341_Write_Cmd ( CMD_SetCoordinateY ); 			     /* 设置Y坐标*/
	ILI9341_Write_Data ( usY >> 8  );
	ILI9341_Write_Data ( usY & 0xff  );
	ILI9341_Write_Data ( ( usY + usHeight - 1 ) >> 8 );
	ILI9341_Write_Data ( ( usY + usHeight - 1) & 0xff );
	
	ILI9341_Write_Cmd ( CMD_SetPixel );	/*窗口设好，进入写 GRAM*/
}

static void ILI9341_SetCursor ( uint16_t usX, uint16_t usY )	
{
	ILI9341_OpenWindow ( usX, usY, 1, 1 );
}

static __inline void ILI9341_FillColor ( uint32_t ulAmout_Point, uint16_t usColor )
{
	uint32_t i = 0;
	
	
	/* memory write */
	ILI9341_Write_Cmd ( CMD_SetPixel );	
		
	for ( i = 0; i < ulAmout_Point; i ++ )
		ILI9341_Write_Data ( usColor );
		
	
}

/* ---------------- 下面几个是在本工程里补的适配函数 ---------------- */

#if (ILI9341_USE_BITBANG == 1)
/**
  * @brief  把 LCD 的 8080 引脚改成普通推挽输出，用软件模拟总线（绕开 FSMC）
  */
void bb_bus_init(void)
{
	GPIO_InitTypeDef g = {0};
	uint8_t i;

	g.Mode  = GPIO_MODE_OUTPUT_PP;
	g.Pull  = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

	for (i = 0; i < 16U; i++)
	{
		g.Pin = s_bb_d[i].pin;
		HAL_GPIO_Init(s_bb_d[i].port, &g);
	}
	g.Pin = BB_RS_PIN; HAL_GPIO_Init(BB_RS_PORT, &g);
	g.Pin = BB_CS_PIN; HAL_GPIO_Init(BB_CS_PORT, &g);
	g.Pin = BB_WR_PIN; HAL_GPIO_Init(BB_WR_PORT, &g);
	g.Pin = BB_RD_PIN; HAL_GPIO_Init(BB_RD_PORT, &g);

	HAL_GPIO_WritePin(BB_CS_PORT, BB_CS_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BB_WR_PORT, BB_WR_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BB_RD_PORT, BB_RD_PIN, GPIO_PIN_SET);
}

/**
  * @brief  8080 写一个 16 位数：rs=0 命令 / rs=1 数据
  * @note   时序和 FSMC 那边一样（地址建立 -> WR 低 -> WR 高），只是由 CPU 手动翻转
  */
void BB_Write(uint16_t v, uint8_t rs)
{
	uint8_t i;
	volatile uint32_t k;

	for (i = 0; i < 16U; i++)
	{
		HAL_GPIO_WritePin(s_bb_d[i].port, s_bb_d[i].pin,
		                  (v & (uint16_t)(1U << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}
	HAL_GPIO_WritePin(BB_RS_PORT, BB_RS_PIN, (rs != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);

	HAL_GPIO_WritePin(BB_CS_PORT, BB_CS_PIN, GPIO_PIN_RESET);
	for (k = 0; k < 20U; k++) { }
	HAL_GPIO_WritePin(BB_WR_PORT, BB_WR_PIN, GPIO_PIN_RESET);
	for (k = 0; k < 60U; k++) { }
	HAL_GPIO_WritePin(BB_WR_PORT, BB_WR_PIN, GPIO_PIN_SET);
	for (k = 0; k < 60U; k++) { }
	HAL_GPIO_WritePin(BB_CS_PORT, BB_CS_PIN, GPIO_PIN_SET);
	for (k = 0; k < 20U; k++) { }
}
#endif /* ILI9341_USE_BITBANG */

/**
  * @brief  硬件复位（PF11，低电平有效）
  * @note   LCD_Draw_Config() 里已经复位过一次，这里留着备用
  */
void ILI9341_Rst(void)
{
	HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_RESET);
	ILI9341_Delay(0xAFF);
	HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_SET);
	ILI9341_Delay(0xAFF);
}

/**
  * @brief  返回初始化时面板的真实应答（0 = 面板完全没回应）
  */
uint16_t ILI9341_GetID(void)
{
	return s_rawid;
}

/**
  * @brief  按指定型号初始化（盲测用：分别按 ILI9341 / ST7789V 试一遍，看屏上出哪种颜色）
  * @param  which  LCDID_ILI9341 或 LCDID_ST7789V
  */
void ILI9341_InitAs(uint16_t which)
{
	s_forceid = which;
#if (ILI9341_USE_BITBANG == 1)
	bb_bus_init();
#endif
	ILI9341_REG_Config();
#if (ILI9341_BL_ACTIVE_LOW == 1)
	HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET);
#else
	LCD_Display_BL_ENABLE(1);
#endif
}

/**
  * @brief  面板寄存器初始化 + 开背光
  * @note   FSMC 和引脚已经由 LCD_Display_FSMC_Init()/LCD_Display_Pin_Init() 配好
  */
void ILI9341_Init(void)
{
#if (ILI9341_USE_BITBANG == 1)
	/*软件模拟总线：把 8080 那批脚从 FSMC 复用改成普通输出*/
	bb_bus_init();
#endif
	s_rawid = ILI9341_ReadID();
	ILI9341_REG_Config();          /* 内部还会再读一次 ID，结果一致 */

	/*背光：野火 3.2 / 2.8 寸模块的 LCD_BK 是"低电平点亮"
	  （官方例程里 ILI9341_BackLed_Control(ENABLE) 就是拉低），
	  而骄阳自带的 4.3/5 寸屏是高电平点亮，所以这里不能直接用
	  LCD_Display_BL_ENABLE()，否则背光会被关掉（现象：屏先白后黑）。*/
#if (ILI9341_BL_ACTIVE_LOW == 1)
	HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET);
#else
	LCD_Display_BL_ENABLE(1);
#endif
}

/**
  * @brief  用指定颜色填充一块矩形（挂到 lcd_fill_color 函数指针）
  */
void ILI9341_Fill(uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight, uint16_t usColor)
{
	ILI9341_OpenWindow(usX, usY, usWidth, usHeight);
	ILI9341_FillColor((uint32_t)usWidth * (uint32_t)usHeight, usColor);
}

/**
  * @brief  用指定颜色画一个点（挂到 lcd_set_pixel 函数指针）
  */
void ILI9341_SetPointPixel_Color(uint16_t usX, uint16_t usY, uint16_t usColor)
{
	if ((usX < LCD_X_LENGTH) && (usY < LCD_Y_LENGTH))
	{
		ILI9341_SetCursor(usX, usY);
		ILI9341_FillColor(1, usColor);
	}
}
