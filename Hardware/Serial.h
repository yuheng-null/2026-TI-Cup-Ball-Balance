#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"
#include <stdio.h>

typedef enum
{
    OPENMV_TARGET_STATE_IDLE = 0,
    OPENMV_TARGET_STATE_NORMAL,
    OPENMV_TARGET_STATE_SHORT_LOST,
    OPENMV_TARGET_STATE_DIRECTION_LOST,
    OPENMV_TARGET_STATE_EMPTY_LOST,
    OPENMV_TARGET_STATE_INVALID
} OpenMV_TargetState_t;

typedef struct
{
    int16_t distance_mm;
    int8_t direction;
    OpenMV_TargetState_t state;
    uint8_t valid;
    uint8_t exact;
    uint32_t frame_count;
} OpenMV_Target_t;

void Serial_Init(void);
void Serial_Task(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);

uint8_t Serial_GetRxFlag(void);
uint8_t Serial_GetRxData(void);
uint8_t Serial_ReadByte(uint8_t *byte_out);
void Serial_ClearRxBuffer(void);

int16_t OpenMV_GetDistance(void);
int8_t OpenMV_GetDirection(void);
OpenMV_TargetState_t OpenMV_GetTargetState(void);
uint8_t OpenMV_IsTargetValid(void);
uint8_t OpenMV_IsTargetExact(void);
uint32_t OpenMV_GetFrameCount(void);
void OpenMV_GetTarget(OpenMV_Target_t *target);

#endif
