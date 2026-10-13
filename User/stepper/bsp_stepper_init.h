#ifndef __BSP_STEP_MOTOR_INIT_H
#define	__BSP_STEP_MOTOR_INIT_H

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "./stepper/bsp_stepper_T_speed.h"

/*==========================================================================
  四个电机的引脚（野火F407电机板 接口1/2/3/4）
  接口1：PUL=PI5(TIM8_CH1)  DIR=PE1   EN=PE0
  接口2：PUL=PI6(TIM8_CH2)  DIR=PI8   EN=PE4
  接口3：PUL=PI7(TIM8_CH3)  DIR=PI11  EN=PI10
  接口4：PUL=PC9(TIM8_CH4)  DIR=PF2   EN=PF1
  四个脉冲脚都在TIM8上，所以只有 TIM8_CC_IRQHandler 一个中断服务函数
==========================================================================*/

/*----------------------- Motor 1 -----------------------*/
#define MOTOR1_DIR_PIN                  	GPIO_PIN_1
#define MOTOR1_DIR_GPIO_PORT            	GPIOE
#define MOTOR1_DIR_GPIO_CLK_ENABLE()   	__HAL_RCC_GPIOE_CLK_ENABLE()

#define MOTOR1_EN_PIN                  	GPIO_PIN_0
#define MOTOR1_EN_GPIO_PORT            	GPIOE
#define MOTOR1_EN_GPIO_CLK_ENABLE()    	__HAL_RCC_GPIOE_CLK_ENABLE()

#define MOTOR1_PUL_IRQn                  TIM8_CC_IRQn
#define MOTOR1_PUL_IRQHandler            TIM8_CC_IRQHandler
#define MOTOR1_PUL_TIM                   TIM8
#define MOTOR1_PUL_CLK_ENABLE()  		__TIM8_CLK_ENABLE()
#define MOTOR1_PUL_PORT       			GPIOI
#define MOTOR1_PUL_PIN             		GPIO_PIN_5
#define MOTOR1_PUL_GPIO_CLK_ENABLE()		__HAL_RCC_GPIOI_CLK_ENABLE()
#define MOTOR1_PUL_GPIO_AF               GPIO_AF3_TIM8
#define MOTOR1_PUL_CHANNEL_x             TIM_CHANNEL_1
#define MOTOR1_TIM_IT_CCx                TIM_IT_CC1
#define MOTOR1_TIM_FLAG_CCx              TIM_FLAG_CC1

/*----------------------- Motor 2 -----------------------*/
#define MOTOR2_DIR_PIN                   GPIO_PIN_8
#define MOTOR2_DIR_GPIO_PORT             GPIOI
#define MOTOR2_DIR_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOI_CLK_ENABLE()

#define MOTOR2_EN_PIN                    GPIO_PIN_4
#define MOTOR2_EN_GPIO_PORT              GPIOE
#define MOTOR2_EN_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE()

#define MOTOR2_PUL_IRQn                  TIM8_CC_IRQn
#define MOTOR2_PUL_IRQHandler            TIM8_CC_IRQHandler
#define MOTOR2_PUL_TIM                   TIM8
#define MOTOR2_PUL_CLK_ENABLE()          __TIM8_CLK_ENABLE()
#define MOTOR2_PUL_PORT                  GPIOI
#define MOTOR2_PUL_PIN                   GPIO_PIN_6
#define MOTOR2_PUL_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOI_CLK_ENABLE()
#define MOTOR2_PUL_GPIO_AF               GPIO_AF3_TIM8
#define MOTOR2_PUL_CHANNEL_x             TIM_CHANNEL_2
#define MOTOR2_TIM_IT_CCx                TIM_IT_CC2
#define MOTOR2_TIM_FLAG_CCx              TIM_FLAG_CC2

/*----------------------- Motor 3 -----------------------*/
#define MOTOR3_DIR_PIN                  	GPIO_PIN_11
#define MOTOR3_DIR_GPIO_PORT            	GPIOI
#define MOTOR3_DIR_GPIO_CLK_ENABLE()   	__HAL_RCC_GPIOI_CLK_ENABLE()

#define MOTOR3_EN_PIN                  	GPIO_PIN_10
#define MOTOR3_EN_GPIO_PORT            	GPIOI
#define MOTOR3_EN_GPIO_CLK_ENABLE()    	__HAL_RCC_GPIOI_CLK_ENABLE()

#define MOTOR3_PUL_IRQn                  TIM8_CC_IRQn
#define MOTOR3_PUL_IRQHandler            TIM8_CC_IRQHandler
#define MOTOR3_PUL_TIM                   TIM8
#define MOTOR3_PUL_CLK_ENABLE()  		__TIM8_CLK_ENABLE()
#define MOTOR3_PUL_PORT       			GPIOI
#define MOTOR3_PUL_PIN             		GPIO_PIN_7
#define MOTOR3_PUL_GPIO_CLK_ENABLE()		__HAL_RCC_GPIOI_CLK_ENABLE()
#define MOTOR3_PUL_GPIO_AF               GPIO_AF3_TIM8
#define MOTOR3_PUL_CHANNEL_x             TIM_CHANNEL_3
#define MOTOR3_TIM_IT_CCx                TIM_IT_CC3
#define MOTOR3_TIM_FLAG_CCx              TIM_FLAG_CC3

/*----------------------- Motor 4 -----------------------*/
#define MOTOR4_DIR_PIN                  	GPIO_PIN_2
#define MOTOR4_DIR_GPIO_PORT            	GPIOF
#define MOTOR4_DIR_GPIO_CLK_ENABLE()   	__HAL_RCC_GPIOF_CLK_ENABLE()

#define MOTOR4_EN_PIN                  	GPIO_PIN_1
#define MOTOR4_EN_GPIO_PORT            	GPIOF
#define MOTOR4_EN_GPIO_CLK_ENABLE()    	__HAL_RCC_GPIOF_CLK_ENABLE()

#define MOTOR4_PUL_IRQn                  TIM8_CC_IRQn
#define MOTOR4_PUL_IRQHandler            TIM8_CC_IRQHandler
#define MOTOR4_PUL_TIM                   TIM8
#define MOTOR4_PUL_CLK_ENABLE()  		__TIM8_CLK_ENABLE()
#define MOTOR4_PUL_PORT       			GPIOC
#define MOTOR4_PUL_PIN             		GPIO_PIN_9
#define MOTOR4_PUL_GPIO_CLK_ENABLE()		__HAL_RCC_GPIOC_CLK_ENABLE()
#define MOTOR4_PUL_GPIO_AF               GPIO_AF3_TIM8
#define MOTOR4_PUL_CHANNEL_x             TIM_CHANNEL_4
#define MOTOR4_TIM_IT_CCx                TIM_IT_CC4
#define MOTOR4_TIM_FLAG_CCx              TIM_FLAG_CC4

/*=================== 两个轴的分组 ===================
  轴0(AXIS_12) = 电机1 + 电机2：共用一个状态机，脉冲完全同步
  轴1(AXIS_34) = 电机3 + 电机4：共用一个状态机，脉冲完全同步
  每轴用一路通道产生中断当"节拍"，另一路每次写同样的比较值，
  两路就在同一个计数点上同时翻转，两个电机自然完全一致。
====================================================*/
#define AXIS12_PACE_CH                   MOTOR1_PUL_CHANNEL_x   /*PI5，产生中断*/
#define AXIS12_SYNC_CH                   MOTOR2_PUL_CHANNEL_x   /*PI6，跟着CH1一起翻转*/
#define AXIS34_PACE_CH                   MOTOR3_PUL_CHANNEL_x   /*PI7，产生中断*/
#define AXIS34_SYNC_CH                   MOTOR4_PUL_CHANNEL_x   /*PC9，跟着CH3一起翻转*/

/*如果某个电机装反了，把对应的宏改成1，方向就反过来了，其它代码不用动*/
#define MOTOR1_DIR_REVERSE               0
#define MOTOR2_DIR_REVERSE               0
#define MOTOR3_DIR_REVERSE               0
#define MOTOR4_DIR_REVERSE               0

/*四个通道共用一个中断：TIM8_CC_IRQHandler 写在 bsp_stepper_init.c 里，
  它负责判断是哪个轴的节拍通道到了，再分别调用 speed_decision_axis()。*/

/****************************************************************/

#define HIGH 1		//高电平
#define LOW  0		//低电平

#define ON  0			//开
#define OFF !0			//关

#define CLOCKWISE 			1//顺时针
#define ANTI_CLOCKWISE	0//逆时针

//输出比较模式周期设置为0xFFFF
#define TIM_PERIOD                   0xFFFF

//控制引脚电平
#define	GPIO_H(p,i)					{p->BSRR=i;}			  								//设置为高电平
#define GPIO_L(p,i)					{p->BSRR=(uint32_t)i << 16;}				//输出低电平
#define GPIO_T(p,i)					{p->ODR ^=i;}												//输出翻转状态

#define MOTOR1_EN(x)					if(x)																			\
														{GPIO_H(MOTOR1_EN_GPIO_PORT,MOTOR1_EN_PIN);}\
														else																			\
														{GPIO_L(MOTOR1_EN_GPIO_PORT,MOTOR1_EN_PIN);}

#define MOTOR2_EN(x)					if(x)																			\
														{GPIO_H(MOTOR2_EN_GPIO_PORT,MOTOR2_EN_PIN);}\
														else																			\
														{GPIO_L(MOTOR2_EN_GPIO_PORT,MOTOR2_EN_PIN);}

#define MOTOR3_EN(x)					if(x)																			\
														{GPIO_H(MOTOR3_EN_GPIO_PORT,MOTOR3_EN_PIN);}\
														else																			\
														{GPIO_L(MOTOR3_EN_GPIO_PORT,MOTOR3_EN_PIN);}

#define MOTOR4_EN(x)					if(x)																			\
														{GPIO_H(MOTOR4_EN_GPIO_PORT,MOTOR4_EN_PIN);}\
														else																			\
														{GPIO_L(MOTOR4_EN_GPIO_PORT,MOTOR4_EN_PIN);}

#define MOTOR1_DIR(x)					if(x)																			\
														{GPIO_H(MOTOR1_DIR_GPIO_PORT,MOTOR1_DIR_PIN);}\
                            else																				\
                            {GPIO_L(MOTOR1_DIR_GPIO_PORT,MOTOR1_DIR_PIN);}

#define MOTOR2_DIR(x)					if(x)																			\
														{GPIO_H(MOTOR2_DIR_GPIO_PORT,MOTOR2_DIR_PIN);}\
                            else																				\
                            {GPIO_L(MOTOR2_DIR_GPIO_PORT,MOTOR2_DIR_PIN);}

#define MOTOR3_DIR(x)					if(x)																			\
														{GPIO_H(MOTOR3_DIR_GPIO_PORT,MOTOR3_DIR_PIN);}\
                            else																				\
                            {GPIO_L(MOTOR3_DIR_GPIO_PORT,MOTOR3_DIR_PIN);}

#define MOTOR4_DIR(x)					if(x)																			\
														{GPIO_H(MOTOR4_DIR_GPIO_PORT,MOTOR4_DIR_PIN);}\
                            else																				\
                            {GPIO_L(MOTOR4_DIR_GPIO_PORT,MOTOR4_DIR_PIN);}

/*函数以及变量声明*/
extern TIM_HandleTypeDef TIM_TimeBaseStructure;
void stepper_Init(void);

#endif