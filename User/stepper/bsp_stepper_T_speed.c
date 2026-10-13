/**
  ******************************************************************************
  * @file    bsp_stepper_T_speed.c
  * @brief   两轴梯形加减速，每个轴带两个完全同步的电机
  *          轴0(AXIS_12)：电机1 + 电机2
  *          轴1(AXIS_34)：电机3 + 电机4
  * @note    四个脉冲脚都在TIM8上，共用一个计数器和一个中断。
  *          每个轴用一路通道当"节拍"产生中断，另一路每次跟着写
  *          同样的比较值，两路就会在同一个计数点上同时翻转。
  *          脉冲数由"减速段步数"和"开始减速的位置"严格凑出来，
  *          发够指定的步数就停，不多不少。
  ******************************************************************************
  */
#include "./stepper/bsp_stepper_T_speed.h"

/*两个轴的全部状态*/
stepperAxis axis[AXIS_NUM];

/*每个轴用到的：节拍通道、同步通道、节拍中断源*/
static const uint32_t axis_pace_ch[AXIS_NUM] = { AXIS12_PACE_CH, AXIS34_PACE_CH };
static const uint32_t axis_sync_ch[AXIS_NUM] = { AXIS12_SYNC_CH, AXIS34_SYNC_CH };
static const uint32_t axis_it[AXIS_NUM]      = { MOTOR1_TIM_IT_CCx, MOTOR3_TIM_IT_CCx };

/**
  * @brief  给一个轴里的两个电机同时设定方向
  */
static void axis_set_dir(uint8_t ax, uint8_t dir)
{
	if (ax == AXIS_12)
	{
		MOTOR1_DIR((uint8_t)(dir ^ MOTOR1_DIR_REVERSE));
		MOTOR2_DIR((uint8_t)(dir ^ MOTOR2_DIR_REVERSE));
	}
	else
	{
		MOTOR3_DIR((uint8_t)(dir ^ MOTOR3_DIR_REVERSE));
		MOTOR4_DIR((uint8_t)(dir ^ MOTOR4_DIR_REVERSE));
	}
}

/**
  * @brief  把一个轴的两路通道写成同一个比较值
  * @note   两路值一样，就会在同一个计数点上同时翻转，两个电机完全同步。
  *         先写同步通道再写节拍通道，只要 value 离当前计数值够远
  *         （大于十几拍），就不会出现一路漏掉这次比较的情况。
  */
static void axis_set_compare(uint8_t ax, uint32_t value)
{
	__HAL_TIM_SET_COMPARE(&TIM_TimeBaseStructure, axis_sync_ch[ax], value);
	__HAL_TIM_SET_COMPARE(&TIM_TimeBaseStructure, axis_pace_ch[ax], value);
}

/**
  * @brief  同时开关一个轴的两路输出通道
  */
static void axis_channel(uint8_t ax, uint16_t state)
{
	TIM_CCxChannelCmd(MOTOR1_PUL_TIM, axis_pace_ch[ax], state);
	TIM_CCxChannelCmd(MOTOR1_PUL_TIM, axis_sync_ch[ax], state);
}

/**
  * @brief  一个轴的速度决策，每进一次该轴的CC中断调用一次
  * @param  ax  AXIS_12 或 AXIS_34
  */
void speed_decision_axis(uint8_t ax)
{
	stepperAxis   *a   = &axis[ax];
	speedRampData *srd = &a->ramp;
	uint32_t tim_count;
	uint32_t tmp;
	/* 保存新（下）一个延时周期 */
	uint16_t new_step_delay = 0;

	/*清除本通道的中断标志*/
	__HAL_TIM_CLEAR_IT(&TIM_TimeBaseStructure, axis_it[ax]);

	/*先按"当前"的脉冲周期设定下一次比较值*/
	tim_count = __HAL_TIM_GET_COUNTER(&TIM_TimeBaseStructure);
	tmp       = tim_count + srd->step_delay/2;
	axis_set_compare(ax, tmp);

	a->toggle++;                    /*定时器翻转模式，两次中断才凑出一个完整脉冲*/
	if (a->toggle == 2)
	{
		a->toggle = 0;

		switch (srd->run_state)
		{
			/*---------- 停止 ----------*/
			case STOP:
				a->rest = 0;
				axis_channel(ax, TIM_CCx_DISABLE);                          /*关输出*/
				__HAL_TIM_DISABLE_IT(&TIM_TimeBaseStructure, axis_it[ax]);  /*关本轴中断*/
				a->running = 0;                                             /*step_count保留，方便外部读取*/
				break;

			/*---------- 加速 ----------*/
			case ACCEL:
				axis_channel(ax, TIM_CCx_ENABLE);
				a->step_count++;
				srd->accel_count++;

				/*计算新(下)一步脉冲周期(时间间隔)*/
				new_step_delay = srd->step_delay - (((2 * srd->step_delay) + a->rest)/(4 * srd->accel_count + 1));
				/*计算余数，下次计算补上，减少误差*/
				a->rest = ((2 * srd->step_delay) + a->rest) % (4 * srd->accel_count + 1);

				/*检查是否该开始减速*/
				if (a->step_count >= srd->decel_start)
				{
					srd->accel_count = srd->decel_val;
					srd->run_state   = DECEL;
				}
				/*检查是否到达期望的最大速度*/
				else if (new_step_delay <= srd->min_delay)
				{
					a->last_accel_delay = new_step_delay;
					new_step_delay      = srd->min_delay;
					a->rest             = 0;
					srd->run_state      = RUN;
				}
				break;

			/*---------- 匀速 ----------*/
			case RUN:
				a->step_count++;
				new_step_delay = srd->min_delay;

				/*检查是否需要开始减速*/
				if (a->step_count >= srd->decel_start)
				{
					srd->accel_count = srd->decel_val;
					new_step_delay   = a->last_accel_delay;
					srd->run_state   = DECEL;
				}
				break;

			/*---------- 减速 ----------*/
			case DECEL:
				a->step_count++;
				srd->accel_count++;

				new_step_delay = srd->step_delay - (((2 * srd->step_delay) + a->rest)/(4 * srd->accel_count + 1));
				a->rest = ((2 * srd->step_delay) + a->rest) % (4 * srd->accel_count + 1);

				/*accel_count从负数加到0，正好把该发的脉冲发完*/
				if (srd->accel_count >= 0)
				{
					srd->run_state = STOP;
				}
				break;

			default:
				break;
		}

		/*求得下一次间隔时间*/
		srd->step_delay = new_step_delay;
	}
}

/**
  * @brief  让某个轴走指定的步数
  * @param  axis_no  AXIS_12（电机1+2）或 AXIS_34（电机3+4）
  * @param  step     步数，正数朝正方向，负数朝反方向
  * @param  accel    加速度，取值为10表示 10*0.1rad/sec^2 = 1rad/sec^2
  * @param  decel    减速度，同上
  * @param  speed    最大速度，取值为10表示 10*0.1rad/sec = 1rad/sec
  * @note   函数只负责装填参数并返回，真正的脉冲在中断里发，不阻塞主循环
  */
void stepper_move_T(uint8_t axis_no, int32_t step, uint32_t accel, uint32_t decel, uint32_t speed)
{
	stepperAxis   *a   = &axis[axis_no];
	speedRampData *srd = &a->ramp;
	/*达到最大速度时的步数*/
	unsigned int max_s_lim;
	/*必须开始减速的步数*/
	unsigned int accel_lim;

	if (axis_no >= AXIS_NUM) return;
	if (step == 0)           return;
	if (a->running)          return;      /*正在运动就先不接受新命令*/
	if (accel == 0)  accel = 1;
	if (decel == 0)  decel = 1;
	if (speed == 0)  speed = 1;

	/*根据步数和正负判断方向*/
	if (step < 0)
	{
		srd->dir = CCW;
		step = -step;
	}
	else
	{
		srd->dir = CW;
	}
	axis_set_dir(axis_no, srd->dir);      /*一个轴里的两个电机同时给方向*/

	/*如果只移动一步*/
	if (step == 1)
	{
		srd->accel_count = -1;
		srd->run_state   = DECEL;
		srd->step_delay  = 1000;
	}
	/*步数不为零才移动*/
	else
	{
		/*设置最大速度极限，min_delay用于定时器的计数器的值*/
		srd->min_delay  = (int32_t)(A_T_x10/speed);

		/*防丢步保护1：脉冲周期不许比MIN_DELAY_LIMIT更短，
		  这样即使速度填得过大，也不会把脉冲发得超出电机能力*/
		if (srd->min_delay < (int)MIN_DELAY_LIMIT)
		{
			srd->min_delay = (int)MIN_DELAY_LIMIT;
		}

		/*通过计算第一个脉冲的延时来设定加速度*/
		srd->step_delay = (int32_t)((T1_FREQ_148 * sqrt(A_SQ / accel))/10);

		/*防丢步保护2：起步周期不许超过半周期上限的两倍，
		  否则会被16位比较寄存器截断，起步第一拍就会比曲线快*/
		if (srd->step_delay > (int)(2*HALF_DELAY_MAX))
		{
			srd->step_delay = (int)(2*HALF_DELAY_MAX);
		}

		/*计算多少步之后达到最大速度的限制*/
		max_s_lim = (uint32_t)(speed*speed/(A_x200*accel/10));
		if (max_s_lim == 0)
		{
			max_s_lim = 1;
		}

		/*计算多少步之后必须开始减速*/
		accel_lim = (uint32_t)(step*decel/(accel+decel));
		if (accel_lim == 0)
		{
			accel_lim = 1;
		}

		if (accel_lim <= max_s_lim)
		{
			srd->decel_val = accel_lim - step;            /*decel_val为负数*/
		}
		else
		{
			srd->decel_val = -(max_s_lim*accel/decel);
		}
		if (srd->decel_val == 0)
		{
			srd->decel_val = -1;
		}

		/*计算开始减速时的步数：加速段 + 减速段 正好等于总步数*/
		srd->decel_start = step + srd->decel_val;

		/*如果最大速度很慢，就不需要加速*/
		if (srd->step_delay <= srd->min_delay)
		{
			srd->step_delay = srd->min_delay;
			srd->run_state  = RUN;
		}
		else
		{
			srd->run_state  = ACCEL;
		}
		srd->accel_count = 0;
	}

	/*复位本轴的计数*/
	a->step_count       = 0;
	a->rest             = 0;
	a->toggle           = 0;
	a->last_accel_delay = 0;
	a->running          = 1;

	/*通道先关掉，等状态机进ACCEL时再把两路一起打开*/
	axis_channel(axis_no, TIM_CCx_DISABLE);

	/*设定第一次比较值（计数器在stepper_Init里已经常开）*/
	axis_set_compare(axis_no, __HAL_TIM_GET_COUNTER(&TIM_TimeBaseStructure) + srd->step_delay/2);

	/*只使能本轴的节拍中断，另一个轴的中断不受影响*/
	__HAL_TIM_ENABLE_IT(&TIM_TimeBaseStructure, axis_it[axis_no]);
}

/**
  * @brief  立即停止某个轴（急停）
  * @note   先关中断再关通道，避免复位过程中被中断打断
  */
void stepper_stop(uint8_t axis_no)
{
	stepperAxis *a;

	if (axis_no >= AXIS_NUM) return;
	a = &axis[axis_no];

	__HAL_TIM_DISABLE_IT(&TIM_TimeBaseStructure, axis_it[axis_no]);
	axis_channel(axis_no, TIM_CCx_DISABLE);

	a->ramp.run_state   = STOP;
	a->ramp.accel_count = 0;
	a->rest             = 0;
	a->last_accel_delay = 0;
	a->toggle           = 0;
	a->running          = 0;
}

/**
  * @brief  两个轴一起急停
  */
void stepper_stop_all(void)
{
	stepper_stop(AXIS_12);
	stepper_stop(AXIS_34);
}

/**
  * @brief  该轴是否正在运动
  */
uint8_t stepper_is_running(uint8_t axis_no)
{
	if (axis_no >= AXIS_NUM) return 0;
	return axis[axis_no].running;
}

/**
  * @brief  该轴已经发出了多少个脉冲
  * @note   正常走完时是总步数；急停时是已经走掉的那部分
  */
uint32_t stepper_get_steps(uint8_t axis_no)
{
	if (axis_no >= AXIS_NUM) return 0;
	return axis[axis_no].step_count;
}

/*********************************************END OF FILE**********************/