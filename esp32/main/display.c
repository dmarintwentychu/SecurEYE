#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

static const char *tag = "display";


SSD1306_t dev;


void init_display()
{
    ESP_LOGI(tag, "INTERFACE is SPI");
    ESP_LOGI(tag, "CONFIG_MOSI_GPIO=%d", 23);
    ESP_LOGI(tag, "CONFIG_SCLK_GPIO=%d", 18);
    ESP_LOGI(tag, "CONFIG_CS_GPIO=%d", 5);
    ESP_LOGI(tag, "CONFIG_DC_GPIO=%d", 4);
    ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d", 15);
    spi_master_init(&dev, 23, 18, 5, 4, 15);

    ESP_LOGI(tag, "Panel is 128x32");
    ssd1306_init(&dev, 128, 32);

    ssd1306_clear_screen(&dev, false);

    for (int i = 0; i<4; i++)
    {
        ssd1306_display_text(&dev, i, "            ", 14, false);
    }
}

void display_all(int noiseD, int numPersonasD, float co2D, float humidityD, float temperatureD, int ldrD)
{
    char buff[40];

    sprintf(buff, "db:%d T:%.1f°C", noiseD,temperatureD);
    ssd1306_display_text(&dev, 0, buff, 13, false);
    sprintf(buff, "Personas:%d", numPersonasD);
    ssd1306_display_text(&dev, 1, buff, 10, false);
    sprintf(buff, "CO2:%.0f", co2D);
    ssd1306_display_text(&dev, 2, buff, 6, false);
    sprintf(buff, "Luz:%d Hm:%.2f %%", ldrD,humidityD);
    ssd1306_display_text(&dev, 3, buff, 14, false);
}