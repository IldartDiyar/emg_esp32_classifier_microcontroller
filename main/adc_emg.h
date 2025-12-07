#ifndef ADC_EMG_H
#define ADC_EMG_H

#include "esp_adc/adc_oneshot.h"

// Initialize ADC for EMG sensor (GPIO1 / ADC1_CH0)
void adc_emg_init(void);

// Read one EMG sample (0–4095)
int adc_emg_read(void);

#endif // ADC_EMG_H
