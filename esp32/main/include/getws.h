#include "esp_http_server.h"


httpd_handle_t setup_ws_server(void);
double getNumPersonas();
typedef struct {
	char url[32];
	char parameter[128];
} URL_t;
