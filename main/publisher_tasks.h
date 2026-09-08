#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tarea 1: publica un estado alternante ("1"/"0") en TOPIC_PUB.
 */
void mqtt_publisher_task(void *pvParameters);

/**
 * @brief Tarea 2: publica lecturas de las entradas analógicas y el micrófono.
 */
void adc_publisher_task(void *pvParameters);

/**
 * @brief Tarea 3: publica lecturas de temperatura y humedad del DHT11.
 */
void dht11_publisher_task(void *pvParameters);

#ifdef __cplusplus
}
#endif
