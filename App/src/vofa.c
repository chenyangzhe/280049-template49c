/**
 * @file    vofa.c
 * @brief   VOFA JustFloat snapshot queue and asynchronous SCI transmitter.
 */

#pragma CODE_SECTION(VOFA_Init, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Set_Channel_Input, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Run, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Request_Send, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Cancel_Send, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Get_Send_State, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Get_State, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Get_Drop_Count, ".TI.ramfunc");

#include "vofa.h"
#include "vofa_drv.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define VOFA_QUEUE_MASK       (VOFA_QUEUE_DEPTH - 1U)
#define VOFA_FRAME_WORDS_MAX  ((VOFA_CHANNEL_NUM_MAX * VOFA_PACK_LENGTH) + \
                               VOFA_PACK_LENGTH)
#define VOFA_FIREWATER_U32    (0x7F800000UL)

#if ((VOFA_QUEUE_DEPTH == 0U) || \
     ((VOFA_QUEUE_DEPTH & (VOFA_QUEUE_DEPTH - 1U)) != 0U))
#error "VOFA_QUEUE_DEPTH must be a power of two"
#endif

typedef struct
{
    uint16_t channel_count;
    float32_t channel[VOFA_CHANNEL_NUM_MAX];
} VofaSnapshot;

VOFA_DATA message_array;

static VofaSnapshot vofaQueue[VOFA_QUEUE_DEPTH];
static volatile uint16_t vofaQueueHead;
static volatile uint16_t vofaQueueTail;
static volatile uint16_t vofaDropCount;

static uint16_t vofaFrame[VOFA_FRAME_WORDS_MAX];
static uint16_t vofaFrameCount;
static uint16_t vofaFrameOffset;
static bool vofaFramePending;

static void Vofa_PackU32LE(uint16_t *dst, uint16_t *offset,
                           uint32_t value)
{
    uint16_t i = *offset;

    dst[i++] = (uint16_t)(value & 0xFFU);
    dst[i++] = (uint16_t)((value >> 8U) & 0xFFU);
    dst[i++] = (uint16_t)((value >> 16U) & 0xFFU);
    dst[i++] = (uint16_t)((value >> 24U) & 0xFFU);

    *offset = i;
}

static void Vofa_PackFloatLE(uint16_t *dst, uint16_t *offset,
                             float32_t value)
{
    uint32_t bits;

    (void)memcpy(&bits, &value, sizeof(value));
    Vofa_PackU32LE(dst, offset, bits);
}

static bool Vofa_BeginFrame(void)
{
    uint16_t channel;
    uint16_t offset = 0U;
    uint16_t slot;

    if (vofaQueueHead == vofaQueueTail)
    {
        return false;
    }

    slot = vofaQueueTail;
    for (channel = 0U;
         channel < vofaQueue[slot].channel_count;
         channel++)
    {
        Vofa_PackFloatLE(vofaFrame, &offset,
                         vofaQueue[slot].channel[channel]);
    }

    Vofa_PackU32LE(vofaFrame, &offset, VOFA_FIREWATER_U32);
    vofaQueueTail = (uint16_t)((vofaQueueTail + 1U) & VOFA_QUEUE_MASK);
    vofaFrameCount = offset;
    vofaFrameOffset = 0U;
    vofaFramePending = true;

    return true;
}

void VOFA_Init(void)
{
    uint16_t channel;

    for (channel = 0U; channel < VOFA_CHANNEL_NUM_MAX; channel++)
    {
        message_array.input_pointer[channel] = NULL;
    }

    message_array.used_channel_max = 0U;
    vofaQueueHead = 0U;
    vofaQueueTail = 0U;
    vofaDropCount = 0U;
    vofaFrameCount = 0U;
    vofaFrameOffset = 0U;
    vofaFramePending = false;
}

void VOFA_Set_Channel_Input(uint16_t channel,
                            const volatile float32_t *input_ptr)
{
    if ((channel >= VOFA_CHANNEL_NUM_MAX) || (input_ptr == NULL))
    {
        return;
    }

    message_array.input_pointer[channel] = input_ptr;
    if (channel >= message_array.used_channel_max)
    {
        message_array.used_channel_max = channel + 1U;
    }
}

void VOFA_Request_Send(void)
{
    uint16_t channel;
    uint16_t next;
    uint16_t slot;

    if (message_array.used_channel_max == 0U)
    {
        return;
    }

    next = (uint16_t)((vofaQueueHead + 1U) & VOFA_QUEUE_MASK);
    if (next == vofaQueueTail)
    {
        if (vofaDropCount < 0xFFFFU)
        {
            vofaDropCount++;
        }
        return;
    }

    slot = vofaQueueHead;
    vofaQueue[slot].channel_count = message_array.used_channel_max;
    for (channel = 0U;
         channel < message_array.used_channel_max;
         channel++)
    {
        const volatile float32_t *input =
            message_array.input_pointer[channel];

        vofaQueue[slot].channel[channel] =
            (input != NULL) ? *input : 0.0f;
    }

    vofaQueueHead = next;
}

void VOFA_Run(void)
{
    uint16_t written;
    uint16_t remaining;

    if ((!vofaFramePending) && (!Vofa_BeginFrame()))
    {
        return;
    }

    remaining = (uint16_t)(vofaFrameCount - vofaFrameOffset);
    written = VOFA_Drv_Write(&vofaFrame[vofaFrameOffset], remaining);
    vofaFrameOffset = (uint16_t)(vofaFrameOffset + written);

    if (vofaFrameOffset >= vofaFrameCount)
    {
        vofaFrameCount = 0U;
        vofaFrameOffset = 0U;
        vofaFramePending = false;
    }
}

void VOFA_Cancel_Send(void)
{
    vofaQueueTail = vofaQueueHead;
    vofaFrameCount = 0U;
    vofaFrameOffset = 0U;
    vofaFramePending = false;
}

VOFA_FLAG_SEND VOFA_Get_Send_State(void)
{
    return ((vofaQueueHead != vofaQueueTail) || vofaFramePending ||
            VOFA_Drv_IsTxBusy())
               ? VOFA_Flag_Send_Enable
               : VOFA_Flag_Send_Disable;
}

VOFA_STATE VOFA_Get_State(void)
{
    return (VOFA_Get_Send_State() == VOFA_Flag_Send_Enable)
               ? VOFA_Flag_Sending
               : VOFA_Flag_Idle;
}

uint16_t VOFA_Get_Drop_Count(void)
{
    return vofaDropCount;
}

