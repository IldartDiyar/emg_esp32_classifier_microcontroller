#pragma once

#include <stdint.h>
#include "esp_adc/adc_continuous.h"

void adc_emg_init();
int adc_emg_get_batch(uint16_t *out_buf, int max_samples);
