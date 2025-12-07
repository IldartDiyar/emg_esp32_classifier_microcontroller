#include "adc_emg.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "ADC";
static adc_oneshot_unit_handle_t adc_handle;

void adc_emg_init()
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,       // Use ADC1
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t cfg = {
        .atten = ADC_ATTEN_DB_11,    // 0–3.3V range
        .bitwidth = ADC_BITWIDTH_12, // 0–4095 values
    };

    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &cfg);

    ESP_LOGI(TAG, "ADC initialized on GPIO1 (ADC1_CH0)");
}

int adc_emg_read()
{
    int val = 0;
    adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &val);
    return val;
}
