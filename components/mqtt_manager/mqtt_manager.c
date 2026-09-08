#include "mqtt_manager.h"

#include "esp_log.h"

static const char *TAG = "MQTT_SISTEMA";
static esp_mqtt_client_handle_t client;
static mqtt_manager_connected_cb_t connected_cb = NULL;

// ============================================================================
// EVENTOS MQTT
// ============================================================================
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conexión exitosa al Broker MQTT");
            if (connected_cb != NULL) {
                connected_cb(client);
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Desconectado del Broker MQTT");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "--- MENSAJE RECIBIDO ---");
            ESP_LOGI(TAG, "Tópico : %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Dato   : %.*s", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Ocurrió un error en el cliente MQTT");
            break;
        default:
            break;
    }
}

void mqtt_manager_set_connected_cb(mqtt_manager_connected_cb_t cb)
{
    connected_cb = cb;
}

void mqtt_manager_start(const char *broker_uri)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

int mqtt_manager_publish(const char *topic, const char *payload)
{
    return esp_mqtt_client_publish(client, topic, payload, 0, 0, 0);
}

esp_err_t mqtt_manager_subscribe(const char *topic, int qos)
{
    return esp_mqtt_client_subscribe(client, topic, qos);
}
