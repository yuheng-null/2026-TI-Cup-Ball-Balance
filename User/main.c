/**
 * @file    main.c
 * @brief   2026 电赛 H 题 —— 车载平衡滚球运动控制系统
 * @note    主控逻辑：
 *          - 静止预演模式：钢球 0 → +5cm → -5cm 闭环控制
 *          - 循迹保持模式：钢球锁定在用户指定位置，同步循迹计时
 *          - OpenMV 协议解析与丢失恢复策略
 *          - 双 PID（整数定点）独立调参
 * @date    2026-08
 ******************************************************************************/

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "Irtracking.h"
#include "Key.h"
#include "LEDSEG.h"
#include "OLED.h"
#include "Serial.h"
#include "Servo.h"
#include "robot.h"
#include "Delay.h"
#include <string.h>

volatile uint8_t g_TrackRunning = 0;
volatile uint32_t g_TrackTimeMs = 0;

#define TRACK_SPEED 56
#define TRACK_TURN_SPEED 56
#define STOP_LINE_ARM_DELAY_MS 350U
#define STOP_LINE_CONFIRM_MS 5U
#define SERVO_CENTER_PULSE_US 1500U
#define SERVO_SAFE_ANGLE_DEG 20
#define SERVO_SAFE_PULSE_DELTA_US ((SERVO_SAFE_ANGLE_DEG * 2000) / 180)

#define PREVIEW_CTRL_PERIOD_MS 20U
#define PREVIEW_OLED_PERIOD_MS 100U
#define PREVIEW_START_HOLD_MS 800U
#define PREVIEW_TARGET_HOLD_MS 1000U
#define PREVIEW_PHASE_MAX_MS 3000U
#define PREVIEW_LOSS_RECOVER_MS 1200U
#define PREVIEW_TARGET_TOL_MM 10
#define PREVIEW_TARGET_POS_MM 50
#define PREVIEW_TARGET_NEG_MM (-44)
#define PREVIEW_EDGE_MM 45

/* 方向修正：若控制方向反了，把 1 改成 -1 */
#define PREVIEW_SERVO_DIR 1

/* 整数PID参数（静止预演模式），除以 PREVIEW_PID_SCALE 后生效 */
#define PREVIEW_PID_SCALE 100
#define PREVIEW_KP 80
#define PREVIEW_KI 0
#define PREVIEW_KD 200
#define PREVIEW_INT_CLAMP 3000

/* 整数PID参数（循迹保持模式），单独调参 */
#define TRACK_BALL_PID_SCALE 100
#define TRACK_BALL_KP 70
#define TRACK_BALL_KI 0
#define TRACK_BALL_KD 150
#define TRACK_BALL_INT_CLAMP 3000

typedef enum
{
    PREVIEW_PHASE_WAIT_BALL = 0,
    PREVIEW_PHASE_ZERO_HOLD,
    PREVIEW_PHASE_MOVE_POS,
    PREVIEW_PHASE_MOVE_NEG,
    PREVIEW_PHASE_HOLD_NEG
} PreviewPhase_t;

typedef struct
{
    int16_t target_mm;
    int16_t ramp_target_mm;
    int16_t measured_mm;
    int16_t error_mm;
    int16_t prev_error_mm;
    int32_t integral;
    int16_t pulse_delta_us;
    uint16_t pulse_cmd_us;
    uint16_t phase_elapsed_ms;
    uint16_t stable_elapsed_ms;
    uint16_t oled_elapsed_ms;
    uint16_t loss_elapsed_ms;
    PreviewPhase_t phase;
    uint8_t has_measurement;
    uint8_t has_last_measurement;
    int16_t last_measurement_mm;
    OpenMV_TargetState_t rx_state;
    uint8_t rx_valid;
} PreviewControl_t;

static uint8_t g_StopLineArmed = 0;
static uint32_t g_StopLineCandidateStartMs = 0;

static uint16_t Servo_ClampPulse(uint16_t pulse)
{
    if (pulse < SERVO_PULSE_MIN_US)
    {
        return SERVO_PULSE_MIN_US;
    }
    if (pulse > SERVO_PULSE_MAX_US)
    {
        return SERVO_PULSE_MAX_US;
    }
    return pulse;
}

static int16_t AbsI16(int16_t value)
{
    return (value >= 0) ? value : (int16_t)(-value);
}

static const char *Preview_StateText(const PreviewControl_t *preview)
{
    if (preview->rx_state == OPENMV_TARGET_STATE_NORMAL)
    {
        return "normal";
    }

    if (preview->rx_state == OPENMV_TARGET_STATE_DIRECTION_LOST || preview->rx_state == OPENMV_TARGET_STATE_SHORT_LOST)
    {
        if (preview->has_last_measurement && preview->last_measurement_mm < 0)
        {
            return "right lost";
        }

        if (preview->has_last_measurement && preview->last_measurement_mm > 0)
        {
            return "left lost";
        }

        return "lost";
    }

    if (preview->rx_state == OPENMV_TARGET_STATE_EMPTY_LOST)
    {
        return "none";
    }

    if (preview->rx_state == OPENMV_TARGET_STATE_INVALID)
    {
        return "none";
    }

    return "none";
}

static void Preview_Init(PreviewControl_t *preview)
{
    memset(preview, 0, sizeof(*preview));
    preview->phase = PREVIEW_PHASE_WAIT_BALL;
    preview->target_mm = 0;
    preview->ramp_target_mm = 0;
    preview->pulse_cmd_us = SERVO_CENTER_PULSE_US;
    preview->rx_state = OPENMV_TARGET_STATE_IDLE;
    preview->rx_valid = 0;
}

static void TrackBall_Init(PreviewControl_t *track_ball)
{
    Preview_Init(track_ball);
    track_ball->phase = PREVIEW_PHASE_HOLD_NEG;
    track_ball->target_mm = 0;
}

static void Preview_UpdateMeasurement(PreviewControl_t *preview)
{
    OpenMV_Target_t target;
    int16_t measured;

    OpenMV_GetTarget(&target);
    preview->rx_state = target.state;
    preview->rx_valid = target.valid;

    if (target.state == OPENMV_TARGET_STATE_NORMAL || target.state == OPENMV_TARGET_STATE_SHORT_LOST)
    {
        measured = (target.direction >= 0) ? target.distance_mm : (int16_t)(-target.distance_mm);
        preview->measured_mm = measured;
        preview->has_measurement = 1;
        preview->has_last_measurement = 1;
        preview->last_measurement_mm = measured;
        preview->loss_elapsed_ms = 0;
        return;
    }

    if (target.state == OPENMV_TARGET_STATE_DIRECTION_LOST)
    {
        measured = (target.direction >= 0) ? PREVIEW_EDGE_MM : (int16_t)(-PREVIEW_EDGE_MM);
        preview->measured_mm = measured;
        preview->has_measurement = 1;
        preview->has_last_measurement = 1;
        preview->last_measurement_mm = measured;
        preview->loss_elapsed_ms = 0;
        return;
    }

    preview->loss_elapsed_ms += PREVIEW_CTRL_PERIOD_MS;
    if (preview->has_last_measurement && preview->loss_elapsed_ms <= PREVIEW_LOSS_RECOVER_MS)
    {
        /* 先按最后一次可信坐标继续补偿，争取把球拉回视野内 */
        preview->measured_mm = preview->last_measurement_mm;
        preview->has_measurement = 1;
        return;
    }

    /* 超时后立即退回水平位置，避免盲调舵机 */
    preview->rx_state = OPENMV_TARGET_STATE_EMPTY_LOST;
    preview->has_measurement = 0;
}

static void Preview_UpdateTarget(PreviewControl_t *preview)
{
    int16_t absError = AbsI16(preview->error_mm);

    if (preview->phase == PREVIEW_PHASE_WAIT_BALL)
    {
        preview->target_mm = 0;
        preview->ramp_target_mm = 0;
        preview->phase_elapsed_ms = 0;
        preview->stable_elapsed_ms = 0;
        if (preview->has_measurement)
        {
            preview->phase = PREVIEW_PHASE_ZERO_HOLD;
            preview->target_mm = 0;
            preview->ramp_target_mm = preview->measured_mm;
            preview->integral = 0;
            preview->prev_error_mm = 0;
            preview->phase_elapsed_ms = 0;
            preview->stable_elapsed_ms = 0;
            Serial_SendString("STEP4 PHASE: BALL DETECTED, START PREVIEW\r\n");
        }
        return;
    }

    if (preview->has_measurement == 0)
    {
        preview->target_mm = 0;
        preview->ramp_target_mm = 0;
        preview->phase = PREVIEW_PHASE_ZERO_HOLD;
        preview->phase_elapsed_ms = 0;
        preview->stable_elapsed_ms = 0;
        return;
    }

    /* 目标斜坡：每次只向最终目标靠近一小步，防止PID看到巨大瞬时误差 */
    if (preview->ramp_target_mm < preview->target_mm)
    {
        preview->ramp_target_mm += 3;
        if (preview->ramp_target_mm > preview->target_mm)
        {
            preview->ramp_target_mm = preview->target_mm;
        }
    }
    else if (preview->ramp_target_mm > preview->target_mm)
    {
        preview->ramp_target_mm -= 3;
        if (preview->ramp_target_mm < preview->target_mm)
        {
            preview->ramp_target_mm = preview->target_mm;
        }
    }

    preview->phase_elapsed_ms += PREVIEW_CTRL_PERIOD_MS;

    if (absError <= PREVIEW_TARGET_TOL_MM)
    {
        preview->stable_elapsed_ms += PREVIEW_CTRL_PERIOD_MS;
    }
    else
    {
        preview->stable_elapsed_ms = 0;
    }

    if (preview->phase == PREVIEW_PHASE_ZERO_HOLD)
    {
        preview->target_mm = 0;
        if (preview->phase_elapsed_ms >= PREVIEW_START_HOLD_MS)
        {
            preview->phase = PREVIEW_PHASE_MOVE_POS;
            preview->target_mm = PREVIEW_TARGET_POS_MM;
            preview->ramp_target_mm = preview->measured_mm;
            preview->integral = 0;
            preview->prev_error_mm = 0;
            preview->phase_elapsed_ms = 0;
            preview->stable_elapsed_ms = 0;
            Serial_SendString("STEP4 PHASE: MOVE +50mm\r\n");
        }
        return;
    }

    if (preview->phase == PREVIEW_PHASE_MOVE_POS)
    {
        preview->target_mm = PREVIEW_TARGET_POS_MM;
        if (preview->stable_elapsed_ms >= PREVIEW_TARGET_HOLD_MS || preview->phase_elapsed_ms >= PREVIEW_PHASE_MAX_MS)
        {
            preview->phase = PREVIEW_PHASE_MOVE_NEG;
            preview->target_mm = PREVIEW_TARGET_NEG_MM;
            preview->ramp_target_mm = preview->measured_mm;
            preview->integral = 0;
            preview->prev_error_mm = 0;
            preview->phase_elapsed_ms = 0;
            preview->stable_elapsed_ms = 0;
            Serial_SendString("STEP4 PHASE: MOVE -44mm\r\n");
        }
        return;
    }

    if (preview->phase == PREVIEW_PHASE_MOVE_NEG)
    {
        preview->target_mm = PREVIEW_TARGET_NEG_MM;
        if (preview->stable_elapsed_ms >= PREVIEW_TARGET_HOLD_MS || preview->phase_elapsed_ms >= PREVIEW_PHASE_MAX_MS)
        {
            preview->phase = PREVIEW_PHASE_HOLD_NEG;
            preview->target_mm = PREVIEW_TARGET_NEG_MM;
            preview->ramp_target_mm = preview->measured_mm;
            preview->integral = 0;
            preview->prev_error_mm = 0;
            preview->phase_elapsed_ms = 0;
            preview->stable_elapsed_ms = 0;
            Serial_SendString("STEP4 PHASE: HOLD -44mm\r\n");
        }
        return;
    }

    if (preview->phase == PREVIEW_PHASE_HOLD_NEG)
    {
        preview->target_mm = PREVIEW_TARGET_NEG_MM;
        return;
    }

    preview->target_mm = 0;
    preview->ramp_target_mm = 0;
}

static void BallControl_ApplyPid(PreviewControl_t *preview, int16_t kp, int16_t ki, int16_t kd, uint16_t pid_scale, int32_t int_clamp)
{
    int32_t delta;
    int32_t mixed;

    if (preview->has_measurement == 0)
    {
        preview->error_mm = 0;
        preview->prev_error_mm = 0;
        preview->integral = 0;
        preview->pulse_delta_us = 0;
        preview->pulse_cmd_us = SERVO_CENTER_PULSE_US;
        Servo_SetPulseUs(preview->pulse_cmd_us);
        return;
    }

    preview->error_mm = (int16_t)(preview->ramp_target_mm - preview->measured_mm);

    preview->integral += preview->error_mm;
    if (preview->integral > int_clamp)
    {
        preview->integral = int_clamp;
    }
    else if (preview->integral < -int_clamp)
    {
        preview->integral = -int_clamp;
    }

    mixed = (int32_t)kp * preview->error_mm;
    mixed += (int32_t)ki * preview->integral;
    mixed += (int32_t)kd * (preview->error_mm - preview->prev_error_mm);

    delta = mixed / pid_scale;

    if (delta > SERVO_SAFE_PULSE_DELTA_US)
    {
        delta = SERVO_SAFE_PULSE_DELTA_US;
    }
    else if (delta < -(int32_t)SERVO_SAFE_PULSE_DELTA_US)
    {
        delta = -(int32_t)SERVO_SAFE_PULSE_DELTA_US;
    }

    preview->pulse_delta_us = (int16_t)delta;
    preview->pulse_cmd_us = Servo_ClampPulse((uint16_t)(SERVO_CENTER_PULSE_US + (int16_t)(PREVIEW_SERVO_DIR * preview->pulse_delta_us)));
    preview->prev_error_mm = preview->error_mm;

    Servo_SetPulseUs(preview->pulse_cmd_us);
}

static void Preview_ApplyControl(PreviewControl_t *preview)
{
    BallControl_ApplyPid(preview, PREVIEW_KP, PREVIEW_KI, PREVIEW_KD, PREVIEW_PID_SCALE, PREVIEW_INT_CLAMP);
}

static void TrackBall_ApplyControl(PreviewControl_t *preview)
{
    BallControl_ApplyPid(preview, TRACK_BALL_KP, TRACK_BALL_KI, TRACK_BALL_KD, TRACK_BALL_PID_SCALE, TRACK_BALL_INT_CLAMP);
}

static void Preview_ShowOLED(const PreviewControl_t *preview)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "X:");
    if (preview->rx_state == OPENMV_TARGET_STATE_NORMAL)
    {
        OLED_ShowSignedNum(1, 3, preview->measured_mm, 3);
    }
    else
    {
        OLED_ShowString(1, 3, "   ");
    }
    OLED_ShowString(2, 1, "ST:");
    OLED_ShowString(2, 4, (char *)Preview_StateText(preview));
}

static void StaticPreview_RunUntilStart(void)
{
    PreviewControl_t preview;

    Preview_Init(&preview);
    Servo_SetPulseUs(SERVO_CENTER_PULSE_US);
    makerobo_brake(0);

    Serial_SendString("\r\n=== STEP4 STATIC PREVIEW ===\r\n");
    Serial_SendString("Target sequence: 0mm -> +50mm -> -45mm (hold)\r\n");
    Serial_SendString("Waiting for ball detection, servo stays level\r\n");
    Serial_SendString("Press KEY after preview completes to enter tracking mode\r\n");
    OLED_Clear();

    while (Key_GetNum() == 0)
    {
        Serial_Task();
        Preview_UpdateMeasurement(&preview);
        Preview_UpdateTarget(&preview);
        Preview_ApplyControl(&preview);

        preview.oled_elapsed_ms += PREVIEW_CTRL_PERIOD_MS;
        if (preview.oled_elapsed_ms >= PREVIEW_OLED_PERIOD_MS)
        {
            preview.oled_elapsed_ms = 0;
            Preview_ShowOLED(&preview);
        }

        Delay_ms(PREVIEW_CTRL_PERIOD_MS);
    }

    Servo_SetPulseUs(SERVO_CENTER_PULSE_US);
    makerobo_brake(0);
    Serial_SendString("STEP4 EXIT: key pressed, enter tracking mode\r\n");
}

static void TIM2_TimeBaseInit(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 10 - 1;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = (IRQn_Type)28;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

static void OLED_ShowTimeValue(uint32_t time_ms)
{
    uint32_t minute = time_ms / 60000;
    uint32_t second = (time_ms % 60000) / 1000;
    uint32_t tenth = (time_ms % 1000) / 100;

    OLED_ShowNum(2, 1, minute, 2);
    OLED_ShowChar(2, 3, ':');
    OLED_ShowNum(2, 4, second, 2);
    OLED_ShowChar(2, 6, '.');
    OLED_ShowNum(2, 7, tenth, 1);
    OLED_ShowChar(2, 8, 's');
}

static uint8_t Robot_Traction(void)
{
    uint8_t left = Left_Irtracking_Get();
    uint8_t right = Right_Irtracking_Get();

    if (g_TrackTimeMs >= STOP_LINE_ARM_DELAY_MS)
    {
        g_StopLineArmed = 1;
    }

    if (left == 0 && right == 0)
    {
        g_StopLineCandidateStartMs = 0;
        makerobo_run(TRACK_SPEED, 0);
        return 0;
    }
    else if (left == 1 && right == 0)
    {
        g_StopLineCandidateStartMs = 0;
        makerobo_Left(TRACK_TURN_SPEED, 0);
        return 0;
    }
    else if (left == 0 && right == 1)
    {
        g_StopLineCandidateStartMs = 0;
        makerobo_Right(TRACK_TURN_SPEED, 0);
        return 0;
    }
    else if (left == 1 && right == 1)
    {
        if (g_StopLineArmed == 0)
        {
            makerobo_run(TRACK_SPEED, 0);
            return 0;
        }

        if (g_StopLineCandidateStartMs == 0)
        {
            g_StopLineCandidateStartMs = g_TrackTimeMs;
        }

        if ((g_TrackTimeMs - g_StopLineCandidateStartMs) >= STOP_LINE_CONFIRM_MS)
        {
            makerobo_brake(0);
            return 1;
        }

        makerobo_brake(0);
        return 0;
    }

    return 0;
}

int main(void)
{
    PreviewControl_t track_ball;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Key_Init();
    Irtracking_Init();
    LEDSEG_Init();
    robot_Init();
    Servo_Init();
    Serial_Init();
    OLED_Init();
    TIM2_TimeBaseInit();
    TrackBall_Init(&track_ball);

    StaticPreview_RunUntilStart();

    OLED_Clear();
    OLED_ShowString(1, 1, "Time:");
    OLED_ShowTimeValue(0);
    makerobo_brake(0);

    while (Key_GetNum() == 0)
    {
        Serial_Task();
        Preview_UpdateMeasurement(&track_ball);
        OLED_Clear();
        OLED_ShowString(1, 1, "Place ball:");
        if (track_ball.has_measurement)
        {
            OLED_ShowSignedNum(2, 1, track_ball.measured_mm, 3);
        }
        else
        {
            OLED_ShowString(2, 1, "----");
        }
        OLED_ShowString(3, 1, "Press KEY");
        Delay_ms(100);
    }

    if (track_ball.has_measurement)
    {
        track_ball.target_mm = track_ball.measured_mm;
    }
    else
    {
        track_ball.target_mm = 0;
    }
    track_ball.ramp_target_mm = track_ball.target_mm;
    track_ball.integral = 0;
    track_ball.prev_error_mm = 0;

    g_TrackTimeMs = 0;
    g_TrackRunning = 1;
    g_StopLineArmed = 0;
    g_StopLineCandidateStartMs = 0;

    Servo_SetPulseUs(SERVO_CENTER_PULSE_US);
    OLED_Clear();
    OLED_ShowString(1, 1, "Time:");
    OLED_ShowTimeValue(0);

    while (1)
    {
        Serial_Task();

        if (g_TrackRunning)
        {
            Preview_UpdateMeasurement(&track_ball);
            TrackBall_ApplyControl(&track_ball);

            if (Robot_Traction())
            {
                g_TrackRunning = 0;
                makerobo_brake(0);
                Servo_SetPulseUs(SERVO_CENTER_PULSE_US);
            }
            OLED_ShowTimeValue(g_TrackTimeMs);
        }
    }
}
