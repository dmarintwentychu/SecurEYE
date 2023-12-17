#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/spiffs.h"
#include "include/wifi.h"

#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" //for delay,mutexs,semphrs rtos operations
#include "freertos/task.h"

float mic = 30.0;

esp_http_client_handle_t client;

float extraerValorMic(char *html)
{
    // Buscar la cadena "Mic: "
    const char *micInicio = strstr(html, "Mic: ");
    if (micInicio == NULL)
    {
        // La cadena "Mic: " no fue encontrada
        fprintf(stderr, "Error: No se encontró la cadena \"Mic: \".\n");
        return -1; // Otra forma de manejar el error, puedes ajustar según tus necesidades
    }

    // Avanzar hasta el valor numérico
    micInicio += strlen("Mic: ");

    // Convertir el valor numérico a double
    float micValue = strtof(micInicio, NULL);
    return micValue;
}

esp_err_t client_event_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        // printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        mic = extraerValorMic(evt->data);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void init_rest_function()
{

    esp_http_client_config_t client_configuration = {
        .url = "http://192.168.82.194/mic",
        .event_handler = client_event_handler};
    client = esp_http_client_init(&client_configuration);
}

float getnoise()
{

    esp_http_client_perform(client);
    return mic;
}