
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include <cJSON.h>

#include "http.h"
#include "sensors.h"

#define MAX_HTTP_RECV_BUFFER 512
static const char *TAG = "HTTP_CLIENT";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // Write out data
            // printf("%.*s", evt->data_len, (char*)evt->data);
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    return ESP_OK;
}

void post()
{
    // get sensor data
    sensors_temp_data data = get_temperatures();

    cJSON *root, *temp;
    root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "address", cJSON_CreateString("Brivibas 118"));
    cJSON_AddItemToObject(root, "temp", temp = cJSON_CreateObject());
    cJSON_AddNumberToObject(temp, "avg", data.avg);
    cJSON_AddNumberToObject(temp, "min", data.min);
    cJSON_AddNumberToObject(temp, "max", data.max);

    char *post_data = cJSON_Print(root);

    esp_http_client_config_t config = {
        .url = "https://9tvgcnacn7.execute-api.eu-central-1.amazonaws.com/default/saveHomeData",
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "x-api-key", "7M2EQltN446Ec3pmu99lv4szS4U9MgzBapUyFeCv");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        char *content = malloc(MAX_HTTP_RECV_BUFFER + 1);

        int content_length = esp_http_client_get_content_length(client);
        int total_read_len = 0;
        int read_len;

        if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER)
        {

            read_len = esp_http_client_read(client, content, content_length);

            if (read_len <= 0)
            {
                ESP_LOGE(TAG, "Error read data");
            }

            content[read_len] = 0;
            ESP_LOGD(TAG, "read_len = %d", read_len);
        }

        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d, \n%s",
                 esp_http_client_get_status_code(client),
                 content_length,
                 content);
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

static void vTaskFunction(void *pvParameters)
{
    /* Block for 1min. */
    const TickType_t xDelay = 60000 / portTICK_PERIOD_MS;

    while (true)
    {
        post();
        vTaskDelay(xDelay);
    }
}

void http_init()
{
    /* Create the task, storing the handle. */
    xTaskCreate(
        vTaskFunction,        /* Function that implements the task. */
        "http_post_task",     /* Text name for the task. */
        8192,                 /* Stack size in words, not bytes. */
        NULL,                 /* Parameter passed into the task. */
        tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
        NULL);                /* Used to pass out the created task's handle. */

    ESP_LOGI(TAG, "Init done");
}
