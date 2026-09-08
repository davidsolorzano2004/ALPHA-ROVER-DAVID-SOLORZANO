#pragma once

#include "mqtt_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback invocado cuando el cliente MQTT se conecta exitosamente
 *        al broker (por ejemplo, para hacer suscripciones iniciales).
 */
typedef void (*mqtt_manager_connected_cb_t)(esp_mqtt_client_handle_t client);

/**
 * @brief Registra el callback de "conectado". Debe llamarse antes de
 *        mqtt_manager_start() si se desea que se invoque en la primera conexión.
 */
void mqtt_manager_set_connected_cb(mqtt_manager_connected_cb_t cb);

/**
 * @brief Inicializa y arranca el cliente MQTT contra el broker indicado.
 */
void mqtt_manager_start(const char *broker_uri);

/**
 * @brief Publica un payload en el tópico indicado (QoS 0, sin retain).
 *
 * @return ID del mensaje publicado (igual que esp_mqtt_client_publish).
 */
int mqtt_manager_publish(const char *topic, const char *payload);

/**
 * @brief Se suscribe a un tópico con el QoS indicado.
 */
esp_err_t mqtt_manager_subscribe(const char *topic, int qos);

#ifdef __cplusplus
}
#endif
