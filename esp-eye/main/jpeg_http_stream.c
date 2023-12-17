#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <esp_err.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "nvs.h"


#include "imports/wifi.h"
#include "imports/jpeg_http_stream.h"
#define STORAGE_NAMESPACE "storage"

char *TAG = "web-server";

float noisewp = 0.0;

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


#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

esp_err_t jpg_stream_httpd_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t *_jpg_buf;
    char *part_buf[64];
    static int64_t last_frame = 0;
    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        return res;
    }

    while (true)
    {

        fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGE(TAG, "Camera capture failed");
            res = -1;
            break;
        }

        if (fb->format != PIXFORMAT_JPEG)
        {
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if (!jpeg_converted)
            {
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = -1;
            }
        }
        else
        {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        if (res == ESP_OK)
        {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (fb->format != PIXFORMAT_JPEG)
        {
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if (res != ESP_OK)
        {
            break;
        }

        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %luKB %lums (%.1ffps)",
                 (uint32_t)(_jpg_buf_len / 1024),
                 (uint32_t)frame_time,
                 1000.0 / (uint32_t)frame_time);


    }

    last_frame = 0;
    return res;
}

void send2wp(float n)
{
    noisewp = n;
}

esp_err_t mic_httpd_handler(httpd_req_t *req)
{
    char mic_webpage[250];
    sprintf(mic_webpage, "<!DOCTYPE html><html lang=\"es\"><head><meta charset=\"UTF-8\"><meta http-equiv=\"refresh\" content=\"1\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>micro</title></head><body><div><p>Mic: %f </p></div></body></html>", noisewp);

    httpd_resp_sendstr(req, mic_webpage);
    //vTaskDelay(1000 / portTICK_PERIOD_MS);

    return ESP_OK;
}

httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {

        /* URI handler for get */
        httpd_uri_t _jpg_stream_httpd_handler = {
            .uri = "/", // Se puede dejar esto en el root
            .method = HTTP_GET,
            .handler = jpg_stream_httpd_handler,
        };
        httpd_register_uri_handler(server, &_jpg_stream_httpd_handler);

        /* URI handler for get*/
        
          httpd_uri_t __mic_httpd_handler = {
              .uri = "/mic",
              .method = HTTP_GET,
              .handler = mic_httpd_handler,
          };
          httpd_register_uri_handler(server, &__mic_httpd_handler);

                  /* URI handler for post */
        httpd_uri_t _root_post_handler = {
            .uri = "/config/post",
            .method = HTTP_POST,
            .handler = root_post_handler,
        };
        httpd_register_uri_handler(server, &_root_post_handler);
        httpd_uri_t _root_config_handler = {
            .uri = "/config",
            .method = HTTP_GET,
            .handler = root_config_handler,
        };
        httpd_register_uri_handler(server, &_root_config_handler);
    }
    return ESP_OK;
}