/**
  ******************************************************************************
  * @file    bsp_touch.c
  * @brief   3.2 寸模块的电阻触摸（XPT2046，软件模拟 SPI）
  *
  *  时序照着野火官方例程（bsp_xpt2046_lcd.c）来，只是改成 HAL 库 + 本工程的引脚：
  *     写命令：CLK 低 -> 逐位送 MOSI -> CLK 高 -> CLK 低
  *     读数据：MOSI 拉低 -> 12 个时钟，每个下降沿读 MISO
  *  坐标换算用"两点校准"：点左上角和右下角，自动判断 X/Y 是否交换、方向是否相反。
  ******************************************************************************
  */

#include "bsp_touch.h"
#include "lcd_draw.h"
#include <string.h>

/* 居中显示一行字（bsp_ui.c 里的那个是 static，这里自己来一份）*/
static void TP_Center(uint16_t y, const char *s, uint16_t fg, sFONT *font)
{
	uint16_t len = (uint16_t)strlen(s);
	uint16_t wpx = (uint16_t)(len * font->Width);
	uint16_t x   = (wpx < LCD_X_LENGTH) ? (uint16_t)((LCD_X_LENGTH - wpx) / 2U) : 0U;

	LCD_DispString_EN(x, y, (char *)s, fg, BLACK, font);
}

/* ---------------- 触摸引脚 ---------------- */
#define TP_CLK_PORT    GPIOG
#define TP_CLK_PIN     GPIO_PIN_9
#define TP_CS_PORT     GPIOH
#define TP_CS_PIN      GPIO_PIN_4
#define TP_MOSI_PORT   GPIOD
#define TP_MOSI_PIN    GPIO_PIN_6
#define TP_MISO_PORT   GPIOH
#define TP_MISO_PIN    GPIO_PIN_5
#define TP_IRQ_PORT    GPIOG
#define TP_IRQ_PIN     GPIO_PIN_8

#define TP_CLK_H()     HAL_GPIO_WritePin(TP_CLK_PORT,  TP_CLK_PIN,  GPIO_PIN_SET)
#define TP_CLK_L()     HAL_GPIO_WritePin(TP_CLK_PORT,  TP_CLK_PIN,  GPIO_PIN_RESET)
#define TP_CS_H()      HAL_GPIO_WritePin(TP_CS_PORT,   TP_CS_PIN,   GPIO_PIN_SET)
#define TP_CS_L()      HAL_GPIO_WritePin(TP_CS_PORT,   TP_CS_PIN,   GPIO_PIN_RESET)
#define TP_MOSI_H()    HAL_GPIO_WritePin(TP_MOSI_PORT, TP_MOSI_PIN, GPIO_PIN_SET)
#define TP_MOSI_L()    HAL_GPIO_WritePin(TP_MOSI_PORT, TP_MOSI_PIN, GPIO_PIN_RESET)
#define TP_MISO_IN()   HAL_GPIO_ReadPin(TP_MISO_PORT,  TP_MISO_PIN)
#define TP_IRQ_IN()    HAL_GPIO_ReadPin(TP_IRQ_PORT,   TP_IRQ_PIN)

/* 通道控制字（和官方例程一致）*/
#define TP_CH_X        0x90
#define TP_CH_Y        0xD0

/* 校准结果： 屏幕X = (A*rawX + B*rawY + C) / Div
             屏幕Y = (D*rawX + E*rawY + F) / Div  */
static int32_t s_A = 0, s_B = 0, s_C = 0;
static int32_t s_D = 0, s_E = 0, s_F = 0, s_Div = 1;
static uint8_t s_calOk = 0;

/* 开机默认校准值：
     假设 XPT2046 原始值范围大约 200~3800（12 位），不交换坐标、方向都为正。
       x_screen = (A*rawX + B*rawY + C) / Div
       y_screen = (D*rawX + E*rawY + F) / Div
   5 个大按钮够粗糙用了 —— 开机直接能点；点不准时再手动校准一次即可。*/
#define TP_DEF_DIV   4096
#define TP_DEF_RANGE 3600          /* 3800 - 200 */

static void TP_ApplyDefaultCal(void)
{
	s_A   = (int32_t)((int32_t)LCD_X_LENGTH * TP_DEF_DIV / TP_DEF_RANGE);
	s_B   = 0;
	s_C   = -(s_A * 200);
	s_D   = 0;
	s_E   = (int32_t)((int32_t)LCD_Y_LENGTH * TP_DEF_DIV / TP_DEF_RANGE);
	s_F   = -(s_E * 200);
	s_Div = TP_DEF_DIV;
	s_calOk = 1U;               /* 开机就算"已校准"，不弹校准画面 */
}

/* ---------------- 手动校准的参数存最后一个 Flash 扇区 ----------------
   地址 0x080E0000 = F407ZG 的 Sector 11（128KB）。
   工程已经在 Keil 里把代码区限制到 0x08000000 + 0xE0000（896KB），
   所以这一块不会被程序占用，写它不会破坏程序。*/
#define TP_CAL_ADDR    0x080E0000U
#define TP_CAL_MAGIC   0x544F5543U      /* "TOUC" */

/* 读：有存过就用存的值（覆盖默认值）；没存过保持默认值 */
static void TP_LoadCal(void)
{
	const uint32_t *p = (const uint32_t *)TP_CAL_ADDR;

	if (p[0] != TP_CAL_MAGIC)
	{
		return;
	}
	if (p[7] == 0U)
	{
		return;
	}

	s_A   = (int32_t)p[1];
	s_B   = (int32_t)p[2];
	s_C   = (int32_t)p[3];
	s_D   = (int32_t)p[4];
	s_E   = (int32_t)p[5];
	s_F   = (int32_t)p[6];
	s_Div = (int32_t)p[7];
}

/* 写：手动校准成功后调用 */
static void TP_SaveCal(void)
{
	FLASH_EraseInitTypeDef er = {0};
	uint32_t err = 0;
	uint32_t buf[8];
	uint32_t i;

	buf[0] = TP_CAL_MAGIC;
	buf[1] = (uint32_t)s_A;
	buf[2] = (uint32_t)s_B;
	buf[3] = (uint32_t)s_C;
	buf[4] = (uint32_t)s_D;
	buf[5] = (uint32_t)s_E;
	buf[6] = (uint32_t)s_F;
	buf[7] = (uint32_t)s_Div;

	if (HAL_FLASH_Unlock() != HAL_OK)
	{
		return;
	}

	er.TypeErase    = FLASH_TYPEERASE_SECTORS;
	er.Banks        = FLASH_BANK_1;
	er.Sector       = FLASH_SECTOR_11;
	er.NbSectors    = 1;
	er.VoltageRange = FLASH_VOLTAGE_RANGE_3;

	if (HAL_FLASHEx_Erase(&er, &err) == HAL_OK)
	{
		for (i = 0U; i < 8U; i++)
		{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, TP_CAL_ADDR + 4U * i, buf[i]) != HAL_OK)
			{
				break;
			}
		}
	}

	HAL_FLASH_Lock();
}

/* 位延时（SPI 时钟别太快，XPT2046 最高约 2MHz）*/
static void TP_Delay(void)
{
	volatile uint32_t k;
	for (k = 0; k < 40U; k++) { }
}

static void TP_WriteCmd(uint8_t cmd)
{
	uint8_t i;

	TP_MOSI_L();
	TP_CLK_L();
	for (i = 0; i < 8U; i++)
	{
		if ((cmd & (uint8_t)(0x80U >> i)) != 0U)
		{
			TP_MOSI_H();
		}
		else
		{
			TP_MOSI_L();
		}
		TP_Delay();
		TP_CLK_H();
		TP_Delay();
		TP_CLK_L();
	}
}

static uint16_t TP_ReadData(void)
{
	uint8_t i;
	uint16_t buf = 0;

	TP_MOSI_L();
	TP_CLK_H();
	for (i = 0; i < 12U; i++)
	{
		TP_CLK_L();
		if (TP_MISO_IN() != GPIO_PIN_RESET)
		{
			buf |= (uint16_t)(1U << (11 - i));
		}
		TP_CLK_H();
	}

	return (uint16_t)(buf & 0x0FFFU);
}

static uint16_t TP_ReadAdc(uint8_t ch)
{
	uint16_t v;

	TP_CS_L();
	TP_WriteCmd(ch);
	v = TP_ReadData();
	TP_CS_H();

	return v;
}

/* 读一组原始值（X/Y 各读几次取平均，滤掉抖动）*/
static void TP_ReadRaw(int32_t *rx, int32_t *ry)
{
	int32_t sx = 0, sy = 0;
	uint8_t i;

	for (i = 0; i < 4U; i++)
	{
		sx += TP_ReadAdc(TP_CH_X);
		sy += TP_ReadAdc(TP_CH_Y);
	}

	*rx = sx / 4;
	*ry = sy / 4;
}

void Touch_Init(void)
{
	GPIO_InitTypeDef g = {0};

	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/* CLK / CS / MOSI 输出 */
	g.Mode  = GPIO_MODE_OUTPUT_PP;
	g.Pull  = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_HIGH;

	g.Pin = TP_CLK_PIN;   HAL_GPIO_Init(TP_CLK_PORT,  &g);
	g.Pin = TP_CS_PIN;    HAL_GPIO_Init(TP_CS_PORT,   &g);
	g.Pin = TP_MOSI_PIN;  HAL_GPIO_Init(TP_MOSI_PORT, &g);

	/* MISO / IRQ 输入 */
	g.Mode = GPIO_MODE_INPUT;
	g.Pull = GPIO_NOPULL;
	g.Pin  = TP_MISO_PIN; HAL_GPIO_Init(TP_MISO_PORT, &g);
	g.Pin  = TP_IRQ_PIN;  HAL_GPIO_Init(TP_IRQ_PORT,  &g);

	TP_CS_H();
	TP_CLK_L();
	TP_MOSI_L();

	TP_ApplyDefaultCal();     /* 先套一套默认校准值：上电直接就能点 */
	TP_LoadCal();             /* 如果以前手动校准过，就用存下来的那一套 */
}

uint8_t Touch_IsPressed(void)
{
	return (TP_IRQ_IN() == GPIO_PIN_RESET) ? 1U : 0U;   /* 按下时 PENIRQ 为低 */
}

uint8_t Touch_IsCalibrated(void)
{
	return s_calOk;
}

void Touch_ReadRawXY(int32_t *rx, int32_t *ry)
{
	TP_ReadRaw(rx, ry);
}

void Touch_RawToScreen(int32_t rx, int32_t ry, uint16_t *x, uint16_t *y)
{
	int32_t sx, sy;

	if (s_calOk != 0U)
	{
		sx = (s_A * rx + s_B * ry + s_C) / s_Div;
		sy = (s_D * rx + s_E * ry + s_F) / s_Div;
	}
	else
	{
		/* 没校准时先粗略按比例来，只为显示，不用它点按钮 */
		sx = (rx - 200) * (int32_t)LCD_X_LENGTH / 3700;
		sy = (ry - 200) * (int32_t)LCD_Y_LENGTH / 3700;
	}

	if (sx < 0) { sx = 0; }
	if (sy < 0) { sy = 0; }
	if (sx > (int32_t)LCD_X_LENGTH - 1) { sx = (int32_t)LCD_X_LENGTH - 1; }
	if (sy > (int32_t)LCD_Y_LENGTH - 1) { sy = (int32_t)LCD_Y_LENGTH - 1; }

	*x = (uint16_t)sx;
	*y = (uint16_t)sy;
}

uint8_t Touch_GetPoint(uint16_t *x, uint16_t *y)
{
	int32_t rx, ry, sx, sy;

	if (s_calOk == 0U)
	{
		return 0U;
	}

	if (Touch_IsPressed() == 0U)
	{
		return 0U;
	}

	TP_ReadRaw(&rx, &ry);

	sx = (s_A * rx + s_B * ry + s_C) / s_Div;
	sy = (s_D * rx + s_E * ry + s_F) / s_Div;

	if (sx < 0)                       { sx = 0; }
	if (sy < 0)                       { sy = 0; }
	if (sx > (int32_t)LCD_X_LENGTH - 1) { sx = (int32_t)LCD_X_LENGTH - 1; }
	if (sy > (int32_t)LCD_Y_LENGTH - 1) { sy = (int32_t)LCD_Y_LENGTH - 1; }

	*x = (uint16_t)sx;
	*y = (uint16_t)sy;
	return 1U;
}

/* 在屏幕上画一个"点这里"的方框 */
static void TP_DrawTarget(uint16_t cx, uint16_t cy, const char *text)
{
	lcd_fill_color((uint16_t)(cx - 12), (uint16_t)(cy - 12), 24, 24, RED);
	lcd_fill_color(cx, (uint16_t)(cy - 12), 4, 24, WHITE);
	lcd_fill_color((uint16_t)(cx - 12), cy, 24, 4, WHITE);
	TP_Center((uint16_t)(cy + 24), text, WHITE, &Font16x24);
}

/* 等一次"按下 -> 松开"，返回原始坐标 */
static void TP_WaitTap(int32_t *rx, int32_t *ry)
{
	while (Touch_IsPressed() != 0U) { }       /* 先等松开 */
	while (Touch_IsPressed() == 0U) { }       /* 等按下 */
	TP_ReadRaw(rx, ry);
	while (Touch_IsPressed() != 0U) { }       /* 等松开 */
}

uint8_t Touch_Calibrate(void)
{
	int32_t rx1, ry1, rx2, ry2;
	int32_t dxr, dyr;
	uint16_t w = LCD_X_LENGTH, h = LCD_Y_LENGTH;

	lcd_fill_color(0, 0, w, h, BLACK);
	TP_Center(10, "TOUCH CALIBRATION", CYAN, &Font16x24);

	/* ---- 左上角 ---- */
	TP_DrawTarget(25, 60, "tap 1: left-top");
	TP_WaitTap(&rx1, &ry1);

	/* ---- 右下角 ---- */
	lcd_fill_color(0, 0, w, h, BLACK);
	TP_Center(10, "TOUCH CALIBRATION", CYAN, &Font16x24);
	TP_DrawTarget((uint16_t)(w - 26), (uint16_t)(h - 26), "tap 2: right-bottom");
	TP_WaitTap(&rx2, &ry2);

	dxr = rx2 - rx1;
	dyr = ry2 - ry1;

	/* 哪个原始轴的跨度大，就是屏幕的 X 轴（自动判断交换）*/
	if ((dxr > 0 ? dxr : -dxr) >= (dyr > 0 ? dyr : -dyr))
	{
		/* 不交换：屏幕X <- rawX，屏幕Y <- rawY  */
		int32_t span_x = (dxr != 0) ? dxr : 1;
		int32_t span_y = (dyr != 0) ? dyr : 1;

		s_A = (int32_t)(((int32_t)(w - 52) * 4096) / span_x);
		s_B = 0;
		s_C = (int32_t)(25 * 4096) - s_A * rx1;

		s_D = 0;
		s_E = (int32_t)(((int32_t)(h - 86) * 4096) / span_y);
		s_F = (int32_t)(60 * 4096) - s_E * ry1;
	}
	else
	{
		/* 交换：屏幕X <- rawY，屏幕Y <- rawX */
		int32_t span_x = (dyr != 0) ? dyr : 1;
		int32_t span_y = (dxr != 0) ? dxr : 1;

		s_A = 0;
		s_B = (int32_t)(((int32_t)(w - 52) * 4096) / span_x);
		s_C = (int32_t)(25 * 4096) - s_B * ry1;

		s_D = (int32_t)(((int32_t)(h - 86) * 4096) / span_y);
		s_E = 0;
		s_F = (int32_t)(60 * 4096) - s_D * rx1;
	}

	s_Div = 4096;
	s_calOk = 1U;
	TP_SaveCal();             /* 手动校准的参数存进 Flash，以后上电不用再校准 */

	/* 校准完提示一下，等一下再进主界面 */
	lcd_fill_color(0, 0, w, h, BLACK);
	TP_Center((uint16_t)(h / 2 - 20), "TOUCH OK", GREEN, &Font16x24);
	{
		volatile uint32_t k;
		for (k = 0; k < 30000000U; k++) { }
	}

	return 1U;
}
