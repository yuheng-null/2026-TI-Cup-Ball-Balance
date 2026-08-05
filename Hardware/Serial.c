#include "Serial.h"
#include "stm32f10x.h" // Device header
#include <stdarg.h>
#include <stdio.h>

#define SERIAL_RX_DMA_SIZE 128U
#define SERIAL_RAW_RX_FIFO_SIZE 128U

typedef enum
{
    OPENMV_PARSE_WAIT_START = 0,
    OPENMV_PARSE_WAIT_SIGN,
    OPENMV_PARSE_READ_DIGITS,
    OPENMV_PARSE_X_LOST_O,
    OPENMV_PARSE_X_LOST_S,
    OPENMV_PARSE_X_LOST_T,
    OPENMV_PARSE_BARE_LOST_UP_O,
    OPENMV_PARSE_BARE_LOST_UP_S,
    OPENMV_PARSE_BARE_LOST_UP_T,
    OPENMV_PARSE_BARE_LOST_O,
    OPENMV_PARSE_BARE_LOST_S,
    OPENMV_PARSE_BARE_LOST_T
} OpenMV_ParseState_t;

static uint8_t g_SerialRxDmaBuffer[SERIAL_RX_DMA_SIZE];
static volatile uint16_t g_SerialRxDmaLastPos = 0;
static volatile uint8_t g_SerialIdleFlag = 0;

static uint8_t g_SerialRawRxFifo[SERIAL_RAW_RX_FIFO_SIZE];
static volatile uint16_t g_SerialRawHead = 0;
static volatile uint16_t g_SerialRawTail = 0;

static volatile uint8_t Serial_RxData;
static volatile uint8_t Serial_RxFlag;

static volatile OpenMV_Target_t g_OpenMVTarget = {0, 0, OPENMV_TARGET_STATE_IDLE, 0, 0, 0};
static OpenMV_ParseState_t g_OpenMVParseState = OPENMV_PARSE_WAIT_START;
static int8_t g_OpenMVPendingDirection = 0;
static int16_t g_OpenMVPendingDistance = 0;
static uint8_t g_OpenMVPendingDigitCount = 0;

static void Serial_ProcessByte(uint8_t byteValue);
static uint8_t Serial_IsDigit(char character);
static void Serial_ProcessDmaRx(void);
static void OpenMV_ResetParser(void);
static void OpenMV_ResetAndReplay(uint8_t byteValue);
static void OpenMV_FinalizeFrame(OpenMV_TargetState_t state, uint8_t valid, uint8_t exact, int16_t distanceMm, int8_t direction);
static void Serial_PushRawByte(uint8_t byteValue);

void Serial_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, DISABLE);
    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)g_SerialRxDmaBuffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = SERIAL_RX_DMA_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel5, ENABLE);

    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

void Serial_Task(void)
{
    if (g_SerialIdleFlag != 0)
    {
        g_SerialIdleFlag = 0;
    }

    Serial_ProcessDmaRx();
}

void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
    {
    }
}

void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        Serial_SendByte(Array[i]);
    }
}

void Serial_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        Serial_SendByte(String[i]);
    }
}

static uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
    }
}

int fputc(int ch, FILE *f)
{
    Serial_SendByte((uint8_t)ch);
    return ch;
}

void Serial_Printf(char *format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Serial_SendString(String);
}

uint8_t Serial_GetRxFlag(void)
{
    if (Serial_RxFlag == 1)
    {
        Serial_RxFlag = 0;
        return 1;
    }
    return 0;
}

uint8_t Serial_GetRxData(void)
{
    return Serial_RxData;
}

uint8_t Serial_ReadByte(uint8_t *byte_out)
{
    if (byte_out == 0)
    {
        return 0;
    }

    if (g_SerialRawHead == g_SerialRawTail)
    {
        return 0;
    }

    *byte_out = g_SerialRawRxFifo[g_SerialRawTail];
    g_SerialRawTail++;
    if (g_SerialRawTail >= SERIAL_RAW_RX_FIFO_SIZE)
    {
        g_SerialRawTail = 0;
    }
    return 1;
}

void Serial_ClearRxBuffer(void)
{
    g_SerialRawHead = 0;
    g_SerialRawTail = 0;
}

int16_t OpenMV_GetDistance(void)
{
    return g_OpenMVTarget.distance_mm;
}

int8_t OpenMV_GetDirection(void)
{
    return g_OpenMVTarget.direction;
}

OpenMV_TargetState_t OpenMV_GetTargetState(void)
{
    return g_OpenMVTarget.state;
}

uint8_t OpenMV_IsTargetValid(void)
{
    return g_OpenMVTarget.valid;
}

uint8_t OpenMV_IsTargetExact(void)
{
    return g_OpenMVTarget.exact;
}

uint32_t OpenMV_GetFrameCount(void)
{
    return g_OpenMVTarget.frame_count;
}

void OpenMV_GetTarget(OpenMV_Target_t *target)
{
    if (target == 0)
    {
        return;
    }

    __disable_irq();
    *target = g_OpenMVTarget;
    __enable_irq();
}

static void Serial_PushRawByte(uint8_t byteValue)
{
    uint16_t nextHead = g_SerialRawHead + 1;
    if (nextHead >= SERIAL_RAW_RX_FIFO_SIZE)
    {
        nextHead = 0;
    }

    if (nextHead == g_SerialRawTail)
    {
        g_SerialRawTail++;
        if (g_SerialRawTail >= SERIAL_RAW_RX_FIFO_SIZE)
        {
            g_SerialRawTail = 0;
        }
    }

    g_SerialRawRxFifo[g_SerialRawHead] = byteValue;
    g_SerialRawHead = nextHead;
}

static void OpenMV_ResetParser(void)
{
    g_OpenMVParseState = OPENMV_PARSE_WAIT_START;
    g_OpenMVPendingDirection = 0;
    g_OpenMVPendingDistance = 0;
    g_OpenMVPendingDigitCount = 0;
}

static void OpenMV_FinalizeFrame(OpenMV_TargetState_t state, uint8_t valid, uint8_t exact, int16_t distanceMm, int8_t direction)
{
    g_OpenMVTarget.distance_mm = distanceMm;
    g_OpenMVTarget.direction = direction;
    g_OpenMVTarget.state = state;
    g_OpenMVTarget.valid = valid;
    g_OpenMVTarget.exact = exact;
    g_OpenMVTarget.frame_count++;

    Serial_RxFlag = 1;
    OpenMV_ResetParser();
}

static void OpenMV_ResetAndReplay(uint8_t byteValue)
{
    OpenMV_ResetParser();
    Serial_ProcessByte(byteValue);
}

static uint8_t Serial_IsDigit(char character)
{
    return (character >= '0' && character <= '9');
}

static void Serial_ProcessByte(uint8_t byteValue)
{
    Serial_RxData = byteValue;
    Serial_PushRawByte(byteValue);

    if (byteValue == '\r')
    {
        return;
    }

    if (byteValue == '\n')
    {
        if (g_OpenMVParseState == OPENMV_PARSE_READ_DIGITS && g_OpenMVPendingDigitCount == 4U)
        {
            OpenMV_FinalizeFrame(OPENMV_TARGET_STATE_NORMAL, 1, 1, g_OpenMVPendingDistance, g_OpenMVPendingDirection);
            return;
        }

        if (g_OpenMVParseState == OPENMV_PARSE_X_LOST_T)
        {
            if (g_OpenMVPendingDigitCount == 0U)
            {
                OpenMV_FinalizeFrame(OPENMV_TARGET_STATE_DIRECTION_LOST, 1, 0, 0, g_OpenMVPendingDirection);
                return;
            }

            if (g_OpenMVPendingDigitCount == 4U)
            {
                OpenMV_FinalizeFrame(OPENMV_TARGET_STATE_SHORT_LOST, 1, 0, g_OpenMVPendingDistance, g_OpenMVPendingDirection);
                return;
            }
        }

        if (g_OpenMVParseState == OPENMV_PARSE_BARE_LOST_UP_T)
        {
            OpenMV_FinalizeFrame(OPENMV_TARGET_STATE_DIRECTION_LOST, 0, 0, 0, g_OpenMVTarget.direction);
            return;
        }

        if (g_OpenMVParseState == OPENMV_PARSE_BARE_LOST_T)
        {
            OpenMV_FinalizeFrame(OPENMV_TARGET_STATE_EMPTY_LOST, 0, 0, 0, 0);
            return;
        }

        OpenMV_ResetParser();
        return;
    }

    switch (g_OpenMVParseState)
    {
    case OPENMV_PARSE_WAIT_START:
        if (byteValue == 'X')
        {
            g_OpenMVParseState = OPENMV_PARSE_WAIT_SIGN;
            g_OpenMVPendingDigitCount = 0;
            g_OpenMVPendingDistance = 0;
            g_OpenMVPendingDirection = 0;
        }
        else if (byteValue == 'L')
        {
            g_OpenMVParseState = OPENMV_PARSE_BARE_LOST_UP_O;
        }
        else if (byteValue == 'l')
        {
            g_OpenMVParseState = OPENMV_PARSE_BARE_LOST_O;
        }
        break;

    case OPENMV_PARSE_WAIT_SIGN:
        if (byteValue == '+' || byteValue == '-')
        {
            g_OpenMVPendingDirection = (byteValue == '+') ? 1 : -1;
            g_OpenMVPendingDistance = 0;
            g_OpenMVPendingDigitCount = 0;
            g_OpenMVParseState = OPENMV_PARSE_READ_DIGITS;
        }
        else if (byteValue == 'X' || byteValue == 'l')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_READ_DIGITS:
        if (Serial_IsDigit((char)byteValue) != 0)
        {
            if (g_OpenMVPendingDigitCount < 4U)
            {
                g_OpenMVPendingDistance = (int16_t)(g_OpenMVPendingDistance * 10 + (byteValue - '0'));
                g_OpenMVPendingDigitCount++;
            }
            else
            {
                OpenMV_ResetParser();
            }
        }
        else if (byteValue == 'L')
        {
            if (g_OpenMVPendingDigitCount == 0U || g_OpenMVPendingDigitCount == 4U)
            {
                g_OpenMVParseState = OPENMV_PARSE_X_LOST_O;
            }
            else
            {
                OpenMV_ResetParser();
            }
        }
        else if (byteValue == 'X' || byteValue == 'l')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_X_LOST_O:
        if (byteValue == 'O')
        {
            g_OpenMVParseState = OPENMV_PARSE_X_LOST_S;
        }
        else if (byteValue == 'X' || byteValue == 'l')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_X_LOST_S:
        if (byteValue == 'S')
        {
            g_OpenMVParseState = OPENMV_PARSE_X_LOST_T;
        }
        else if (byteValue == 'X' || byteValue == 'l')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_X_LOST_T:
        if (byteValue == 'T')
        {
            /* 等待 \n 完成整帧 */
        }
        else if (byteValue == 'X' || byteValue == 'l')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_BARE_LOST_O:
        if (byteValue == 'o')
        {
            g_OpenMVParseState = OPENMV_PARSE_BARE_LOST_S;
        }
        else if (byteValue == 'X' || byteValue == 'l')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_BARE_LOST_UP_O:
        if (byteValue == 'O')
        {
            g_OpenMVParseState = OPENMV_PARSE_BARE_LOST_UP_S;
        }
        else if (byteValue == 'X' || byteValue == 'l' || byteValue == 'L')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_BARE_LOST_UP_S:
        if (byteValue == 'S')
        {
            g_OpenMVParseState = OPENMV_PARSE_BARE_LOST_UP_T;
        }
        else if (byteValue == 'X' || byteValue == 'l' || byteValue == 'L')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_BARE_LOST_UP_T:
        if (byteValue == 'T')
        {
            /* 等待 \n 完成整帧 */
        }
        else if (byteValue == 'X' || byteValue == 'l' || byteValue == 'L')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_BARE_LOST_S:
        if (byteValue == 's')
        {
            g_OpenMVParseState = OPENMV_PARSE_BARE_LOST_T;
        }
        else if (byteValue == 'X' || byteValue == 'l')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    case OPENMV_PARSE_BARE_LOST_T:
        if (byteValue == 't')
        {
            /* 等待 \n 完成整帧 */
        }
        else if (byteValue == 'X' || byteValue == 'l')
        {
            OpenMV_ResetAndReplay(byteValue);
        }
        else
        {
            OpenMV_ResetParser();
        }
        break;

    default:
        OpenMV_ResetParser();
        break;
    }
}

static void Serial_ProcessDmaRx(void)
{
    uint16_t dmaCurrentPos = SERIAL_RX_DMA_SIZE - DMA_GetCurrDataCounter(DMA1_Channel5);

    while (g_SerialRxDmaLastPos != dmaCurrentPos)
    {
        Serial_ProcessByte(g_SerialRxDmaBuffer[g_SerialRxDmaLastPos]);
        g_SerialRxDmaLastPos++;
        if (g_SerialRxDmaLastPos >= SERIAL_RX_DMA_SIZE)
        {
            g_SerialRxDmaLastPos = 0;
        }
    }
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        volatile uint32_t temp;
        temp = USART1->SR;
        temp = USART1->DR;
        (void)temp;
        g_SerialIdleFlag = 1;
    }
}
