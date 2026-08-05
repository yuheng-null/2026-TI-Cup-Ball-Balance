#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f10x.h"

#define SERVO_PULSE_MIN_US 500U
#define SERVO_PULSE_MAX_US 2500U

void Servo_Init(void);
void Servo_SetAngle(uint8_t angle);
void Servo_SetPulseUs(uint16_t pulse_us);
uint16_t Servo_GetPulseUs(void);

#endif
