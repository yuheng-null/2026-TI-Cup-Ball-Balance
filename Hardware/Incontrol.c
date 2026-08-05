#include "stm32f10x.h"
#include "Delay.h"

/* IR remote control port definitions */
#define IRED_PORT        GPIOA
#define IRED_PIN         GPIO_Pin_8
#define IRED_PORT_RCC    RCC_APB2Periph_GPIOA

uint32_t IR_Receivecode;
uint8_t  IR_Receiveflag;

void IRremote_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(IRED_PORT_RCC | RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitStructure.GPIO_Pin = IRED_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(IRED_PORT, &GPIO_InitStructure);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource8);
    EXTI_ClearITPendingBit(EXTI_Line8);

    EXTI_InitStructure.EXTI_Line = EXTI_Line8;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

uint8_t IRremote_Counttime(void)
{
    u8 t = 0;
    while (GPIO_ReadInputDataBit(IRED_PORT, IRED_PIN) == 1)
    {
        t++;
        Delay_us(20);
        if (t >= 250) return t;
    }
    return t;
}

void EXTI9_5_IRQHandler(void)
{
    /* IR remote handler - not used in competition */
}
