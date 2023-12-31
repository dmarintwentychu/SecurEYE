#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <scd30.h>

#include <esp_log.h>
#include <esp_err.h>

float co2, temperature, humidity;

static const char *TAG = "scd30";


void task(void *pvParameters)
{
    i2c_dev_t dev = {0};

    ESP_ERROR_CHECK(scd30_init_desc(&dev, 0, 21, 22));

    uint16_t version, major_ver, minor_ver;
    ESP_ERROR_CHECK(scd30_read_firmware_version(&dev, &version));

    major_ver = (version >> 8) & 0xf;
    minor_ver = version & 0xf;

    ESP_LOGI(TAG, "SCD30 Firmware Version: %d.%d", major_ver, minor_ver);

    ESP_LOGI(TAG, "Starting continuous measurement");
    ESP_ERROR_CHECK(scd30_trigger_continuous_measurement(&dev, 0));

    bool data_ready;
    while (1)
    {
        vTaskDelay(2000);

        scd30_get_data_ready_status(&dev, &data_ready);

        if (data_ready)
        {
            esp_err_t res = scd30_read_measurement(&dev, &co2, &temperature, &humidity);
            if (res != ESP_OK)
            {
                ESP_LOGE(TAG, "Error reading results %d (%s)", res, esp_err_to_name(res));
                continue;
            }

            if (co2 == 0)
            {
                ESP_LOGW(TAG, "Invalid sample detected, skipping");
                continue;
            }

            /*ESP_LOGI(TAG, "CO2: %.0f ppm", co2);
            ESP_LOGI(TAG, "Temperature: %.2f °C", temperature);
            ESP_LOGI(TAG, "Humidity: %.2f %%", humidity);*/
        }
    }
}

void co2_init()
{
    ESP_ERROR_CHECK(i2cdev_init());

    xTaskCreatePinnedToCore(task, "test", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);
}

float getCO2()
{
    return co2;
}
float getHumidity()
{
    return humidity;
}

float getTemperature()
{
    return temperature;
}