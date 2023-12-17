#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include <esp_err.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_spiffs.h"

#include "cJSON.h"


static const char *TAG = "spiffs";

int get_num_ssids() {
    FILE *file = fopen("/spiffs/datos.json", "r");
    if (file == NULL) {
        perror("Que esta pasando estamos todos locos");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(file_size + 1);

    if (buffer == NULL) {
        perror("Error al asignar memoria para el buffer");
        fclose(file);
        return -1;
    }

    fread(buffer, sizeof(char), file_size, file);
    fclose(file);
    buffer[file_size] = '\0';

    cJSON *json_object = cJSON_Parse(buffer);
    free(buffer);

    if (json_object == NULL) {
        printf("Parser failed\n");
        return -1;
    }

    cJSON *ssid_array = cJSON_GetObjectItem(json_object, "ssid");

    if (ssid_array == NULL || !cJSON_IsArray(ssid_array)) {
        cJSON_Delete(json_object);
        return -1;
    }

    int num_elem = cJSON_GetArraySize(ssid_array);

    cJSON_Delete(json_object);

    return num_elem;
}


int read_json(int index, char *ssid, int ssid_size, char *pass, int pass_size) {
    // Reemplaza "tu_archivo.json" con el nombre de tu archivo
    FILE *file = fopen("/spiffs/datos.json", "r");

    if (file == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(file_size + 1);

    if (buffer == NULL) {
        perror("Error al asignar memoria para el buffer");
        fclose(file);
        return -1;
    }

    fread(buffer, sizeof(char), file_size, file);
    fclose(file);
    buffer[file_size] = '\0';

    cJSON *json_object = cJSON_Parse(buffer);
    free(buffer);

    if (json_object == NULL) {
        printf("Parser failed\n");
        return -1;
    }

    cJSON *ssid_array = cJSON_GetObjectItem(json_object, "ssid");
    cJSON *pass_array = cJSON_GetObjectItem(json_object, "pass");

    if (ssid_array == NULL || pass_array == NULL || !cJSON_IsArray(ssid_array) || !cJSON_IsArray(pass_array)) {
        cJSON_Delete(json_object);
        return -1;
    }

    cJSON *ssid_item = cJSON_GetArrayItem(ssid_array, index);
    cJSON *pass_item = cJSON_GetArrayItem(pass_array, index);

    if (ssid_item == NULL || pass_item == NULL || !cJSON_IsString(ssid_item) || !cJSON_IsString(pass_item)) {
        cJSON_Delete(json_object);
        return -1;
    }

    strncpy(ssid, cJSON_GetStringValue(ssid_item), ssid_size);
    strncpy(pass, cJSON_GetStringValue(pass_item), pass_size);

    cJSON_Delete(json_object);

    return 0;
}


void init_filesystem()
{

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    if (used > total)
    {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        }
        else
        {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

    /*ESP_LOGI(TAG, "Opening file");
    f = fopen("/spiffs/config.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fclose(f);*/
}

//------------------------------------------------------------------------------------
//-----------------------FIN FILESYSTEM-----------------------------------------------
//------------------------------------------------------------------------------------