#include "Servo.h"

static uint16_t g_ServoPulseUs = 1500;

/**
 * @brief  MG996R舵机初始化
 * @note   使用TIM3_CH2，通过部分重映射到PB5引脚
 *         PWM频率50Hz，脉宽500us~2500us对应常见控制范围
 *         定时器时钟72MHz，预分频72-1=1MHz，周期20000-1=20ms
 */
void Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // 使能TIM3时钟（APB1）和GPIOB、AFIO时钟（APB2）
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    // 配置PB5为复用推挽输出（TIM3_CH2部分重映射）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // TIM3部分重映射：CH1->PB4, CH2->PB5, CH3->PB0, CH4->PB1
    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);

    // 时基配置：50Hz PWM
    // 定时器时钟 = 72MHz
    // 预分频 = 72-1 → 1MHz (1us per tick)
    // 周期 = 20000-1 → 20ms (50Hz)
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;
    TIM_TimeBaseStructure.TIM_Period = 20000 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // PWM模式配置（CH2）
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = g_ServoPulseUs; // 初始中位
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);

    // 使能预装载
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);

    // 启动TIM3
    TIM_Cmd(TIM3, ENABLE);
}

/**
 * @brief  设置舵机角度
 * @param  angle: 目标角度 0°~180°
 * @note   0°对应500us脉宽，180°对应2500us脉宽
 */
void Servo_SetAngle(uint8_t angle)
{
    uint16_t pulse;
    if (angle > 180)
    {
        angle = 180;
    }
    // 角度到脉宽映射：pulse = 500 + angle * (2000 / 180)
    pulse = 500 + (uint16_t)((uint32_t)angle * 2000 / 180);
    Servo_SetPulseUs(pulse);
}

void Servo_SetPulseUs(uint16_t pulse_us)
{
    if (pulse_us < SERVO_PULSE_MIN_US)
    {
        pulse_us = SERVO_PULSE_MIN_US;
    }
    if (pulse_us > SERVO_PULSE_MAX_US)
    {
        pulse_us = SERVO_PULSE_MAX_US;
    }

    g_ServoPulseUs = pulse_us;
    TIM_SetCompare2(TIM3, g_ServoPulseUs);
}

uint16_t Servo_GetPulseUs(void)
{
    return g_ServoPulseUs;
}
