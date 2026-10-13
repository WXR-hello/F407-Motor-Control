#ifndef __BSP_STEPPER_T_SPEED_H
#define	__BSP_STEPPER_T_SPEED_H

#include "stm32f4xx.h"
#include "./stepper/bsp_stepper_init.h"
#include "math.h"

/*梯形加减速相关变量*/
typedef struct 
{
	/*当前电机状态*/
	uint8_t  run_state ; 
	/*旋转方向*/
	uint8_t  dir ;    
	/*脉冲间隔*/
	int  step_delay;  
	/*减速位置*/
	int  decel_start; 
	/*减速步数*/
	int  decel_val;   
	/*最小间隔*/
	int  min_delay;   
	/*加速步数*/
	int  accel_count; 
}speedRampData;

#define ACCEL_R(x)	ceil(x)//向上取整
#define DECEL_R(x)	floor(x)//向下取整

#define FALSE             0
#define TRUE              1
#define CW                0 // 顺时针
#define CCW               1 // 逆时针

/*电机速度决策中的四个状态*/
#define STOP              0 // 停止状态
#define ACCEL             1 // 加速状态
#define DECEL             2 // 减速状态
#define RUN               3 // 匀速状态

/*======================== 定时器频率 ========================
  TIM8挂在APB2上，时钟是168MHz。分频后：
      定时器实际计数频率 = 168MHz / TIM_PRESCALER
  初始化时写进TIM8分频寄存器的是 TIM_PRESCALER-1，
  和这里的 T1_FREQ 刚好对得上，算出来的速度才是准的。

  为什么从原来的5改成28？
  原来初始化写的是5-1=4，实际时钟是 168/5 = 33.6MHz，
  而这里算的是 168/(5+1) = 28MHz，两个数差了20%，
  也就是实际速度比设定值快20%，调参时会误导人。
  改成28以后两边都是 168/28 = 6MHz，完全对齐。

  为什么不用更细的28MHz？因为比较寄存器只有16位，
  半周期超过65535拍就会被硬件截断，起步第一拍的周期
  会比曲线算出来的短3倍多，恰恰是最容易丢步的地方。
  6MHz下起步周期约56834拍，半周期28417，落在范围内。
===========================================================*/
#define TIM_PRESCALER     28
#define T1_FREQ           (SystemCoreClock/TIM_PRESCALER) // 频率ft值

/*=================== 防止丢步的两道软件保护 ===================
  MIN_DELAY_LIMIT：脉冲周期下限，6MHz下120拍=20us，
                   最高约50kpps。就算速度参数填得过大，
                   也不会把脉冲发得超出电机能力。
  HALF_DELAY_MAX ：半周期上限。比较值只能写16位，超过65535
                   会被截断，把周期卡在范围内，加减速曲线才不会变形。
=============================================================*/
#define MIN_DELAY_LIMIT   120U
#define HALF_DELAY_MAX    60000U

/*电机单圈参数*/
#define STEP_ANGLE				1.8									//步进电机的步距角 单位：度
#define FSPR              (360.0/1.8)         //步进电机的一圈所需脉冲数

#define MICRO_STEP        32          				//细分器细分数，必须和驱动器拨码一致
#define SPR               (FSPR*MICRO_STEP)   //细分后一圈所需脉冲数 = 6400

/*数学常数,用于简化计算*/
#define ALPHA             ((float)(2*3.14159/SPR))       // α= 2*pi/spr
#define A_T_x10           ((float)(10*ALPHA*T1_FREQ))
#define T1_FREQ_148       ((float)((T1_FREQ*0.676)/10)) // 0.69为误差修正值
#define A_SQ              ((float)(2*100000*ALPHA)) 
#define A_x200            ((float)(200*ALPHA)) 

/*======================== 两个轴 ========================
  轴0(AXIS_12) = 电机1 + 电机2
  轴1(AXIS_34) = 电机3 + 电机4
  同一个轴里的两个电机由同一套状态机驱动，脉冲完全同步；
  两个轴之间互相独立，可以各走各的距离和速度。
========================================================*/
#define AXIS_12           0
#define AXIS_34           1
#define AXIS_NUM          2

/*每个轴的运行状态*/
typedef struct
{
	speedRampData ramp;             /*加减速参数与状态机*/
	uint32_t      step_count;       /*已经发出的脉冲数*/
	int32_t       rest;             /*除法余数，减小累计误差*/
	uint16_t      last_accel_delay; /*减速起点的脉冲周期*/
	uint8_t       toggle;           /*翻转计数：2次中断=1个完整脉冲*/
	uint8_t       running;          /*1=该轴正在运动*/
}stepperAxis;

extern stepperAxis axis[AXIS_NUM];

/*----------------------- 对外接口 -----------------------*/
/*让某个轴（两个电机）走指定的步数，正数一个方向，负数反向*/
void     stepper_move_T(uint8_t axis_no, int32_t step, uint32_t accel, uint32_t decel, uint32_t speed);
/*立即停止某个轴*/
void     stepper_stop(uint8_t axis_no);
/*立即停止两个轴*/
void     stepper_stop_all(void);
/*该轴是否正在运动：1=运动中，0=已停*/
uint8_t  stepper_is_running(uint8_t axis_no);
/*上一趟实际发出去的脉冲数（急停时可以用来算还剩多少）*/
uint32_t stepper_get_steps(uint8_t axis_no);
/*速度决策：中断服务函数里按轴调用*/
void     speed_decision_axis(uint8_t ax);

#endif