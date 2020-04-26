#pragma once

#include <esp_event.h>

typedef struct sensors_temp_data
{
    float avg;
    float min;
    float max;
} sensors_temp_data;

typedef enum {
    SENSORS_READ               /*!< Sensor values were read */
} sensors_event_t;

ESP_EVENT_DECLARE_BASE(SENSORS_EVENT);

void sensors_init();

void read_dht();
void reset_temp_sum();
void reset_temp_minmax();

sensors_temp_data get_temperatures();
uint8_t get_humidity();
