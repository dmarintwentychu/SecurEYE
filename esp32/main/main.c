#include <stdio.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" //for delay,mutexs,semphrs rtos operations
#include "freertos/task.h"
#include "esp_sleep.h"

#include "driver/gpio.h"
#include "driver/adc.h"

#include "include/spiffs.h"
#include "include/wifi.h"

#include "include/getwp.h"
#include "include/getws.h"
#include "include/display.h"

#include "include/co2.h"
#include "include/mqtt.h"


#define VALUE_PORT GPIO_NUM_33
#define VALUE_PORT2 GPIO_NUM_32

static const char *TAG = "main";

static bool is_button_pressed() {
    return gpio_get_level(VALUE_PORT2) == 1;
}

static void enter_light_sleep() {
    printf("Entering light sleep mode...\n");
    
    // Enable GPIO as wake-up source
    esp_sleep_enable_ext0_wakeup(VALUE_PORT2, 1); 

    // Enter light sleep mode
    esp_light_sleep_start();
    printf("Woke up from light sleep mode.\n");
}


void app_main()
{

    float noise = 30.0;
    double numPersonas = 0;
    int ldr = 0;
    float CO2 = 0;
    float humidity = 0;
    float temperature = 0;

    init_filesystem();

    char tokenesp32[30];
    getToken("esp32", tokenesp32);

    wifi_init();
    
    tryspiffsFile();

    vTaskDelay(15000 / portTICK_PERIOD_MS);
    init_rest_function();
    setup_ws_server();

    co2_init();

    init_display();

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_0);
    gpio_set_direction(VALUE_PORT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(VALUE_PORT, GPIO_PULLUP_ONLY);

    gpio_set_direction(VALUE_PORT2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(VALUE_PORT2, GPIO_PULLUP_ONLY);

    mqtt_init(tokenesp32);

    while (1)
    {
        // VARIABLES DE LA CÁMARA:
        noise = getnoise();
        numPersonas = getNumPersonas();
        ldr = adc1_get_raw(ADC1_CHANNEL_5);
        CO2 = getCO2();
        humidity = getHumidity();
        temperature = getTemperature();

        display_all(noise, numPersonas, CO2, humidity, temperature, ldr);

        mqtt_send(noise, "noise");
        mqtt_send(CO2, "co2");
        mqtt_send(humidity, "humidity");
        mqtt_send(numPersonas, "npeople");
        mqtt_send(ldr, "ldr");
        mqtt_send(temperature, "temperature");

       if (is_button_pressed()) {
            printf("Button pressed! Entering light sleep...\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            enter_light_sleep();
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
