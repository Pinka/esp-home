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
#include <wifi.h>
#include <webserver.h>

#include "sdkconfig.h"
#include "u8g2_esp32_hal.h"

// SDA - GPIO21
#define PIN_SDA 5

// SCL - GPIO22
#define PIN_SCL 4

static const char *TAG = "ssd1306";

// a structure which will contain all the data for one display
u8g2_t u8g2;

void init_display();
void update_display();
void mandelbrot_set(u8g2_t *u8g2);

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

	// Init display
	init_display();
	update_display();

	webserver_init();
	wifi_init();

	vTaskDelete(NULL);
}

void init_display()
{

	u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.sda = PIN_SDA;
	u8g2_esp32_hal.scl = PIN_SCL;
	u8g2_esp32_hal_init(u8g2_esp32_hal);

	u8g2_Setup_ssd1306_i2c_128x32_univision_f(
		&u8g2,
		U8G2_R0,
		//u8x8_byte_sw_i2c,
		u8g2_esp32_i2c_byte_cb,
		u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
	u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);

	ESP_LOGI(TAG, "u8g2_InitDisplay");
	u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,

	ESP_LOGI(TAG, "u8g2_SetPowerSave");
	u8g2_SetPowerSave(&u8g2, 0); // wake up display
	ESP_LOGI(TAG, "u8g2_ClearBuffer");
	u8g2_ClearBuffer(&u8g2);
}

void update_display()
{

	// ESP_LOGI(TAG, "u8g2_DrawBox");
	// u8g2_DrawBox(&u8g2, 0, 26, 80, 6);
	// u8g2_DrawFrame(&u8g2, 0, 26, 100, 6);

	// ESP_LOGI(TAG, "u8g2_SetFont");
	// u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
	// ESP_LOGI(TAG, "u8g2_DrawStr");
	// u8g2_DrawStr(&u8g2, 2, 17, "Hello !");
	// ESP_LOGI(TAG, "u8g2_SendBuffer");
	// u8g2_SendBuffer(&u8g2);

	mandelbrot_set(&u8g2);

	u8g2_DrawPixel(&u8g2, 0, 0);
	u8g2_DrawPixel(&u8g2, 0, 31);

	u8g2_DrawPixel(&u8g2, 127, 0);
	u8g2_DrawPixel(&u8g2, 127, 31);


	ESP_LOGI(TAG, "u8g2_SendBuffer");
	u8g2_SendBuffer(&u8g2);
}

void mandelbrot_set(u8g2_t *u8g2)
{
	unsigned ImageHeight = 32;
	unsigned ImageWidth = 128;

	double MinRe = -2.0;
	double MaxRe = 1.0;
	// double MinRe = -4.0;
	// double MaxRe = 2.0;


	double MinIm = -1.2;
	double MaxIm = MinIm + (MaxRe - MinRe) * ImageHeight / ImageWidth;
	double Re_factor = (MaxRe - MinRe) / (ImageWidth - 1);
	double Im_factor = (MaxIm - MinIm) / (ImageHeight - 1);
	unsigned MaxIterations = 50;

	for (unsigned y = 0; y < ImageHeight; ++y)
	{
		double c_im = MaxIm - y * Im_factor;
		for (unsigned x = 0; x < ImageWidth; ++x)
		{
			double c_re = MinRe + x * Re_factor;

			double Z_re = c_re, Z_im = c_im;
			bool isInside = true;
			for (unsigned n = 0; n < MaxIterations; ++n)
			{
				double Z_re2 = Z_re * Z_re, Z_im2 = Z_im * Z_im;
				if (Z_re2 + Z_im2 > 4)
				{
					isInside = false;
					break;
				}
				Z_im = 2 * Z_re * Z_im + c_im;
				Z_re = Z_re2 - Z_im2 + c_re;
			}
			if (isInside)
			{
				ESP_LOGI(TAG, "u8g2_DrawPixel %d, %d", x, y);
				u8g2_DrawPixel(u8g2, x, y);
			}
		}
	}
}