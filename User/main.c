/**
  ******************************************************************************
  * @file    main.c
  * @brief   四个步进电机：1、2号同步，3、4号同步，用KEY2按键切换控制哪一组
  *
  *   按键分配（引脚见 Project\key.c，都在 Key_Init() 里配好）：
  *     KEY1 = PA0   按一次，让当前模式对应的那一组走指定的距离
  *     KEY2 = PG2   切换模式：模式0只控制电机1、2；模式1控制电机3、4
  *     KEY3 = PC13  切换单位：10cm -> 1cm -> 1mm 循环
  *     KEY5 = PG4   按当前单位 +（加一个单位）
  *     KEY4 = PG3   按当前单位 -（减一个单位；负值就是反转）
  *
  *   LED指示：
  *     模式0（电机1、2）-> LED1(PA15) 亮
  *     模式1（电机3、4）-> LED2(PE2)  亮
  ******************************************************************************
  */
#include "main.h"
#include "stm32f4xx.h"
#include "./delay/core_delay.h"
#include "./stepper/bsp_stepper_init.h"
#include "./led/bsp_led.h"
#include <stdbool.h>
#include "key.h"
#include "./stepper/bsp_stepper_T_speed.h"
#include "./lcd/bsp_ui.h"
#include "./lcd/bsp_touch.h"

/*======================== 运动参数 ========================
  单位：0.1rad/sec、0.1rad/sec^2
  set_speed = 100 -> 10rad/s -> 约95转/分
                    丝杠导程5mm时约8mm/s
  这三个值越大走得越快，但太大就会丢步。
  调的时候一次只加一点，加到听不到"咯噔"声、位置也对得上为止。
===========================================================*/
__IO uint32_t set_speed = 100;         // 速度 单位为0.1rad/sec
__IO uint32_t accel_val = 100;         // 加速度 单位为0.1rad/sec^2
__IO uint32_t decel_val = 100;         // 减速度 单位为0.1rad/sec^2

/*====================== 两组各自的距离 ======================
  单位mm，正数朝一个方向，负数朝反方向
===========================================================*/
/*距离的上下限，单位mm（10cm 档一次加 100mm，所以上限放宽到 1000）*/
#define DIST_MAX   1000
__IO int32_t Motor_Distance12 = 10;     //模式0：电机1、2 要走的距离
__IO int32_t Motor_Distance34 = 10;     //模式1：电机3、4 要走的距离

/*当前模式：0=只控制电机1、2   1=控制电机3、4*/
__IO uint8_t Mode = 0;

/*====================== 距离的单位（KEY3 切换）======================
  三个档位，KEY5/KEY4 就是按这个档位"加一个/减一个"：
      0 = 10cm 档（每次 ±100 mm）
      1 = 1cm  档（每次 ±10 mm）
      2 = 1mm  档（每次 ±1 mm）
==================================================================*/
#define UNIT_NUM   3
static const int32_t s_unit_mm[UNIT_NUM] = { 100, 10, 1 };
__IO uint8_t UnitIdx = 0;        /* 当前单位档位：0=10cm 1=1cm 2=1mm */

/*====================== 距离换算 ======================
  电机转一圈需要 200步 × 32细分 = 6400 个脉冲
  Screw_Lead12/34 是"电机转一圈，机构走多少毫米"
    轴1接丝杠：填导程，5mm导程就填5
    轴2接齿轮/皮带：填齿轮或皮带转一圈对应的毫米数
  距离(mm) -> 脉冲数 = 距离 × 6400 ÷ 每圈毫米数
=====================================================*/
#define PULSE_PER_REV   6400.0
__IO  double_t Screw_Lead12 = 5;        //轴1：丝杠导程5mm
__IO  double_t Screw_Lead34 = 5;        //轴2：按实际传动改

/**
  * @brief  把距离(mm)换算成需要的脉冲数
  * @param  Distance   距离，单位mm
  * @param  Lead_MM    机构转一圈走多少mm
  * @retval 需要的脉冲数
  */
int32_t Calc_PulseNum(int32_t Distance, double_t Lead_MM)
{
	return (int32_t)((double)Distance * PULSE_PER_REV / Lead_MM);
}

/**
  * @brief  执行一个动作（物理按键和屏幕触摸按钮共用这一段）
  * @param  act  UI_ACT_UNIT / MINUS / PLUS / MODE / RUN
  */
static void DoAction(uint8_t act)
{
	switch (act)
	{
		case UI_ACT_UNIT:                 /* 切换单位 10cm -> 1cm -> 1mm */
			UnitIdx = (uint8_t)((UnitIdx + 1U) % UNIT_NUM);
			break;

		case UI_ACT_PLUS:                 /* 按当前单位 + */
			if (Mode == 0)
			{
				Motor_Distance12 += s_unit_mm[UnitIdx];
				if (Motor_Distance12 > DIST_MAX)  Motor_Distance12 = DIST_MAX;
			}
			else
			{
				Motor_Distance34 += s_unit_mm[UnitIdx];
				if (Motor_Distance34 > DIST_MAX)  Motor_Distance34 = DIST_MAX;
			}
			break;

		case UI_ACT_MINUS:                /* 按当前单位 - */
			if (Mode == 0)
			{
				Motor_Distance12 -= s_unit_mm[UnitIdx];
				if (Motor_Distance12 < -DIST_MAX) Motor_Distance12 = -DIST_MAX;
			}
			else
			{
				Motor_Distance34 -= s_unit_mm[UnitIdx];
				if (Motor_Distance34 < -DIST_MAX) Motor_Distance34 = -DIST_MAX;
			}
			break;

		case UI_ACT_MODE:                 /* 切换模式 */
			Mode ^= 1;
			if (Mode == 0)
			{
				LED1_ON;
				LED2_OFF;
			}
			else
			{
				LED1_OFF;
				LED2_ON;
			}
			break;

		case UI_ACT_RUN:                  /* 让当前模式那一组走一次 */
			if (Mode == 0)
			{
				stepper_move_T(AXIS_12,
				               Calc_PulseNum(Motor_Distance12, Screw_Lead12),
				               accel_val, decel_val, set_speed);
			}
			else
			{
				stepper_move_T(AXIS_34,
				               Calc_PulseNum(Motor_Distance34, Screw_Lead34),
				               accel_val, decel_val, set_speed);
			}
			break;

		default:
			break;
	}
}

/**
  * @brief  按键扫描：只在"松开->按下"的那一次返回1，并做简单消抖
  * @note   主循环一微秒能跑好几轮，不加这个的话按住一下会被当成
  *         按了几万次，距离数值会直接跑飞，脉冲数也跟着溢出。
  */
static uint8_t Key_Scan_Edge(GPIO_TypeDef *port, uint16_t pin, uint8_t *last)
{
	uint8_t now = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1 : 0;
	volatile uint32_t guard;

	if (now && (*last == 0))
	{
		*last = 1;
		for (guard = 0; guard < 100000; guard++) { }   /*几毫秒，避开机械抖动*/
		return 1;
	}
	if (!now)
	{
		*last = 0;
	}
	return 0;
}

///////////////////////////////////////////////////////
/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void) 
{
	uint8_t last_k1 = 0, last_k2 = 0, last_k3 = 0, last_k4 = 0, last_k5 = 0;

	/* HAL 初始化：LCD 驱动要用 HAL_Delay()/HAL_GetTick()，必须先调它；
	   本工程的 HAL_InitTick() 被 core_delay.c 重写成"使能 DWT 计数器"，
	   所以这里不会占用 SysTick，步进电机的 TIM8 中断不受影响。*/
	HAL_Init();

	/* 初始化系统时钟为168MHz */
	SystemClock_Config();
	/*初始化USART 配置模式为 115200 8-N-1，中断接收*/
	//DEBUG_USART_Config();
	/*四个步进电机初始化（IO + TIM8四通道 + 中断）*/
	stepper_Init();
	/*LED初始化*/
	LED_GPIO_Config();
	/*按键初始化（PA0 / PG2 PG3 PG4 / PC13）*/
	Key_Init();

	/*屏幕初始化（野火 F407 骄阳官方 LCD 例程：FSMC 8080 + 自动识别面板）*/
	if (UI_Init() == 0U)
	{
		/* 触摸初始化：内部先套一套默认校准值，所以上电直接就能点，不会弹校准画面 */
		Touch_Init();

		/* 只有想重新校准时才用：上电时手指按住屏幕不放，就进两点校准 */
		if (Touch_IsPressed() != 0U)
		{
			(void)Touch_Calibrate();
			UI_RedrawAll();
		}
	}

	/*上电默认模式0：只控制电机1、2*/
	Mode = 0;
	LED1_ON;      /*模式0亮LED1*/
	LED2_OFF;

	while(1)
	{ 
		/*=============== KEY2(PG2)：切换模式 ===============*/
		if (Key_Scan_Edge(GPIOG, GPIO_PIN_2, &last_k2))
		{
			DoAction(UI_ACT_MODE);
		}

		/*=============== KEY3(PC13)：切换单位 10cm / 1cm / 1mm ===============*/
		if (Key_Scan_Edge(GPIOC, GPIO_PIN_13, &last_k3))
		{
			DoAction(UI_ACT_UNIT);
		}

		/*=========== KEY1(PA0)：按当前模式走一次 ===========*/
		if (Key_Scan_Edge(GPIOA, GPIO_PIN_0, &last_k1))
		{
			DoAction(UI_ACT_RUN);
		}

		/*======= KEY5(PG4)：按当前单位 + =======*/
		if (Key_Scan_Edge(GPIOG, GPIO_PIN_4, &last_k5))
		{
			DoAction(UI_ACT_PLUS);
		}

		/*======= KEY4(PG3)：按当前单位 - =======*/
		if (Key_Scan_Edge(GPIOG, GPIO_PIN_3, &last_k4))
		{
			DoAction(UI_ACT_MINUS);
		}

		/*=============== 屏幕触摸按钮 ===============
		  点哪个按钮就执行和物理按键一样的动作；
		  点完等手抬起来，避免一次按住被当成连发。*/
#if (UI_TOUCH_DEBUG == 1)
		/*【触摸调试模式】不停显示 INT 电平、原始值和换算坐标，方便判断触摸有没有反应 */
		{
			static uint32_t s_dbgCnt = 0;

			if (++s_dbgCnt >= 2000U)
			{
				int32_t  rx = 0, ry = 0;
				uint16_t dx = 0, dy = 0;

				s_dbgCnt = 0;
				Touch_ReadRawXY(&rx, &ry);
				Touch_RawToScreen(rx, ry, &dx, &dy);
				UI_ShowTouchDebug(Touch_IsPressed(), rx, ry, dx, dy, Touch_IsCalibrated());
			}
		}
#else
		{
			uint16_t tx, ty;

			if (Touch_GetPoint(&tx, &ty) != 0U)
			{
				uint8_t act = UI_TouchHandle(tx, ty);

				if (act != UI_ACT_NONE)
				{
					DoAction(act);
				}

				while (Touch_IsPressed() != 0U) { }
			}
			else if ((Touch_IsPressed() != 0U) && (Touch_IsCalibrated() == 0U))
			{
				/* Flash 里还没有校准数据（第一次用）：按下的这一下就当成"要校准"，
				   松手后进校准画面，校准完写进 Flash，以后就不用再校准了。 */
				while (Touch_IsPressed() != 0U) { }
				(void)Touch_Calibrate();
				UI_RedrawAll();
			}
		}
#endif /* UI_TOUCH_DEBUG */

		/*=============== 屏幕：显示当前模式"要移动的距离" ===============
		  内容没变化时 UI_Update() 内部不会重画，所以在主循环里随便调用，
		  不会影响发脉冲（脉冲是在 TIM8 中断里发的）。*/
		UI_Update(Mode,
		          (Mode == 0) ? Motor_Distance12 : Motor_Distance34,
		          stepper_is_running((Mode == 0) ? AXIS_12 : AXIS_34),
		          stepper_get_steps((Mode == 0) ? AXIS_12 : AXIS_34),
		          UnitIdx);
	} 	
}
/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
 void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while(1) {};
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    while(1) {};
  }

  /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
  if (HAL_GetREVID() == 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
