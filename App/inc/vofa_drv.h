/**
 * @file    vofa_drv.h
 * @brief   VOFA SCIA transport driver: FIFO interrupts and software rings.
 */

#ifndef VOFA_DRV_H
#define VOFA_DRV_H

#include <stdbool.h>
#include <stdint.h>

#include "sci.h"

#define VOFA_DRV_TX_RING_LEN    (256U)
#define VOFA_DRV_RX_RING_LEN    (256U)

/** Initialize SCIA FIFO interrupt sources and software TX/RX rings. */
void VOFA_Drv_Init(void);

/**
 * Queue bytes for asynchronous SCIA transmission from the background context.
 * The returned count can be smaller than length when the TX ring is full.
 */
uint16_t VOFA_Drv_Write(const uint16_t *src, uint16_t length);

/** Read received bytes from the SCIA RX software ring in the background. */
uint16_t VOFA_Drv_Read(uint16_t *dest, uint16_t length);

/** Move a short trailing SCI RX FIFO packet into the RX ring from background. */
void VOFA_Drv_Poll_Rx(void);

/** Return the number of bytes currently buffered by the SCIA RX software ring. */
uint16_t VOFA_Drv_Get_Rx_Count(void);

/** Return the monotonically wrapping count of bytes lost by the RX path. */
uint16_t VOFA_Drv_Get_Rx_Drop_Count(void);

/** Return true while software TX data is waiting for the FIFO ISR. */
bool VOFA_Drv_IsTxBusy(void);

/** SCIA TX FIFO ISR; main owns vector registration and PIE enable. */
__interrupt void VOFA_Drv_SCI_TxISR(void);

/** SCIA RX FIFO ISR; it only transfers bytes from FIFO to the RX ring. */
__interrupt void VOFA_Drv_SCI_RxISR(void);

#endif /* VOFA_DRV_H */
