#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_http_server.h"

void pwm_led_init(void);
httpd_handle_t start_webserver(void);

#endif // WEBSERVER_H