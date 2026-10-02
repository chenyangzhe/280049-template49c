/**
 * @file    vofa_qt.h
 * @brief   EricTool host receive service above the SCIA transport driver.
 */

#ifndef VOFA_QT_H
#define VOFA_QT_H

#include <stdint.h>

#define VOFA_QT_SERVICE_MAX_BYTES    (32U)

/**
 * Runs in the background context with raw bytes supplied by EricTool.
 * The handler owns protocol framing and command semantics.
 */
typedef void (*VOFA_QT_RX_HANDLER)(const uint16_t *data, uint16_t length);

/** Initialize the protocol-neutral EricTool host service. */
void VOFA_QT_Init(void);

/** Register the background handler for raw EricTool command bytes. */
void VOFA_QT_Set_Rx_Handler(VOFA_QT_RX_HANDLER handler);

/** Pull raw received bytes without using a handler. */
uint16_t VOFA_QT_Read(uint16_t *dest, uint16_t length);

/** Dispatch at most VOFA_QT_SERVICE_MAX_BYTES from the RX ring per call. */
void VOFA_QT_Service(void);

#endif /* VOFA_QT_H */
