#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h" //for delay,mutexs,semphrs rtos operations
#include "freertos/task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_event.h"

#include "include/wifi.h"
#include "include/getws.h"
#include "include/mqtt.h"

#define STORAGE_NAMESPACE "storage"

static const char *TAG = "getws";

double nPersonas = 0;

void find_in_buffer(char buffer[], int pos, char *resultado)
{
    if (buffer != NULL)
    {
        int length = strlen(buffer);
        int i = 0;

        for (; buffer[i] != '=' && i < length; i++)
            ;
        i++;
        if (pos == 1)
        {
            for (; buffer[i] != '=' && i < length; i++)
                ;

            i++;
        }
        int j = 0;
        for (; buffer[i] != '&' && i < length; i++, j++)
        {
            if (buffer[i] == '+')
            {
                resultado[j] = ' ';
            }
            else
            {
                resultado[j] = buffer[i];
            }
        }

        resultado[j] = '\0';
    }
}

int find_value(char *key, char *parameter, char *value)
{
    // char * addr1;
    char *addr1 = strstr(parameter, key);
    if (addr1 == NULL)
        return 0;
    ESP_LOGD(TAG, "addr1=%s", addr1);

    char *addr2 = addr1 + strlen(key);
    ESP_LOGD(TAG, "addr2=[%s]", addr2);

    char *addr3 = strstr(addr2, "&");
    ESP_LOGD(TAG, "addr3=%p", addr3);
    if (addr3 == NULL)
    {
        strcpy(value, addr2);
    }
    else
    {
        int length = addr3 - addr2;
        ESP_LOGD(TAG, "addr2=%p addr3=%p length=%d", addr2, addr3, length);
        strncpy(value, addr2, length);
        value[length] = 0;
    }
    ESP_LOGI(TAG, "key=[%s] value=[%s]", key, value);
    return strlen(value);
}

esp_err_t load_key_value(char *key, char *value, size_t size)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        return err;

    // Read
    size_t _size = size;
    err = nvs_get_str(my_handle, key, value, &_size);
    ESP_LOGI(TAG, "nvs_get_str err=%d", err);
    // if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    if (err != ESP_OK)
        return err;
    ESP_LOGI(TAG, "err=%d key=[%s] value=[%s] _size=%d", err, key, value, _size);

    // Close
    nvs_close(my_handle);
    // return ESP_OK;
    return err;
}

static esp_err_t mqtt_config_handler(httpd_req_t *req)
{

    ESP_LOGI(TAG, "root_get_handler req->uri=[%s]", req->uri);
    char key[64];
    char parameter[128];
    esp_err_t err;

    httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><html>");
    httpd_resp_sendstr_chunk(req, "<head><meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>configmqtt</title></head>");

    httpd_resp_sendstr_chunk(req, "<body>");

    httpd_resp_sendstr_chunk(req, "<h1>CAMBIA EL TOKEN: </h1>");

    strcpy(key, "Cambiar");
    char esp32[32] = {0};
    err = load_key_value(key, parameter, sizeof(parameter));
    ESP_LOGI(TAG, "%s=%d", key, err);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "parameter=[%s]", parameter);
        find_value("esp32=", parameter, esp32);
    }

    httpd_resp_sendstr_chunk(req, "<h2>Introduce el token mqtt</h2>");
    httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/configmqtt/post\">");
    httpd_resp_sendstr_chunk(req, "esp32: <input type=\"text\" name=\"esp32\" value=\"");
    if (strlen(esp32))
        httpd_resp_sendstr_chunk(req, esp32);
    httpd_resp_sendstr_chunk(req, "\">");
    httpd_resp_sendstr_chunk(req, "<br>");

    httpd_resp_sendstr_chunk(req, "<input type=\"submit\" name=\"submit\" value=\"Cambiar\">");

    err = load_key_value(key, parameter, sizeof(parameter));
    ESP_LOGI(TAG, "%s=%d", key, err);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "parameter=[%s]", parameter);
        httpd_resp_sendstr_chunk(req, key);
        httpd_resp_sendstr_chunk(req, ":");
        httpd_resp_sendstr_chunk(req, parameter);
    }

    httpd_resp_sendstr_chunk(req, "</form><br>");

    httpd_resp_sendstr_chunk(req, "</body></html>");

    return ESP_OK;
}

static esp_err_t mqtt_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "root_post_handler req->uri=[%s]", req->uri);
    ESP_LOGI(TAG, "root_post_handler content length %d", req->content_len);
    char *buf = malloc(req->content_len + 1);
    size_t off = 0;
    while (off < req->content_len)
    {
        /* Read data received in the request */
        int ret = httpd_req_recv(req, buf + off, req->content_len - off);
        if (ret <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                httpd_resp_send_408(req);
            }
            free(buf);
            return ESP_FAIL;
        }
        off += ret;
        ESP_LOGI(TAG, "root_post_handler recv length %d", ret);
    }
    buf[off] = '\0';
    ESP_LOGI(TAG, "root_post_handler buf=[%s]", buf);

    char token[100];

    find_in_buffer(buf, 0, token);

    // write_file(ssid, pass);

    printf("\nSSID = %s\n", token);

    URL_t urlBuf;
    find_value("submit=", buf, urlBuf.url);
    ESP_LOGI(TAG, "urlBuf.url=[%s]", urlBuf.url);
    // strcpy(urlBuf.url, "submit4");
    // strcpy(urlBuf.parameter, &req->uri[9]);
    strcpy(urlBuf.parameter, buf);

    free(buf);

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/configmqtt");
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
    httpd_resp_set_hdr(req, "Connection", "close");
#endif
    httpd_resp_sendstr(req, "post successfully");

    // if (ser2ser == 1)

    new_token(token);

    return ESP_OK;
}

static esp_err_t root_config_handler(httpd_req_t *req)
{

    ESP_LOGI(TAG, "root_get_handler req->uri=[%s]", req->uri);
    char key[64];
    char parameter[128];
    esp_err_t err;

    httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><html>");
    httpd_resp_sendstr_chunk(req, "<head><meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>config</title></head>");

    httpd_resp_sendstr_chunk(req, "<body>");

    httpd_resp_sendstr_chunk(req, "<h1>CAMBIA LAS CREDENCIALES WIFI:</h1>");

    strcpy(key, "Cambiar");
    char SSID[32] = {0};
    char Password[32] = {0};
    err = load_key_value(key, parameter, sizeof(parameter));
    ESP_LOGI(TAG, "%s=%d", key, err);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "parameter=[%s]", parameter);
        find_value("SSID=", parameter, SSID);
        find_value("Password=", parameter, Password);
    }

    httpd_resp_sendstr_chunk(req, "<h2>Introduce el SSID y la Contrasenia:</h2>");
    httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/config/post\">");
    httpd_resp_sendstr_chunk(req, "SSID: <input type=\"text\" name=\"SSID\" value=\"");
    if (strlen(SSID))
        httpd_resp_sendstr_chunk(req, SSID);
    httpd_resp_sendstr_chunk(req, "\">");
    httpd_resp_sendstr_chunk(req, "<br>");
    httpd_resp_sendstr_chunk(req, "Password: <input type=\"text\" name=\"Password\" value=\"");
    if (strlen(Password))
        httpd_resp_sendstr_chunk(req, Password);
    httpd_resp_sendstr_chunk(req, "\">");
    httpd_resp_sendstr_chunk(req, "<br>");
    httpd_resp_sendstr_chunk(req, "<input type=\"submit\" name=\"submit\" value=\"Cambiar\">");

    err = load_key_value(key, parameter, sizeof(parameter));
    ESP_LOGI(TAG, "%s=%d", key, err);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "parameter=[%s]", parameter);
        httpd_resp_sendstr_chunk(req, key);
        httpd_resp_sendstr_chunk(req, ":");
        httpd_resp_sendstr_chunk(req, parameter);
    }

    httpd_resp_sendstr_chunk(req, "</form><br>");

    httpd_resp_sendstr_chunk(req, "</body></html>");

    return ESP_OK;
}

static esp_err_t root_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "root_post_handler req->uri=[%s]", req->uri);
    ESP_LOGI(TAG, "root_post_handler content length %d", req->content_len);
    char *buf = malloc(req->content_len + 1);
    size_t off = 0;
    while (off < req->content_len)
    {
        /* Read data received in the request */
        int ret = httpd_req_recv(req, buf + off, req->content_len - off);
        if (ret <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                httpd_resp_send_408(req);
            }
            free(buf);
            return ESP_FAIL;
        }
        off += ret;
        ESP_LOGI(TAG, "root_post_handler recv length %d", ret);
    }
    buf[off] = '\0';
    ESP_LOGI(TAG, "root_post_handler buf=[%s]", buf);

    char ssid[100];
    char pass[100];

    find_in_buffer(buf, 0, ssid);
    find_in_buffer(buf, 1, pass);

    // write_file(ssid, pass);

    printf("\nSSID = %s\n", ssid);
    printf("PASSWD = %s\n", pass);

    URL_t urlBuf;
    find_value("submit=", buf, urlBuf.url);
    ESP_LOGI(TAG, "urlBuf.url=[%s]", urlBuf.url);
    // strcpy(urlBuf.url, "submit4");
    // strcpy(urlBuf.parameter, &req->uri[9]);
    strcpy(urlBuf.parameter, buf);

    free(buf);

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/config");
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
    httpd_resp_set_hdr(req, "Connection", "close");
#endif
    httpd_resp_sendstr(req, "post successfully");

    // if (ser2ser == 1)

    wifi_new_connection(ssid, pass);

    return ESP_OK;
}

esp_err_t ws_httpd_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len)
    {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
        char buf[10];
        sprintf(buf, "%s", ws_pkt.payload);
        nPersonas = strtod(buf, NULL);
    }
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);

    free(buf);
    return ret;
}

httpd_handle_t setup_ws_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {

        /* URI handler for get */
        httpd_uri_t _jpg_stream_httpd_handler = {
            .uri = "/", // Se puede dejar esto en el root
            .method = HTTP_GET,
            .handler = ws_httpd_handler,
            .user_ctx = NULL,
            .is_websocket = true

        };
        httpd_register_uri_handler(server, &_jpg_stream_httpd_handler);

        /* URI handler for post */
        httpd_uri_t _root_post_handler = {
            .uri = "/configwifi/post",
            .method = HTTP_POST,
            .handler = root_post_handler,
        };
        httpd_register_uri_handler(server, &_root_post_handler);
        httpd_uri_t _root_config_handler = {
            .uri = "/configwifi",
            .method = HTTP_GET,
            .handler = root_config_handler,
        };
        httpd_register_uri_handler(server, &_root_config_handler);

        /* URI handler for post */
        httpd_uri_t _mqtt_post_handler = {
            .uri = "/configmqtt/post",
            .method = HTTP_POST,
            .handler = mqtt_post_handler,
        };
        httpd_register_uri_handler(server, &_mqtt_post_handler);
        httpd_uri_t _mqtt_config_handler = {
            .uri = "/configmqtt",
            .method = HTTP_GET,
            .handler = mqtt_config_handler,
        };
        httpd_register_uri_handler(server, &_mqtt_config_handler);
    }
    return ESP_OK;
}

double getNumPersonas()
{
    return nPersonas;
}