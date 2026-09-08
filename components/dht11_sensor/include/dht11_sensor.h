#pragma once

#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lee temperatura y humedad de un sensor DHT11 conectado al pin indicado.
 *
 * @param pin          Pin GPIO donde está conectado el DHT11.
 * @param temperatura  Salida: temperatura en °C.
 * @param humedad      Salida: humedad relativa en %.
 * @return ESP_OK si la lectura y el checksum son válidos, ESP_FAIL en caso contrario.
 */
esp_err_t dht11_sensor_read(gpio_num_t pin, int *temperatura, int *humedad);

#ifdef __cplusplus
}
#endif
