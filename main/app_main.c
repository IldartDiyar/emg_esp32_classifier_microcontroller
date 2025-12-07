#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "net.h"
#include "nvs_flash.h" 
#include "ws.h"
#include "adc_emg.h"

static const char *TAG = "APP";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing NVS...");
    nvs_flash_init();

    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Initializing ADC...");
    adc_emg_init();

    ESP_LOGI(TAG, "Starting WebSocket...");
    ws_start();

    while (1) vTaskDelay(pdMS_TO_TICKS(5000));
}
