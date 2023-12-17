#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"

esp_err_t jpg_stream_httpd_handler(httpd_req_t *req);
httpd_handle_t setup_server(void);
void send2wp(float n);
esp_err_t mic_httpd_handler(httpd_req_t *req);
typedef struct {
	char url[32];
	char parameter[128];
} URL_t;

