/**
  ******************************************************************************
  * @file    bsp_ui.c
  * @brief   屏幕显示：把"要移动的距离"等状态画到 LCD 上
  *
  *  排版按屏幕实际分辨率算（LCD_X_LENGTH / LCD_Y_LENGTH）：
  *    3.2 / 2.8 寸（240x320）用紧凑排版，4.3 / 4.5 寸、5 寸用宽松排版，
  *    横屏竖屏都能居中显示。全程只用整数转字符串，不依赖浮点打印。
  ******************************************************************************
  */

#include "bsp_ui.h"
#include "lcd_draw.h"
#include "bsp_ili9341_lcd.h"
#include "../led/bsp_led.h"
#include <string.h>

/* 上电后由 LCD_Draw_Config() 填好 */
static uint16_t s_w = 0;
static uint16_t s_h = 0;

/* 各行的纵坐标（按屏幕高度算出来，小屏用紧凑排版）*/
static uint16_t s_title_y = 0;
static uint16_t s_label_y = 0;
static uint16_t s_dist_y  = 0;
static uint16_t s_unit_y  = 0;
static uint16_t s_mode_y  = 0;
static uint16_t s_state_y = 0;
static uint16_t s_pulse_y = 0;
static uint16_t s_hint_y  = 0;

/* 上一帧的内容，用来判断"有没有变化"，避免每个循环都重画 */
static int32_t  s_last_dist  = 0x7FFFFFFF;
static uint32_t s_last_pulse = 0xFFFFFFFF;
static uint8_t  s_last_mode  = 0xFF;
static uint8_t  s_last_run   = 0xFF;
static uint8_t  s_last_unit  = 0xFF;
static uint32_t s_last_tick  = 0;

static char s_buf[32];

/*开机三色自检：1 = 打开（上电先红、绿、蓝各刷一屏，用来判断数据线通不通）
                 0 = 关闭，直接进界面
  现象对照：
    三色都能看到        -> 面板已初始化、16 位数据线正常
    某种颜色不对（缺色）-> 对应数据线接触不良
    一直白、什么颜色都没有 -> 面板根本没被初始化（排线/接触/座子问题）*/
#define UI_SELFTEST   0

/* 背光极性（和 bsp_ili9341_lcd.h 里的 ILI9341_BL_ACTIVE_LOW 保持一致）*/
#if (ILI9341_BL_ACTIVE_LOW == 1)
  #define UI_BL_ON()   HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET)
  #define UI_BL_OFF()  HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_SET)
#else
  #define UI_BL_ON()   HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_SET)
  #define UI_BL_OFF()  HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET)
#endif

/* 开机背光自检：亮 1 秒 / 灭 1 秒，重复 3 次
     ① 屏幕跟着一亮一灭  = 背光那路通到模块了，问题在"画面内容"
     ② 屏幕毫无变化      = 背光那路也没通（排线/座子/模块）
   自检完保持"亮"。不想要就把这个宏改成 0。*/
#define UI_BL_TEST    0

/*面板型号盲测：1 = 打开（上电先按 ILI9341 初始化并刷红，再按 ST7789V 初始化并刷绿）
                 0 = 关闭
  现象对照：出现红色 = 屏是 ILI9341（3.2 寸）；出现绿色 = 屏是 ST7789V（2.8 寸）；
            两种都没有 = 面板根本没收到初始化指令（排线 / 座子 / 屏的问题）。*/
#define UI_PANEL_DEBUG   0

/**
  * @brief  粗略延时（本工程 HAL_GetTick 是 DWT 秒级量化，不适合做毫秒延时）
  */
static void UiDelayLoop(uint32_t n)
{
	volatile uint32_t k;
	for (k = 0; k < n; k++) { }
}

/**
  * @brief  屏幕白屏时用板上的 LED1 报信，不看屏也能知道走到哪一步
  * @note   慢闪 1 次      = 面板有应答，屏幕初始化成功（界面已经画上去）
  *         LED1/LED2 交替闪 3 轮 = 面板一点没回应（排线 / 座子 / 型号不对）
  *   慢闪是故意的：之前 0.1 秒一闪的数不清，现在一拍约 0.5 秒，一眼能数。
  */
static void UiOkBlink(void)
{
	LED1_ON;
	UiDelayLoop(8000000U);
	LED1_OFF;
	UiDelayLoop(4000000U);
}

static void UiAlarmBlink(void)
{
	uint8_t i;

	for (i = 0; i < 3U; i++)
	{
		LED1_ON;
		UiDelayLoop(4000000U);
		LED1_OFF;
		LED2_ON;
		UiDelayLoop(4000000U);
		LED2_OFF;
	}
}

/**
  * @brief  有符号整数转字符串（不用 printf，microlib 下也稳）
  */
static char *I2S(char *p, int32_t v)
{
	char	tmp[12];
	uint8_t	n = 0;
	uint32_t a;

	if (v < 0)
	{
		*p++ = '-';
		a = (uint32_t)(-v);
	}
	else
	{
		a = (uint32_t)v;
	}

	if (a == 0U)
	{
		*p++ = '0';
		*p = 0;
		return p;
	}

	while (a > 0U)
	{
		tmp[n++] = (char)('0' + (a % 10U));
		a /= 10U;
	}
	while (n > 0U)
	{
		*p++ = tmp[--n];
	}
	*p = 0;
	return p;
}

/**
  * @brief  把字符串居中画在某一行
  */
static void ShowCenter(uint16_t y, const char *s, uint16_t fg, sFONT *font)
{
	uint16_t len = (uint16_t)strlen(s);
	uint16_t w   = (uint16_t)(len * font->Width);
	uint16_t x   = (w < s_w) ? (uint16_t)((s_w - w) / 2) : 0;

	LCD_DispString_EN(x, y, (char *)s, fg, BLACK, font);
}

/**
  * @brief  初始化屏幕 + 画静态部分
  */
/* ==================== 触摸按钮 ====================
   两行五个按钮，和物理按键功能一一对应：
     第一行： UNIT（显示当前单位） |  -  |  +
     第二行： MODE                 |   RUN
   ==================================================*/
#define BTN_BG      0x4208U      /* 按钮底色：深灰 */
#define BTN_BG_DOWN 0x07E0U      /* 按下时：绿色 */
#define BTN_FG      0xFFFFU      /* 字：白 */
#define BTN_FG_DOWN 0x0000U      /* 按下时字：黑 */
#define BTN_MARGIN  4U
#define BTN_GAP     4U

static const char    *s_btxt[5] = { "", "-", "+", "MODE", "RUN" };
static char           s_unitBtn[8] = "10cm";   /* UNIT 按钮上显示当前单位 */
static const uint8_t  s_bact[5] = { UI_ACT_UNIT, UI_ACT_MINUS, UI_ACT_PLUS,
                                    UI_ACT_MODE, UI_ACT_RUN };
static uint16_t s_bx[5][2];
static uint16_t s_by[5][2];

static void DrawButton(uint8_t idx, uint16_t bg, uint16_t fg)
{
	uint16_t x0 = s_bx[idx][0], x1 = s_bx[idx][1];
	uint16_t y0 = s_by[idx][0], y1 = s_by[idx][1];
	uint16_t w  = (uint16_t)(x1 - x0 + 1U);
	uint16_t h  = (uint16_t)(y1 - y0 + 1U);
	uint16_t len;
	uint16_t tx, ty;

	lcd_fill_color(x0, y0, w, h, bg);

	len = (uint16_t)strlen((idx == 0U) ? s_unitBtn : s_btxt[idx]);
	tx  = (uint16_t)(x0 + ((w > (len * Font16x24.Width)) ? ((w - len * Font16x24.Width) / 2U) : 0U));
	ty  = (uint16_t)(y0 + ((h > Font16x24.Height) ? ((h - Font16x24.Height) / 2U) : 0U));
	LCD_DispString_EN(tx, ty, (char *)((idx == 0U) ? s_unitBtn : s_btxt[idx]), fg, bg, &Font16x24);
}

static void LayoutButtons(void)
{
	uint16_t w = s_w;
	uint16_t third = (uint16_t)((w - 2U * BTN_MARGIN - 2U * BTN_GAP) / 3U);
	uint16_t half  = (uint16_t)((w - 2U * BTN_MARGIN - BTN_GAP) / 2U);
	uint16_t h1, h2, y2, y1;

	if (s_h < 400U)
	{
		h1 = 46U; h2 = 40U;
	}
	else
	{
		h1 = 56U; h2 = 50U;
	}

	y2 = (uint16_t)(s_h - h2 - BTN_MARGIN);
	y1 = (uint16_t)(y2 - BTN_GAP - h1);

	s_bx[0][0] = BTN_MARGIN;                        s_bx[0][1] = (uint16_t)(BTN_MARGIN + third - 1U);
	s_bx[1][0] = (uint16_t)(BTN_MARGIN + third + BTN_GAP);   s_bx[1][1] = (uint16_t)(BTN_MARGIN + 2U * third + BTN_GAP - 1U);
	s_bx[2][0] = (uint16_t)(BTN_MARGIN + 2U * (third + BTN_GAP)); s_bx[2][1] = (uint16_t)(w - BTN_MARGIN - 1U);
	s_bx[3][0] = BTN_MARGIN;                        s_bx[3][1] = (uint16_t)(BTN_MARGIN + half - 1U);
	s_bx[4][0] = (uint16_t)(BTN_MARGIN + half + BTN_GAP);    s_bx[4][1] = (uint16_t)(w - BTN_MARGIN - 1U);

	s_by[0][0] = y1; s_by[0][1] = (uint16_t)(y1 + h1 - 1U);
	s_by[1][0] = y1; s_by[1][1] = (uint16_t)(y1 + h1 - 1U);
	s_by[2][0] = y1; s_by[2][1] = (uint16_t)(y1 + h1 - 1U);
	s_by[3][0] = y2; s_by[3][1] = (uint16_t)(y2 + h2 - 1U);
	s_by[4][0] = y2; s_by[4][1] = (uint16_t)(y2 + h2 - 1U);
}

static void DrawButtons(void)
{
	uint8_t i;

	for (i = 0U; i < 5U; i++)
	{
		DrawButton(i, BTN_BG, BTN_FG);
	}
}

void UI_RedrawAll(void)
{
	if (s_w == 0U)
	{
		return;
	}

	lcd_fill_color(0, 0, s_w, s_h, BLACK);

	ShowCenter(s_title_y, "STEPPER MOTOR", CYAN,  &Font16x32);
	ShowCenter(s_label_y, "DIST TO MOVE",  WHITE, &Font16x24);

	LayoutButtons();
	DrawButtons();

	/* 强制定位一次，下一次 UI_Update 会把动态内容画全 */
	s_last_dist  = 0x7FFFFFFF;
	s_last_pulse = 0xFFFFFFFF;
	s_last_mode  = 0xFF;
	s_last_run   = 0xFF;
	s_last_unit  = 0xFF;
}

uint8_t UI_Init(void)
{
	if (LCD_Draw_Config() != 0U)
	{
		UiAlarmBlink();         /* 认屏失败：两个灯交替闪 3 轮 */
		return 1U;              /* 没认出屏幕：线没插好，或者不是支持的这几种面板 */
	}

	s_w = LCD_X_LENGTH;
	s_h = LCD_Y_LENGTH;

	if (s_h < 400U)
	{
		/* ---- 3.2 / 2.8 寸（240x320）紧凑排版 ---- */
		s_title_y = 4;
		s_label_y = (uint16_t)(s_title_y + Font16x32.Height + 4U);
		s_dist_y  = (uint16_t)(s_label_y + Font16x24.Height + 2U);
		s_unit_y  = (uint16_t)(s_dist_y  + Font24x48.Height + 2U);
		s_mode_y  = (uint16_t)(s_unit_y  + Font16x24.Height + 4U);
		s_state_y = (uint16_t)(s_mode_y  + Font16x24.Height + 2U);
		s_pulse_y = (uint16_t)(s_state_y + Font16x24.Height + 2U);
		s_hint_y  = (uint16_t)(s_pulse_y + Font16x24.Height + 6U);
	}
	else
	{
		/* ---- 4.3 / 4.5 寸、5 寸 宽松排版 ---- */
		s_title_y = 10;
		s_label_y = (uint16_t)(s_title_y + Font16x32.Height + 12U);
		s_dist_y  = (uint16_t)(s_label_y + Font16x24.Height + 10U);
		s_unit_y  = (uint16_t)(s_dist_y  + Font24x48.Height + 6U);
		s_mode_y  = (uint16_t)(s_unit_y  + Font16x24.Height + 14U);
		s_state_y = (uint16_t)(s_mode_y  + Font16x24.Height + 8U);
		s_pulse_y = (uint16_t)(s_state_y + Font16x24.Height + 8U);
		s_hint_y  = (uint16_t)(s_pulse_y + Font16x24.Height + 16U);
	}

	lcd_fill_color(0, 0, s_w, s_h, BLACK);

#if (UI_BL_TEST == 1)
	/* ---- 背光自检：亮 1 秒 / 灭 1 秒，重复 3 次，最后保持亮 ---- */
	{
		uint8_t n;
		for (n = 0; n < 3U; n++)
		{
			UI_BL_ON();
			UiDelayLoop(15000000U);
			UI_BL_OFF();
			UiDelayLoop(15000000U);
		}
		UI_BL_ON();
	}
#endif

	/* ---- 静态：标题 + 标签 + 按键提示 ---- */
	UI_RedrawAll();

#if (UI_SELFTEST == 1)
	/* ---- 开机反复刷色（红/绿/蓝/白 各约 1 秒，循环 20 轮 ≈ 80 秒）----
	     目的：① 屏能显示就一眼看到在变色；② 这段时间里一直在往屏里写数据，
	           方便用万用表/示波器量 WR、D0~D15 有没有动作。*/
	{
		uint8_t round;
		for (round = 0; round < 20U; round++)
		{
			lcd_fill_color(0, 0, s_w, s_h, RED);
			UiDelayLoop(15000000U);
			lcd_fill_color(0, 0, s_w, s_h, GREEN);
			UiDelayLoop(15000000U);
			lcd_fill_color(0, 0, s_w, s_h, BLUE);
			UiDelayLoop(15000000U);
			lcd_fill_color(0, 0, s_w, s_h, WHITE);
			UiDelayLoop(15000000U);
		}
	}

	/* 自检完把界面重画一遍（上面被颜色盖掉了）*/
	UI_RedrawAll();
#endif

	/* ---- LED 只用来辅助判断，正常看屏幕就行 ----
	     LED1 慢闪 1 次        = 屏幕初始化流程走完了
	     LED2 再慢闪 1 次      = 顺带提示：面板没应答读 ID（不影响显示，说明 RD 读通路有问题）*/
	UiOkBlink();
	if (LCD_PANEL_ID == 0U)
	{
		LED2_ON;
		UiDelayLoop(8000000U);
		LED2_OFF;
	}
	return 0U;
}

/**
  * @brief  刷新动态内容（只在变化时重画）
  */
void UI_Update(uint8_t mode, int32_t dist_mm, uint8_t running, uint32_t pulse, uint8_t unit_idx)
{
	char    *p;
	uint8_t  need_pulse;

	if (s_w == 0U)
	{
		return;                 /* 屏幕没初始化成功，什么都不做 */
	}

	/* ---- 要移动的距离：大字号居中（按当前单位显示）---- */
	if ((dist_mm != s_last_dist) || (mode != s_last_mode) || (unit_idx != s_last_unit))
	{
		s_last_dist = dist_mm;

		if (unit_idx >= 2U)
		{
			/* 1mm 档：按 mm 显示整数 */
			p = I2S(s_buf, dist_mm);
			*p++ = ' ';
			*p++ = 'm';
			*p++ = 'm';
			*p   = 0;
		}
		else
		{
			/* 10cm / 1cm 档：按 cm 显示，保留 1 位小数 */
			uint32_t a = (uint32_t)((dist_mm < 0) ? -dist_mm : dist_mm);
			p = s_buf;
			if (dist_mm < 0)
			{
				*p++ = '-';
			}
			p = I2S(p, (int32_t)(a / 10U));
			*p++ = '.';
			*p++ = (char)('0' + (char)(a % 10U));
			*p++ = ' ';
			*p++ = 'c';
			*p++ = 'm';
			*p   = 0;
		}

		lcd_fill_color(0, s_dist_y, s_w, Font24x48.Height, BLACK);
		ShowCenter(s_dist_y, s_buf, YELLOW, &Font24x48);
	}

	/* ---- 当前单位（KEY3 切换）---- */
	if (unit_idx != s_last_unit)
	{
		s_last_unit = unit_idx;

		strcpy(s_buf, "STEP : ");
		strcat(s_buf, (unit_idx == 0U) ? "10cm" : ((unit_idx == 1U) ? "1cm" : "1mm"));
		lcd_fill_color(0, s_unit_y, s_w, Font16x24.Height, BLACK);
		ShowCenter(s_unit_y, s_buf, YELLOW, &Font16x24);

		/* UNIT 按钮上的文字也跟着变 */
		strcpy(s_unitBtn, (unit_idx == 0U) ? "10cm" : ((unit_idx == 1U) ? "1cm" : "1mm"));
		DrawButton(0U, BTN_BG, BTN_FG);
	}

	/* ---- 模式 ---- */
	if (mode != s_last_mode)
	{
		s_last_mode = mode;

		strcpy(s_buf, (mode == 0U) ? "MODE : 1 & 2" : "MODE : 3 & 4");
		lcd_fill_color(0, s_mode_y, s_w, Font16x24.Height, BLACK);
		ShowCenter(s_mode_y, s_buf, GREEN, &Font16x24);
	}

	/* ---- 运行状态（运动时约 1 秒刷一次脉冲数）---- */
	need_pulse = (uint8_t)((running != 0U) &&
	                       ((HAL_GetTick() - s_last_tick) >= 200U));

	if ((running != s_last_run) || (need_pulse != 0U))
	{
		s_last_tick = HAL_GetTick();

		strcpy(s_buf, (running != 0U) ? "STATE : RUNNING" : "STATE : READY");
		lcd_fill_color(0, s_state_y, s_w, Font16x24.Height, BLACK);
		ShowCenter(s_state_y, s_buf, (running != 0U) ? BRRED : WHITE, &Font16x24);
	}

	if ((pulse != s_last_pulse) || (running != s_last_run))
	{
		s_last_pulse = pulse;

		strcpy(s_buf, "PULSE : ");
		(void)I2S(s_buf + 8, (int32_t)pulse);
		lcd_fill_color(0, s_pulse_y, s_w, Font16x24.Height, BLACK);
		ShowCenter(s_pulse_y, s_buf, GBLUE, &Font16x24);
	}

	s_last_run = running;
}

/**
  * @brief  触摸点落在按钮上时，返回对应动作（并让按钮闪一下做反馈）
  * @param  x,y  触摸点（屏幕坐标）
  * @retval UI_ACT_xxx，没命中按钮返回 UI_ACT_NONE
  */
uint8_t UI_TouchHandle(uint16_t x, uint16_t y)
{
	uint8_t i;

	if (s_w == 0U)
	{
		return UI_ACT_NONE;
	}

	for (i = 0U; i < 5U; i++)
	{
		if ((x >= s_bx[i][0]) && (x <= s_bx[i][1]) &&
		    (y >= s_by[i][0]) && (y <= s_by[i][1]))
		{
			/* 按下反馈：按钮变绿，松开后恢复 */
			DrawButton(i, BTN_BG_DOWN, BTN_FG_DOWN);
			UiDelayLoop(4000000U);
			DrawButton(i, BTN_BG, BTN_FG);

			return s_bact[i];
		}
	}

	return UI_ACT_NONE;
}
