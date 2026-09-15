#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "adc_sensor.h"
#include "publisher_tasks.h"
#include "led_bar.h"
#include "gps_sensor.h"
#include "telemetry.h"
#include "webserver.h"

static const char *TAG = "MAIN";

static void on_mqtt_connected(esp_mqtt_client_handle_t client) {
    mqtt_manager_subscribe(TOPIC_SUB, 0);
    ESP_LOGI(TAG, "Suscrito al tópico: %s", TOPIC_SUB);
}

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando ESP32 Rover HMI...");

    // Inicializar Flash NVS
    if (nvs_flash_init() == ESP_ERR_NVS_NO_FREE_PAGES || nvs_flash_init() == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
    }

    // Inicializar Conectividad Red & MQTT
    wifi_manager_init_sta(WIFI_SSID, WIFI_PASS);
    mqtt_manager_set_connected_cb(on_mqtt_connected);
    mqtt_manager_start(BROKER_URI);

    // Inicializar Servidores y Periféricos
    start_webserver(); 
    adc_sensor_init(ADC1_CHANNEL_ENTRADA_1, ADC1_CHANNEL_ENTRADA_2, ADC1_CHANNEL_MIC);
    led_bar_init(); 
    pwm_led_init(); 
    gps_sensor_init();

    // Crear Tareas FreeRTOS
    xTaskCreate(gps_reading_task,           "gps_task",      4096, NULL, 5, NULL);
    xTaskCreate(mqtt_publisher_task,        "mqtt_pub",      2048, NULL, 5, NULL);
    xTaskCreate(adc_publisher_task,         "adc_pub",       4096, NULL, 5, NULL);
    xTaskCreate(dht11_publisher_task,       "dht11_pub",     4096, NULL, 5, NULL);
    xTaskCreate(telemetry_mqtt_publisher_task, "telemetry_pub", 4096, NULL, 5, NULL);
}