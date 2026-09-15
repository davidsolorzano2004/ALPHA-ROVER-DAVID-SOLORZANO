#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#include "app_config.h"
#include "telemetry.h"
#include "webserver.h"

static const char *TAG = "WEBSERVER";

extern const char dashboard_html_start[] asm("_binary_dashboard_html_start");
extern const char dashboard_html_end[]   asm("_binary_dashboard_html_end");

void pwm_led_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = LED5_GPIO,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&ch);
}

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, dashboard_html_start, (dashboard_html_end - dashboard_html_start));
}

static esp_err_t telemetry_get_handler(httpd_req_t *req) {
    char json_response[256];
    get_telemetry_json(json_response, sizeof(json_response));
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t control_get_handler(httpd_req_t *req) {
    char buf[100], dir[32];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK && 
        httpd_query_key_value(buf, "dir", dir, sizeof(dir)) == ESP_OK) {
        
        ESP_LOGI(TAG, "Comando Motor Recibido: %s", dir);
        int m[4] = {0, 0, 0, 0}; // md1, md2, mi1, mi2

        if (!strcmp(dir, "AVANZAR"))         { m[0]=1; m[2]=1; }
        else if (!strcmp(dir, "RETROCEDER")) { m[1]=1; m[3]=1; }
        else if (!strcmp(dir, "IZQUIERDA"))  { m[0]=1; m[3]=1; }
        else if (!strcmp(dir, "DERECHA"))    { m[1]=1; m[2]=1; }

        gpio_set_level(LED1_GPIO, m[0]); gpio_set_level(LED2_GPIO, m[1]); 
        gpio_set_level(LED3_GPIO, m[2]); gpio_set_level(LED4_GPIO, m[3]); 
    }
    return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t light_get_handler(httpd_req_t *req) {
    char buf[100], val[10];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK && 
        httpd_query_key_value(buf, "val", val, sizeof(val)) == ESP_OK) {
        
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, atoi(val));
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
    return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uris[] = {
            { "/", HTTP_GET, root_get_handler, NULL },
            { "/telemetry", HTTP_GET, telemetry_get_handler, NULL },
            { "/control", HTTP_GET, control_get_handler, NULL },
            { "/light", HTTP_GET, light_get_handler, NULL }
        };
        for (int i = 0; i < 4; i++) {
            httpd_register_uri_handler(server, &uris[i]);
        }
        ESP_LOGI(TAG, "Servidor HMI iniciado.");
    }
    return server;
}