/**
 * @file    vofa.h
 * @brief   VOFA JustFloat snapshot queue and asynchronous SCI transmitter.
 */

#ifndef VOFA_H
#define VOFA_H

#include <stdint.h>
#include "inc/hw_types.h"

#define VOFA_CHANNEL_NUM_MAX    (10U)
#define VOFA_PACK_LENGTH        (4U)
#define VOFA_QUEUE_DEPTH        (16U)

typedef enum VOFA_FLAG_SEND
{
    VOFA_Flag_Send_Disable = 0U,
    VOFA_Flag_Send_Enable  = 1U
} VOFA_FLAG_SEND;

typedef enum VOFA_STATE
{
    VOFA_Flag_Idle    = 0U,
    VOFA_Flag_Sending = 1U
} VOFA_STATE;

typedef struct VOFA_DATA
{
    const volatile float32_t *input_pointer[VOFA_CHANNEL_NUM_MAX];
    uint16_t used_channel_max;
} VOFA_DATA;

extern VOFA_DATA message_array;

/** Initialize the software queues and leave the SCI TX interrupt idle. */
void VOFA_Init(void);

/** Associate one VOFA channel with a live observable. */
void VOFA_Set_Channel_Input(uint16_t channel,
                            const volatile float32_t *input_ptr);

/** Capture all configured channels into the snapshot queue. */
void VOFA_Request_Send(void);

/** Pack and enqueue one pending frame without waiting for SCI hardware. */
void VOFA_Run(void);

/** Discard snapshots that have not yet entered the SCI TX ring. */
void VOFA_Cancel_Send(void);

VOFA_FLAG_SEND VOFA_Get_Send_State(void);
VOFA_STATE VOFA_Get_State(void);

/** Number of snapshots dropped because the snapshot queue was full. */
uint16_t VOFA_Get_Drop_Count(void);

#endif /* VOFA_H */
