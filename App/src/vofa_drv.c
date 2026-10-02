/**
 * @file    vofa_drv.c
 * @brief   VOFA SCIA transport driver implementation.
 */

#pragma CODE_SECTION(VOFA_Drv_Init, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Drv_Write, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Drv_Read, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Drv_Poll_Rx, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Drv_Get_Rx_Count, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Drv_Get_Rx_Drop_Count, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Drv_IsTxBusy, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Drv_SCI_TxISR, ".TI.ramfunc");
#pragma CODE_SECTION(VOFA_Drv_SCI_RxISR, ".TI.ramfunc");

#include "vofa_drv.h"
#include "board.h"
#include "interrupt.h"

#include <stddef.h>

#define VOFA_DRV_TX_RING_MASK    (VOFA_DRV_TX_RING_LEN - 1U)
#define VOFA_DRV_RX_RING_MASK    (VOFA_DRV_RX_RING_LEN - 1U)

#if ((VOFA_DRV_TX_RING_LEN == 0U) || \
     ((VOFA_DRV_TX_RING_LEN & (VOFA_DRV_TX_RING_LEN - 1U)) != 0U))
#error "VOFA_DRV_TX_RING_LEN must be a power of two"
#endif

#if ((VOFA_DRV_RX_RING_LEN == 0U) || \
     ((VOFA_DRV_RX_RING_LEN & (VOFA_DRV_RX_RING_LEN - 1U)) != 0U))
#error "VOFA_DRV_RX_RING_LEN must be a power of two"
#endif

static uint16_t vofaDrvTxRing[VOFA_DRV_TX_RING_LEN];
static volatile uint16_t vofaDrvTxHead;
static volatile uint16_t vofaDrvTxTail;
static uint16_t vofaDrvRxRing[VOFA_DRV_RX_RING_LEN];
static volatile uint16_t vofaDrvRxHead;
static volatile uint16_t vofaDrvRxTail;
static volatile uint16_t vofaDrvRxDropCount;

static void Vofa_Drv_StoreRxByte(uint16_t byte)
{
    uint16_t next =
        (uint16_t)((vofaDrvRxHead + 1U) & VOFA_DRV_RX_RING_MASK);

    if (next == vofaDrvRxTail)
    {
        vofaDrvRxDropCount++;
    }
    else
    {
        vofaDrvRxRing[vofaDrvRxHead] = byte & 0x00FFU;
        vofaDrvRxHead = next;
    }
}

static void Vofa_Drv_DrainRxFifo(void)
{
    while (SCI_getRxFIFOStatus(VOFA_UART_BASE) != SCI_FIFO_RX0)
    {
        Vofa_Drv_StoreRxByte(SCI_readCharNonBlocking(VOFA_UART_BASE));
    }
}

static void Vofa_Drv_ClearRxOverflow(void)
{
    if (SCI_getOverflowStatus(VOFA_UART_BASE))
    {
        SCI_clearOverflowStatus(VOFA_UART_BASE);
        vofaDrvRxDropCount++;
    }
}

void VOFA_Drv_Init(void)
{
    vofaDrvTxHead = 0U;
    vofaDrvTxTail = 0U;
    vofaDrvRxHead = 0U;
    vofaDrvRxTail = 0U;
    vofaDrvRxDropCount = 0U;

    SCI_disableInterrupt(VOFA_UART_BASE, SCI_INT_RXFF | SCI_INT_TXFF);
    SCI_setFIFOInterruptLevel(VOFA_UART_BASE, SCI_FIFO_TX0, SCI_FIFO_RX4);
    SCI_clearInterruptStatus(VOFA_UART_BASE, SCI_INT_RXFF | SCI_INT_TXFF);
    SCI_clearOverflowStatus(VOFA_UART_BASE);
    SCI_enableInterrupt(VOFA_UART_BASE, SCI_INT_RXFF);
}

uint16_t VOFA_Drv_Write(const uint16_t *src, uint16_t length)
{
    uint16_t count;
    uint16_t head;

    if ((src == NULL) || (length == 0U))
    {
        return 0U;
    }

    head = vofaDrvTxHead;
    for (count = 0U; count < length; count++)
    {
        uint16_t next = (uint16_t)((head + 1U) & VOFA_DRV_TX_RING_MASK);

        if (next == vofaDrvTxTail)
        {
            break;
        }

        vofaDrvTxRing[head] = src[count] & 0x00FFU;
        head = next;
    }

    if (count != 0U)
    {
        vofaDrvTxHead = head;
        SCI_enableInterrupt(VOFA_UART_BASE, SCI_INT_TXFF);
    }

    return count;
}

uint16_t VOFA_Drv_Read(uint16_t *dest, uint16_t length)
{
    uint16_t count;

    if ((dest == NULL) || (length == 0U))
    {
        return 0U;
    }

    for (count = 0U;
         (count < length) && (vofaDrvRxTail != vofaDrvRxHead);
         count++)
    {
        dest[count] = vofaDrvRxRing[vofaDrvRxTail] & 0x00FFU;
        vofaDrvRxTail =
            (uint16_t)((vofaDrvRxTail + 1U) & VOFA_DRV_RX_RING_MASK);
    }

    return count;
}

void VOFA_Drv_Poll_Rx(void)
{
    bool interruptsWereDisabled;

    if ((SCI_getRxFIFOStatus(VOFA_UART_BASE) == SCI_FIFO_RX0) &&
        (!SCI_getOverflowStatus(VOFA_UART_BASE)))
    {
        return;
    }

    interruptsWereDisabled = Interrupt_disableGlobal();
    Vofa_Drv_ClearRxOverflow();
    Vofa_Drv_DrainRxFifo();
    SCI_clearInterruptStatus(VOFA_UART_BASE, SCI_INT_RXFF);

    if (!interruptsWereDisabled)
    {
        (void)Interrupt_enableGlobal();
    }
}

uint16_t VOFA_Drv_Get_Rx_Count(void)
{
    return (uint16_t)((vofaDrvRxHead - vofaDrvRxTail) &
                      VOFA_DRV_RX_RING_MASK);
}

uint16_t VOFA_Drv_Get_Rx_Drop_Count(void)
{
    return vofaDrvRxDropCount;
}

bool VOFA_Drv_IsTxBusy(void)
{
    return (vofaDrvTxHead != vofaDrvTxTail);
}

__interrupt void VOFA_Drv_SCI_TxISR(void)
{
    SCI_clearInterruptStatus(VOFA_UART_BASE, SCI_INT_TXFF);

    while (vofaDrvTxTail != vofaDrvTxHead)
    {
        if (SCI_getTxFIFOStatus(VOFA_UART_BASE) == SCI_FIFO_TX16)
        {
            break;
        }

        SCI_writeCharNonBlocking(VOFA_UART_BASE,
                                 vofaDrvTxRing[vofaDrvTxTail] & 0x00FFU);
        vofaDrvTxTail =
            (uint16_t)((vofaDrvTxTail + 1U) & VOFA_DRV_TX_RING_MASK);
    }

    if (vofaDrvTxTail == vofaDrvTxHead)
    {
        SCI_disableInterrupt(VOFA_UART_BASE, SCI_INT_TXFF);
    }

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

__interrupt void VOFA_Drv_SCI_RxISR(void)
{
    Vofa_Drv_ClearRxOverflow();
    Vofa_Drv_DrainRxFifo();

    SCI_clearInterruptStatus(VOFA_UART_BASE, SCI_INT_RXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
