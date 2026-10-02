#include "AdcCapture.h"

#include "board.h"
#include "driverlib.h"
#include "inc/hw_memmap.h"

/*
 * DMA side-band waveform capture:
 *   trigger : ADCA INT1, same cadence as ADC SOC0 conversion complete
 *   source  : ADCARESULT0
 *   dest    : adc_capture_buf[0..399]
 *
 * This module is diagnostic only. Do not make the CLA control loop depend on
 * DMA completion ordering.
 */
volatile uint16_t adc_capture_buf[ADC_CAPTURE_BUF_LEN];
volatile uint16_t adc_capture_init_done;

__attribute__((noinline)) void AdcCapture_Init(void)
{
    adc_capture_init_done = 0U;

    DMA_stopChannel(myDMA0_BASE);
    DMA_disableTrigger(myDMA0_BASE);

    DMA_configAddresses(myDMA0_BASE,
                        (const void *)adc_capture_buf,
                        (const void *)ADCARESULT_BASE);
    DMA_configBurst(myDMA0_BASE, 1U, 0, 0);
    DMA_configTransfer(myDMA0_BASE, ADC_CAPTURE_BUF_LEN, 0, 1);
    DMA_configWrap(myDMA0_BASE,
                   65535U,
                   0,
                   ADC_CAPTURE_BUF_LEN,
                   -((int16_t)ADC_CAPTURE_BUF_LEN));
    DMA_configMode(myDMA0_BASE,
                   DMA_TRIGGER_ADCA1,
                   DMA_CFG_ONESHOT_DISABLE |
                   DMA_CFG_CONTINUOUS_ENABLE |
                   DMA_CFG_SIZE_16BIT);

    DMA_enableTrigger(myDMA0_BASE);
    DMA_startChannel(myDMA0_BASE);

    adc_capture_init_done = 1U;
}
