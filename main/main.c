#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h" 
#include "driver/ledc.h" // <-- NUEVA LIBRERÍA PARA PWM

#include "app_config.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "adc_sensor.h"
#include "publisher_tasks.h"
#include "led_bar.h"

extern const char dashboard_html_start[] asm("_binary_dashboard_html_start");
extern const char dashboard_html_end[]   asm("_binary_dashboard_html_end");

static const char *TAG = "MQTT_SISTEMA";

// =============================================================================
// VARIABLES GLOBALES PARA TELEMETRÍA
// =============================================================================
float g_vbat1 = 0.0;    
float g_vbat2 = 0.0;    
float g_temp  = 0.0;    
float g_hum   = 0.0;    
float g_mic_v = 0.0;    
char g_mic_lvl[16] = "silencio"; 

// =============================================================================
// CONFIGURACIÓN DE PWM (LEDC) PARA EL GPIO 22
// =============================================================================
static void pwm_led_init(void) {
    // Configurar el temporizador PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT, // Resolución de 8 bits (0 a 255)
        .freq_hz          = 5000,             // Frecuencia de 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Configurar el canal PWM asociado al GPIO 22 (LED5_GPIO)
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED5_GPIO,          // GPIO 22 configurado en app_config.h
        .duty           = 0,                  // Inicia apagado (0)
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

// =============================================================================
// HANDLERS DEL SERVIDOR WEB
// =============================================================================

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, dashboard_html_start, (dashboard_html_end - dashboard_html_start));
}

static esp_err_t telemetry_get_handler(httpd_req_t *req) {
    char json_response[256];
    snprintf(json_response, sizeof(json_response),
             "{\"vbat1\":%.2f,\"vbat2\":%.2f,\"temp\":%.1f,\"hum\":%.1f,\"mic_v\":%.3f,\"mic_lvl\":\"%s\"}",
             g_vbat1, g_vbat2, g_temp, g_hum, g_mic_v, g_mic_lvl);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t control_get_handler(httpd_req_t *req) {
    char buf[100];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char dir_param[32];
        if (httpd_query_key_value(buf, "dir", dir_param, sizeof(dir_param)) == ESP_OK) {
            ESP_LOGI(TAG, "Comando Motor Recibido: %s", dir_param);
            
            int md1 = 0, md2 = 0, mi1 = 0, mi2 = 0;

            if (strcmp(dir_param, "AVANZAR") == 0) {
                md1 = 1; md2 = 0; mi1 = 1; mi2 = 0;
            } else if (strcmp(dir_param, "RETROCEDER") == 0) {
                md1 = 0; md2 = 1; mi1 = 0; mi2 = 1;
            } else if (strcmp(dir_param, "IZQUIERDA") == 0) {
                md1 = 1; md2 = 0; mi1 = 0; mi2 = 1;
            } else if (strcmp(dir_param, "DERECHA") == 0) {
                md1 = 0; md2 = 1; mi1 = 1; mi2 = 0;
            } else { 
                md1 = 0; md2 = 0; mi1 = 0; mi2 = 0;
            }

            gpio_set_level(LED1_GPIO, md1); 
            gpio_set_level(LED2_GPIO, md2); 
            gpio_set_level(LED3_GPIO, mi1); 
            gpio_set_level(LED4_GPIO, mi2); 
            // Ya no forzamos el apagado de LED5_GPIO aquí para no dañar el PWM
        }
    }
    return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

// Handler actualizado para controlar el brillo
static esp_err_t light_get_handler(httpd_req_t *req) {
    char buf[100];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char val_param[10];
        if (httpd_query_key_value(buf, "val", val_param, sizeof(val_param)) == ESP_OK) {
            int duty = atoi(val_param); // Convierte el texto (0-255) a número
            ESP_LOGI(TAG, "Nivel PWM Recibido: %d", duty);
            
            // Aplica el valor PWM al canal configurado
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }
    }
    return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

// =============================================================================
// REGISTRO DE RUTAS Y ARRANQUE
// =============================================================================
static const httpd_uri_t root_uri      = { .uri = "/",          .method = HTTP_GET, .handler = root_get_handler,      .user_ctx = NULL };
static const httpd_uri_t telemetry_uri = { .uri = "/telemetry", .method = HTTP_GET, .handler = telemetry_get_handler, .user_ctx = NULL };
static const httpd_uri_t control_uri   = { .uri = "/control",   .method = HTTP_GET, .handler = control_get_handler,   .user_ctx = NULL };
static const httpd_uri_t light_uri     = { .uri = "/light",     .method = HTTP_GET, .handler = light_get_handler,     .user_ctx = NULL };

static httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &telemetry_uri);
        httpd_register_uri_handler(server, &control_uri);
        httpd_register_uri_handler(server, &light_uri);
        ESP_LOGI(TAG, "Servidor HMI iniciado y rutas registradas.");
        return server;
    }
    return NULL;
}

static void on_mqtt_connected(esp_mqtt_client_handle_t client) {
    mqtt_manager_subscribe(TOPIC_SUB, 0);
    ESP_LOGI(TAG, "Suscrito al tópico: %s", TOPIC_SUB);
}

// =============================================================================
// APP MAIN
// =============================================================================
void app_main(void) {
    ESP_LOGI(TAG, "Iniciando ESP32 Rover HMI...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_manager_init_sta(WIFI_SSID, WIFI_PASS);
    
    mqtt_manager_set_connected_cb(on_mqtt_connected);
    mqtt_manager_start(BROKER_URI);

    start_webserver(); 

    adc_sensor_init(ADC1_CHANNEL_ENTRADA_1, ADC1_CHANNEL_ENTRADA_2, ADC1_CHANNEL_MIC);
    
    led_bar_init(); // Configura pines base
    pwm_led_init(); // Sobrescribe la configuración del GPIO22 para que funcione con PWM

    xTaskCreate(mqtt_publisher_task, "mqtt_pub_task", 2048, NULL, 5, NULL);
    xTaskCreate(adc_publisher_task,  "adc_pub_task",  4096, NULL, 5, NULL);
    xTaskCreate(dht11_publisher_task, "dht11_pub_task", 4096, NULL, 5, NULL);
}