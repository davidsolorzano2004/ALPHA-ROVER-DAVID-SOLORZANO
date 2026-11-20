#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include "mqtt_client.h"

typedef void (*mqtt_manager_connected_cb_t)(esp_mqtt_client_handle_t client);
typedef void (*mqtt_manager_data_cb_t)(const char *topic, int topic_len, const char *data, int data_len);

void mqtt_manager_set_connected_cb(mqtt_manager_connected_cb_t cb);
void mqtt_manager_set_data_cb(mqtt_manager_data_cb_t cb);
void mqtt_manager_start(const char *broker_uri);
int mqtt_manager_publish(const char *topic, const char *payload);
esp_err_t mqtt_manager_subscribe(const char *topic, int qos);

#endif // MQTT_MANAGER_H