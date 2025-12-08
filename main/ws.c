#include "ws.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "string.h"
#include "adc_emg.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <stdlib.h>
#include "esp_rom_sys.h"

static const char *TAG = "WS";
static esp_websocket_client_handle_t client;

#define DEVICE_NAME "ESP32-C6-01"

static int g_device_id = 0;
static int g_handshake_ok = 0;
static int g_streaming = 0;
static int g_duration_sec = 0;

static int64_t g_server_time = 0;
static int64_t g_stream_start_esp = 0;

static TaskHandle_t stream_task = NULL;

static void ws_send(const char *msg)
{
    if (!client) return;
    ESP_LOGI(TAG, "SEND: %s", msg);
    esp_websocket_client_send_text(client, msg, strlen(msg), portMAX_DELAY);
}

static void ws_send_handshake()
{
    char buf[128];
    snprintf(buf, sizeof(buf),
        "{\"event\":\"handshake\",\"device_name\":\"%s\"}",
        DEVICE_NAME);
    ws_send(buf);
}

static void stream_task_fn(void *arg)
{
    ESP_LOGI(TAG, "STREAM STARTED (DMA MODE)");

    TickType_t end_tick = xTaskGetTickCount() +
        (g_duration_sec * 1000 / portTICK_PERIOD_MS);

    ws_send("{\"event\":\"raw_stream_begin\"}");

    uint16_t dma_buf[256];

    while (g_streaming && xTaskGetTickCount() < end_tick)
    {
        int n = adc_emg_get_batch(dma_buf, 256);

        if (n > 0)
        {
            int64_t now_us = esp_timer_get_time();
            int64_t delta_us = now_us - g_stream_start_esp;
            int64_t final_ts = g_server_time * 1000000LL + delta_us;

            char json[4096];
            int pos = snprintf(json, sizeof(json),
                "{\"event\":\"raw_stream_in_process\",\"timestamp\":%lld,\"raw\":[",
                (long long)final_ts);

            for (int i = 0; i < n; i++)
            {
                pos += snprintf(json + pos, sizeof(json) - pos,
                                "%d%s", dma_buf[i],
                                (i < n - 1) ? "," : "");
            }

            snprintf(json + pos, sizeof(json) - pos, "]}");

            ws_send(json);
        }

        vTaskDelay(1);
    }

    ws_send("{\"event\":\"raw_stream_finish\"}");

    stream_task = NULL;
    g_streaming = 0;
    vTaskDelete(NULL);
}


static void handle_msg(const char *msg)
{
    ESP_LOGI(TAG, "PARSE: %s", msg);

    if (strstr(msg, "\"event\":\"handshake_ok\""))
    {
        const char *d = strstr(msg, "\"device_id\":");
        if (d) g_device_id = atoi(d + 12);

        g_handshake_ok = 1;
        ESP_LOGI(TAG, "Handshake OK, device ID=%d", g_device_id);
        return;
    }

    if (strstr(msg, "\"event\":\"raw_stream\""))
    {
        const char *dur = strstr(msg, "\"duration\":");
        if (dur) g_duration_sec = atoi(dur + 11);

        const char *ts = strstr(msg, "\"server_time\":");
        if (ts) g_server_time = atoll(ts + 14);

        g_stream_start_esp = esp_timer_get_time();

        g_streaming = 1;

        if (!stream_task)
            xTaskCreate(stream_task_fn, "stream_task", 16384, NULL, 2, &stream_task);

        return;
    }

    if (strstr(msg, "\"event\":\"stop_raw_stream\""))
    {
        g_streaming = 0;
        return;
    }
}

static void websocket_event_handler(void *args,
                                    esp_event_base_t base,
                                    int32_t event_id,
                                    void *event_data)
{
    esp_websocket_event_data_t *data = event_data;

    switch (event_id)
    {
        case WEBSOCKET_EVENT_CONNECTED:
            ws_send_handshake();
            break;

        case WEBSOCKET_EVENT_DATA:
            if (data->op_code == 1)
                handle_msg((char *)data->data_ptr);
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            g_streaming = 0;
            break;
    }
}

void ws_start(void)
{
    esp_websocket_client_config_t cfg = {
        .uri = "ws://194.32.142.57:8080/ws/esp"
    };

    while (!g_handshake_ok)
    {
        ESP_LOGI(TAG, "Connecting to WebSocket...");

        client = esp_websocket_client_init(&cfg);

        esp_websocket_register_events(
            client,
            WEBSOCKET_EVENT_ANY,
            websocket_event_handler,
            NULL
        );

        esp_websocket_client_start(client);

        int waited = 0;
        while (!g_handshake_ok && waited < 5000) {
            vTaskDelay(pdMS_TO_TICKS(100));
            waited += 100;
        }

        if (!g_handshake_ok) {
            ESP_LOGW(TAG, "Handshake not received, retrying in 5s...");
            esp_websocket_client_stop(client);
            esp_websocket_client_destroy(client);
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    ESP_LOGI(TAG, "WebSocket connected and handshake completed!");
}
