

#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "dht.h"
#include "sensors.h"

static const char *TAG = "SENSORS";
static const dht_sensor_type_t sensor_type = DHT_TYPE_AM2301;
static const gpio_num_t dht_gpio = 21;

static const uint16_t maxReadCount = 200;

static uint16_t readCount = 0;
static uint16_t tempSum = 0;

static uint16_t tempMin = 0;
static uint16_t tempAvg = 0;
static uint16_t tempMax = 0;

static int16_t humidity = 0;

ESP_EVENT_DEFINE_BASE(SENSORS_EVENT);

void read_dht()
{
    int16_t temperature = 0;

    if (dht_read_data(sensor_type, dht_gpio, &humidity, &temperature) == ESP_OK)
    {
        readCount++;
        tempSum = tempSum + temperature;
        tempAvg = tempSum / readCount;

        if (tempMin == 0 || tempMin > temperature)
        {
            tempMin = temperature;
        }

        if (tempMax < temperature)
        {
            tempMax = temperature;
        }

        if (readCount == maxReadCount)
        {
            reset_temp_sum();
        }

        ESP_ERROR_CHECK(esp_event_post(SENSORS_EVENT, SENSORS_READ, NULL, 0, portMAX_DELAY));
        ESP_LOGI(TAG, "Humidity: %d%% TempAvg: %.1fC TempMin: %.1fC TempMax: %.1fC", humidity / 10, tempAvg / 10.0, tempMin / 10.0, tempMax / 10.0);
    }
    else
    {
        ESP_LOGI(TAG, "Could not read data from sensor");
    }
}

void reset_temp_sum()
{
    readCount = 0;
    tempSum = 0;
}

void reset_temp_minmax()
{
    tempMin = tempAvg;
    tempMax = tempAvg;
}

sensors_temp_data get_temperatures()
{
    sensors_temp_data data = {
        .avg = tempAvg / 10.0,
        .min = tempMin / 10.0,
        .max = tempMax / 10.0,
    };

    return data;
}

uint8_t get_humidity()
{
    return (uint8_t)(humidity / 10);
}

static void vTaskFunction(void *pvParameters)
{
    /* Block for 5s. */
    const TickType_t xDelay = 5000 / portTICK_PERIOD_MS;

    while (true)
    {
        vTaskDelay(xDelay);
        read_dht();
    }
}

void sensors_init()
{
    /* Create the task, storing the handle. */
    xTaskCreate(
        vTaskFunction,        /* Function that implements the task. */
        "sensor_read_task",   /* Text name for the task. */
        2048,                 /* Stack size in words, not bytes. */
        NULL,                 /* Parameter passed into the task. */
        tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
        NULL);                /* Used to pass out the created task's handle. */

    ESP_LOGI(TAG, "Init done");
}
