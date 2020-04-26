#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>

#include "esp_event.h"

#include <u8g2.h>

#include <display.h>
#include <wifi.h>
#include <webserver.h>
#include <http.h>
#include <bluetooth.h>
#include <sensors.h>

#include "sdkconfig.h"

static const char *TAG = "MAIN";

static void task_update_display(void *pvParameters);

static void sensors_read_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void display_update_temp_avg(float temp);
static void display_update_humidity(uint8_t value);

static TaskHandle_t displayTaskHandle = NULL;

void app_main()
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Init event loop
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	// Register event to notify when sensors where read
	ESP_ERROR_CHECK(esp_event_handler_register(SENSORS_EVENT, SENSORS_READ, &sensors_read_handler, NULL));

	// Init display
	display_init();
	webserver_init();
	wifi_init();
	http_init();
	// bluetooth_init();
	sensors_init();

	// Sensor data on display
	xTaskCreate(
		task_update_display,   /* Function that implements the task. */
		"task_update_display", /* Text name for the task. */
		2048,				   /* Stack size in words, not bytes. */
		NULL,				   /* Parameter passed into the task. */
		tskIDLE_PRIORITY + 1,  /* Priority at which the task is created. */
		&displayTaskHandle);   /* Used to pass out the created task's handle. */

	vTaskDelete(NULL);
}

static void sensors_read_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	ESP_LOGI(TAG, "SENSORS EVENT FIRED");
	vTaskResume(displayTaskHandle);
}

static void task_update_display(void *pvParameters)
{
	const u8g2_t u8g2 = get_display_instance();

	while (true)
	{
		vTaskSuspend(NULL);

		// get sensor data
		sensors_temp_data data = get_temperatures();
		uint8_t humidity = get_humidity();

		display_update_temp_avg(data.avg);
		display_update_humidity(humidity);

		u8g2_SetDrawColor(&u8g2, 1);
		u8g2_DrawPixel(&u8g2, 0, 0);
		u8g2_UpdateDisplayArea(&u8g2, 0, 0, 1, 1);

		// 0.5s delay
		vTaskDelay(500 / portTICK_PERIOD_MS);

		u8g2_SetDrawColor(&u8g2, 0);
		u8g2_DrawPixel(&u8g2, 0, 0);
		u8g2_UpdateDisplayArea(&u8g2, 0, 0, 1, 1);
	}
}

static void display_update_temp_avg(float temp)
{
	u8g2_t u8g2 = get_display_instance();

	char text_value[16];
	u8g2_uint_t text_width = 0;

	uint8_t tile_x_count = 7;
	uint8_t tile_y_count = 2;
	uint8_t tile_x = 16 - tile_x_count;
	uint8_t tile_y = 0;

	uint8_t area_width = tile_x_count * 8;
	uint8_t area_height = tile_y_count * 8;

	uint8_t text_x_offset = 5;
	uint8_t text_x;
	uint8_t text_y = 14;

	// Convert value to string
	sprintf(text_value, "%.1f", temp);

	// Clear area in buffer
	u8g2_SetDrawColor(&u8g2, 0);
	u8g2_DrawBox(&u8g2, 128 - area_width, 0, 128, area_height);

	// Set font
	u8g2_SetFont(&u8g2, u8g2_font_profont22_mn);

	// Calculate text x position
	text_width = u8g2_GetStrWidth(&u8g2, text_value);
	text_x = 128 - text_width - text_x_offset;

	// Draw text
	u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawStr(&u8g2, text_x, text_y, text_value);

	// Draw °
	u8g2_DrawCircle(&u8g2, 128 - 3, 2, 2, U8G2_DRAW_ALL);

	// Update area on display
	u8g2_UpdateDisplayArea(&u8g2, tile_x, tile_y, tile_x_count, tile_y_count);

	ESP_LOGI(TAG, "Display update temp avg %.1f", temp);
}

static void display_update_humidity(uint8_t value)
{
	u8g2_t u8g2 = get_display_instance();

	char text_value[16];
	u8g2_uint_t text_width = 0;

	uint8_t tile_x_count = 7;
	uint8_t tile_y_count = 2;
	uint8_t tile_x = 16 - tile_x_count;
	uint8_t tile_y = 2;

	uint8_t area_width = tile_x_count * 8;
	uint8_t area_height = tile_y_count * 8;

	uint8_t text_x_offset = 5;
	uint8_t text_x;
	uint8_t text_y = 32;

	// Convert value to string
	sprintf(text_value, "%d", value);

	// Clear area in buffer
	u8g2_SetDrawColor(&u8g2, 0);
	u8g2_DrawBox(&u8g2, 128 - area_width, text_y - area_height, 128, area_height);

	// Set font
	u8g2_SetFont(&u8g2, u8g2_font_profont22_mn);

	// Calculate text x position
	text_width = u8g2_GetStrWidth(&u8g2, text_value);
	text_x = 128 - text_width - text_x_offset;

	// Draw text
	u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawStr(&u8g2, text_x, text_y, text_value);

	// Draw %
	u8g2_DrawPixel(&u8g2, 128 - 5, 18);
	u8g2_DrawPixel(&u8g2, 127, 22);
	u8g2_DrawLine(&u8g2, 128 - 5, 22, 127, 18);

	// Update area on display
	u8g2_UpdateDisplayArea(&u8g2, tile_x, tile_y, tile_x_count, tile_y_count);

	ESP_LOGI(TAG, "Display update humidity %d", value);
}
