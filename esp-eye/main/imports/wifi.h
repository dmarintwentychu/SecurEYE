#include "esp_system.h"        //esp_init funtions esp_err_t
#include "esp_event.h"         //for wifi event
#include "esp_wifi.h"          //esp_wifi_init functions and wifi operations


void wifi_init(void);
void tryspiffsFile(void);
void wifi_new_connection(char* s,char* p);
