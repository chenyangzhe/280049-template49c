#ifndef ADC_CAPTURE_H
#define ADC_CAPTURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADC_CAPTURE_BUF_LEN 400U

/*
 * ADC raw sample capture buffer.
 * DMA writes ADCARESULT0 into this buffer for debug/VOFA observation only.
 * The CLA control loop still reads ADCARESULT directly.
 */
extern volatile uint16_t adc_capture_buf[ADC_CAPTURE_BUF_LEN];
extern volatile uint16_t adc_capture_init_done;

void AdcCapture_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_CAPTURE_H */
