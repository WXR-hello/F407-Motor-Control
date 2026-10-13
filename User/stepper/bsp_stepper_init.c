/**
  ******************************************************************************
  * @file    bsp_stepper_init.c
  * @brief   四个步进电机的初始化 + TIM8_CC 中断服务函数
  * @attention 实验平台:野火  STM32 F407 开发板
  ******************************************************************************
  */
#include "./stepper/bsp_stepper_init.h"
#include "./delay/core_delay.h"
#include "stm32f4xx.h"
#include "math.h"
#include <math.h>

TIM_HandleTypeDef TIM_TimeBaseStructure;

/**
  * @brief  配置四个电机的 DIR / ENA / PUL 引脚
  * @param  无
  * @retval 无
  */
static void Stepper_GPIO_Config(void) 
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/*开启四个电机用到的GPIO时钟*/
	MOTOR1_DIR_GPIO_CLK_ENABLE();     /*GPIOE  PE1 */
	MOTOR1_EN_GPIO_CLK_ENABLE();      /*GPIOE  PE0 */
	MOTOR1_PUL_GPIO_CLK_ENABLE();     /*GPIOI  PI5 */
	MOTOR2_DIR_GPIO_CLK_ENABLE();     /*GPIOI  PI8 */
	MOTOR2_EN_GPIO_CLK_ENABLE();      /*GPIOE  PE4 */
	MOTOR2_PUL_GPIO_CLK_ENABLE();     /*GPIOI  PI6 */
	MOTOR3_DIR_GPIO_CLK_ENABLE();     /*GPIOI  PI11*/
	MOTOR3_EN_GPIO_CLK_ENABLE();      /*GPIOI  PI10*/
	MOTOR3_PUL_GPIO_CLK_ENABLE();     /*GPIOI  PI7 */
	MOTOR4_DIR_GPIO_CLK_ENABLE();     /*GPIOF  PF2 */
	MOTOR4_EN_GPIO_CLK_ENABLE();      /*GPIOF  PF1 */
	MOTOR4_PUL_GPIO_CLK_ENABLE();     /*GPIOC  PC9 */

	/*---------- 方向、使能：普通推挽输出 ----------*/
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	GPIO_InitStruct.Pin = MOTOR1_DIR_PIN;
	HAL_GPIO_Init(MOTOR1_DIR_GPIO_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = MOTOR1_EN_PIN;
	HAL_GPIO_Init(MOTOR1_EN_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = MOTOR2_DIR_PIN;
	HAL_GPIO_Init(MOTOR2_DIR_GPIO_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = MOTOR2_EN_PIN;
	HAL_GPIO_Init(MOTOR2_EN_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = MOTOR3_DIR_PIN;
	HAL_GPIO_Init(MOTOR3_DIR_GPIO_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = MOTOR3_EN_PIN;
	HAL_GPIO_Init(MOTOR3_EN_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = MOTOR4_DIR_PIN;
	HAL_GPIO_Init(MOTOR4_DIR_GPIO_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = MOTOR4_EN_PIN;
	HAL_GPIO_Init(MOTOR4_EN_GPIO_PORT, &GPIO_InitStruct);

	/*---------- 脉冲：TIM8复用推挽输出 ----------*/
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_NOPULL;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = MOTOR1_PUL_GPIO_AF;   /*GPIO_AF3_TIM8，四个脚都一样*/

	GPIO_InitStruct.Pin = MOTOR1_PUL_PIN;
	HAL_GPIO_Init(MOTOR1_PUL_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = MOTOR2_PUL_PIN;
	HAL_GPIO_Init(MOTOR2_PUL_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = MOTOR3_PUL_PIN;
	HAL_GPIO_Init(MOTOR3_PUL_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = MOTOR4_PUL_PIN;
	HAL_GPIO_Init(MOTOR4_PUL_PORT, &GPIO_InitStruct);

	/*初始方向都设为顺时针*/
	MOTOR1_DIR(CW);
	MOTOR2_DIR(CW);
	MOTOR3_DIR(CW);
	MOTOR4_DIR(CW);

	/*四个驱动器全部使能（ENA低电平有效），不转的时候电机是锁住的，
	  这样负载不会因为断电而滑动，也就不容易丢位置*/
	MOTOR1_EN(ON);
	MOTOR2_EN(ON);
	MOTOR3_EN(ON);
	MOTOR4_EN(ON);
}

 /**
  * @brief  中断优先级配置
  * @param  无
  * @retval 无
  */
static void TIMx_NVIC_Configuration(void)
{
	/* 外设中断配置 */
	HAL_NVIC_SetPriority(MOTOR1_PUL_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(MOTOR1_PUL_IRQn);
}

/*
 * 注意：TIM_TimeBaseInitTypeDef结构体里面有5个成员，TIM6和TIM7的寄存器里面只有
 * TIM_Prescaler和TIM_Period，所以使用TIM6和TIM7的时候只需初始化这两个成员即可，
 * 另外三个成员是通用定时器和高级定时器才有.
 *-----------------------------------------------------------------------------
 * TIM_Prescaler         都有
 * TIM_CounterMode			 TIMx,x[6,7]没有，其他都有（基本定时器）
 * TIM_Period            都有
 * TIM_ClockDivision     TIMx,x[6,7]没有，其他都有(基本定时器)
 * TIM_RepetitionCounter TIMx,x[1,8]才有(高级定时器)
 *-----------------------------------------------------------------------------
 */

/**
  * @brief  TIM8配置：四个通道都配成输出比较翻转模式
  * @note   计数器从现在开始一直跑，两个轴共用它，
  *         靠各自通道的比较值决定什么时候翻转。
  */
void TIM_PWMOUTPUT_Config(void)
{
	TIM_OC_InitTypeDef  TIM_OCInitStructure = {0};

	/*使能定时器时钟*/
	MOTOR1_PUL_CLK_ENABLE();

	TIM_TimeBaseStructure.Instance = MOTOR1_PUL_TIM;      /*TIM8*/
	/* 累计 TIM_Period个后产生一个更新或者中断
	   计数器是16位的，所以取0xFFFF */
	TIM_TimeBaseStructure.Init.Period = TIM_PERIOD-1;
	/* 高级控制定时器时钟源TIMxCLK = HCLK = 168MHz
	   168/(28) = 6MHz，一个计数拍约0.167us
	   定时器频率 = TIMxCLK/(TIM_Prescaler+1) */
	TIM_TimeBaseStructure.Init.Prescaler = TIM_PRESCALER-1;
	/*计数方式*/
	TIM_TimeBaseStructure.Init.CounterMode = TIM_COUNTERMODE_UP;
	/*采样时钟分频*/
	TIM_TimeBaseStructure.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
	TIM_TimeBaseStructure.Init.RepetitionCounter = 0 ;
	/*初始化定时器*/
	HAL_TIM_OC_Init(&TIM_TimeBaseStructure);

	/*输出比较翻转模式：CNT等于CCRx时，该通道的输出翻转一次*/
	TIM_OCInitStructure.OCMode = TIM_OCMODE_TOGGLE;
	/*比较输出的计数值*/
	TIM_OCInitStructure.Pulse = 0;
	/*当定时器计数值小于CCR1_Val时为高电平*/
	TIM_OCInitStructure.OCPolarity = TIM_OCPOLARITY_HIGH;
	/*设置互补通道输出的极性*/
	TIM_OCInitStructure.OCNPolarity = TIM_OCNPOLARITY_LOW;
	/*快速模式设置*/
	TIM_OCInitStructure.OCFastMode = TIM_OCFAST_DISABLE;
	/*空闲电平*/
	TIM_OCInitStructure.OCIdleState = TIM_OCIDLESTATE_RESET;
	/*互补通道设置*/
	TIM_OCInitStructure.OCNIdleState = TIM_OCNIDLESTATE_RESET;

	/*四个通道一起配好：CH1/CH2是轴0，CH3/CH4是轴1*/
	HAL_TIM_OC_ConfigChannel(&TIM_TimeBaseStructure, &TIM_OCInitStructure, MOTOR1_PUL_CHANNEL_x);
	HAL_TIM_OC_ConfigChannel(&TIM_TimeBaseStructure, &TIM_OCInitStructure, MOTOR2_PUL_CHANNEL_x);
	HAL_TIM_OC_ConfigChannel(&TIM_TimeBaseStructure, &TIM_OCInitStructure, MOTOR3_PUL_CHANNEL_x);
	HAL_TIM_OC_ConfigChannel(&TIM_TimeBaseStructure, &TIM_OCInitStructure, MOTOR4_PUL_CHANNEL_x);

	/*高级定时器必须先开MOE，通道才能输出*/
	__HAL_TIM_MOE_ENABLE(&TIM_TimeBaseStructure);
	/*计数器常开，两个轴共用；各轴自己的中断在stepper_move_T里单独使能*/
	__HAL_TIM_ENABLE(&TIM_TimeBaseStructure);
}

/**
  * @brief  引脚初始化
  * @retval 无
  */
void stepper_Init(void)
{
	/*四个电机的IO配置*/
	Stepper_GPIO_Config();
	/*定时器配置*/
	TIM_PWMOUTPUT_Config();
	/*中断配置*/
	TIMx_NVIC_Configuration();
}

/**
  * @brief  定时器中断服务函数
  * @note   四个脉冲脚都在TIM8上，所以四路共用一个中断
  *         轴0用CC1当节拍，轴1用CC3当节拍；
  *         CC2/CC4只是跟着翻转，不产生中断
  */
void MOTOR1_PUL_IRQHandler(void)
{ 
	/*---------- 轴0：电机1 + 电机2 ----------*/
	if (__HAL_TIM_GET_FLAG(&TIM_TimeBaseStructure, MOTOR1_TIM_FLAG_CCx) != RESET)
	{
		if (__HAL_TIM_GET_IT_SOURCE(&TIM_TimeBaseStructure, MOTOR1_TIM_IT_CCx) != RESET)
		{
			speed_decision_axis(AXIS_12);
		}
		else
		{
			__HAL_TIM_CLEAR_IT(&TIM_TimeBaseStructure, MOTOR1_TIM_IT_CCx);
		}
	}

	/*---------- 轴1：电机3 + 电机4 ----------*/
	if (__HAL_TIM_GET_FLAG(&TIM_TimeBaseStructure, MOTOR3_TIM_FLAG_CCx) != RESET)
	{
		if (__HAL_TIM_GET_IT_SOURCE(&TIM_TimeBaseStructure, MOTOR3_TIM_IT_CCx) != RESET)
		{
			speed_decision_axis(AXIS_34);
		}
		else
		{
			__HAL_TIM_CLEAR_IT(&TIM_TimeBaseStructure, MOTOR3_TIM_IT_CCx);
		}
	}
}

/*********************************************END OF FILE**********************/