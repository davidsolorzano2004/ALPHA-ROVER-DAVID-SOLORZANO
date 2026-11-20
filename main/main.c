#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <string.h>
#include <stdlib.h>

#include "app_config.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "adc_sensor.h"
#include "publisher_tasks.h"
#include "led_bar.h"
#include "gps_sensor.h"
#include "telemetry.h"
#include "webserver.h"
#include "motor_driver.h"

static const char *TAG = "MAIN";

// Callback que se ejecuta cuando el ESP32 se conecta al Broker MQTT
static void on_mqtt_connected(esp_mqtt_client_handle_t client) {
    ESP_LOGI(TAG, "Conectado al Broker MQTT exitosamente");
    mqtt_manager_subscribe("taller2/dispositivoA/control", 0);
}

// Procesa los comandos de movimiento y luz recibidos por MQTT
static void on_mqtt_data_received(const char *topic, int topic_len, const char *data, int data_len) {
    char cmd[64] = {0};
    if (data_len < sizeof(cmd)) {
        memcpy(cmd, data, data_len);
        cmd[data_len] = '\0';
    } else {
        return;
    }

    ESP_LOGI(TAG, "Comando MQTT Recibido: %s", cmd);

    if (strcmp(cmd, "AVANZAR") == 0)         motor_mover(MOTOR_ADELANTE);
    else if (strcmp(cmd, "RETROCEDER") == 0) motor_mover(MOTOR_ATRAS);
    else if (strcmp(cmd, "IZQUIERDA") == 0)  motor_mover(MOTOR_IZQUIERDA);
    else if (strcmp(cmd, "DERECHA") == 0)    motor_mover(MOTOR_DERECHA);
    else if (strncmp(cmd, "LIGHT:", 6) == 0) {
        int val = atoi(cmd + 6);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, val);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        return;
    }
    else motor_mover(MOTOR_PARAR);   // cualquier otro comando detiene (igual que antes)
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
    mqtt_manager_set_data_cb(on_mqtt_data_received);
    mqtt_manager_start(BROKER_URI);

    // Inicializar Servidores y Periféricos
    start_webserver();
    adc_sensor_init(ADC1_CHANNEL_ENTRADA_1, ADC1_CHANNEL_ENTRADA_2, ADC1_CHANNEL_MIC);
    led_bar_init();
    pwm_led_init();
    motor_init(ID1, ID2);   // ID1 -> S1 (M1) | ID2 -> S2 (M2) del RoboClaw
    gps_sensor_init();

    // Crear Tareas FreeRTOS
    xTaskCreate(gps_reading_task,           "gps_task",      4096, NULL, 5, NULL);
    xTaskCreate(mqtt_publisher_task,        "mqtt_pub",      2048, NULL, 5, NULL);
    xTaskCreate(adc_publisher_task,         "adc_pub",       4096, NULL, 5, NULL);
    xTaskCreate(dht11_publisher_task,       "dht11_pub",     4096, NULL, 5, NULL);
    xTaskCreate(telemetry_mqtt_publisher_task, "telemetry_pub", 4096, NULL, 5, NULL);
}