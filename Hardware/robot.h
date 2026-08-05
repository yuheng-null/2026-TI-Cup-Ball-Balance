#ifndef __ROBOT_H
#define __ROBOT_H

void robot_Init(void);
void robot_speed(uint8_t left1_speed, uint8_t left2_speed, uint8_t right1_speed, uint8_t right2_speed);
// 机器人运动
void makerobo_run(uint8_t speed, uint16_t time);       // 机器人前进
void makerobo_brake(uint16_t time);                    // 机器人停止
void makerobo_Left(int8_t speed, uint16_t time);       // 左转
void makerobo_Spin_Left(int8_t speed, uint16_t time);  // 原地左转
void makerobo_Right(int8_t speed, uint16_t time);      // 右转
void makerobo_Spin_Right(int8_t speed, uint16_t time); // 原地右转
void makerobo_back(int8_t speed, uint16_t time);       // 机器人后退
#endif
