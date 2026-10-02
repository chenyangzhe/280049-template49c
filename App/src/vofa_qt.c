/**
 * @file    vofa_qt.c
 * @brief   EricTool host receive service implementation.
 */

#include "vofa_qt.h"
#include "vofa_drv.h"

#include <stddef.h>

static VOFA_QT_RX_HANDLER vofaQtRxHandler;

void VOFA_QT_Init(void)
{
    vofaQtRxHandler = NULL;
}

void VOFA_QT_Set_Rx_Handler(VOFA_QT_RX_HANDLER handler)
{
    vofaQtRxHandler = handler;
}

uint16_t VOFA_QT_Read(uint16_t *dest, uint16_t length)
{
    return VOFA_Drv_Read(dest, length);
}

void VOFA_QT_Service(void)
{
    uint16_t data[VOFA_QT_SERVICE_MAX_BYTES];
    uint16_t length;

    VOFA_Drv_Poll_Rx();

    if (vofaQtRxHandler == NULL)
    {
        return;
    }

    length = VOFA_Drv_Read(data, VOFA_QT_SERVICE_MAX_BYTES);
    if (length != 0U)
    {
        vofaQtRxHandler(data, length);
    }
}
