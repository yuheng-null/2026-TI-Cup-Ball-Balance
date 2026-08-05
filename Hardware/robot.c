#include "stm32f10x.h" // Device header
#include "PWM.h"
#include "Delay.h"
// 机器人初始化
void robot_Init(void)
{
	PWM_Init();
}

// 4路PWM控制速度调节
void robot_speed(uint8_t left1_speed, uint8_t left2_speed, uint8_t right1_speed, uint8_t right2_speed)
{
	TIM_SetCompare1(TIM4, left1_speed);
	TIM_SetCompare2(TIM4, left2_speed);
	TIM_SetCompare3(TIM4, right1_speed);
	TIM_SetCompare4(TIM4, right2_speed);
}

// 机器人运动控制
// 机器人前进
void makerobo_run(int8_t speed, uint16_t time) // 前进控制
{
	if (speed > 100)
	{
		speed = 100;
	}
	if (speed < 0)
	{
		speed = 0;
	}
	robot_speed(speed, 0, speed, 0);
	Delay_ms(time); // 时间为延时
					// robot_speed(0,0,0,0);           // 机器人停止
}

void makerobo_brake(uint16_t time) // 刹车控制
{
	robot_speed(0, 0, 0, 0); // 立即停止
	Delay_ms(time);			 // 时间为延时
}

void makerobo_Left(int8_t speed, uint16_t time) // 左转控制
{
	if (speed > 100)
	{
		speed = 100;
	}
	if (speed < 0)
	{
		speed = 0;
	}
	robot_speed(0, 0, speed, 0);
	Delay_ms(time); // 时间为延时
					// robot_speed(0,0,0,0);           // 机器人停止
}

void makerobo_Spin_Left(int8_t speed, uint16_t time) // 原地左转控制
{
	if (speed > 100)
	{
		speed = 100;
	}
	if (speed < 0)
	{
		speed = 0;
	}
	robot_speed(0, speed, speed, 0);
	Delay_ms(time); // 时间为延时
	// robot_speed(0,0,0,0);           // 机器人停止
}

void makerobo_Right(int8_t speed, uint16_t time) // 右转控制
{
	if (speed > 100)
	{
		speed = 100;
	}
	if (speed < 0)
	{
		speed = 0;
	}
	robot_speed(speed, 0, 0, 0);
	Delay_ms(time); // 时间为延时
					// robot_speed(0,0,0,0);           // 机器人停止
}

void makerobo_Spin_Right(int8_t speed, uint16_t time) // 原地右转控制
{
	if (speed > 100)
	{
		speed = 100;
	}
	if (speed < 0)
	{
		speed = 0;
	}
	robot_speed(speed, 0, 0, speed);
	Delay_ms(time); // 时间为延时
	// robot_speed(0,0,0,0);           // 机器人停止
}

void makerobo_back(int8_t speed, uint16_t time) // 机器人后退
{
	if (speed > 100)
	{
		speed = 100;
	}
	if (speed < 0)
	{
		speed = 0;
	}
	robot_speed(0, speed, 0, speed);
	Delay_ms(time); // 时间为延时
					// robot_speed(0,0,0,0);           // 机器人停止
}
