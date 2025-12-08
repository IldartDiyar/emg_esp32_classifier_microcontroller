#include "adc_emg.h"
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "driver/adc.h"

static const char *TAG = "ADC_CONT";

#define DMA_BUFFER_SIZE     512
#define SAMPLE_BUFFER_SIZE  256

static adc_continuous_handle_t adc_handle;


void adc_emg_init()
{
    ESP_LOGI(TAG, "Initializing ADC Continuous Mode...");

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 2048,
        .conv_frame_size = DMA_BUFFER_SIZE,
    };

    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle));

    adc_continuous_config_t config = {
        .sample_freq_hz = 2000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    adc_digi_pattern_config_t pattern = {
        .atten = ADC_ATTEN_DB_12,
        .channel = ADC_CHANNEL_0,          
        .unit = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12,
    };

    config.pattern_num = 1;
    config.adc_pattern = &pattern;

    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &config));

    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));

    ESP_LOGI(TAG, "ADC Continuous Mode Started!");
}


int adc_emg_get_batch(uint16_t *out_buf, int max_samples)
{
    uint8_t raw[DMA_BUFFER_SIZE];
    uint32_t bytes_read = 0;

    esp_err_t ret = adc_continuous_read(adc_handle,
                                        raw,
                                        sizeof(raw),
                                        &bytes_read,
                                        0);

    if (ret == ESP_OK && bytes_read > 0)
    {
        int samples = bytes_read / sizeof(adc_digi_output_data_t);
        samples = (samples > max_samples) ? max_samples : samples;

        adc_digi_output_data_t *d = (adc_digi_output_data_t *)raw;

        for (int i = 0; i < samples; i++)
        {
            out_buf[i] = d[i].type2.data;
        }

        return samples;
    }

    return 0;
}
