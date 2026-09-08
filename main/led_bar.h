#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configura los 5 GPIOs de los LEDs como salidas y los apaga todos.
 *        Debe llamarse una sola vez en app_main, antes de crear led_bar_task.
 */
void led_bar_init(void);

/**
 * @brief Tarea de FreeRTOS: hace un parpadeo secuencial fijo de los 5 LEDs
 *        (efecto "chaser", uno tras otro en bucle). No depende de ningún
 *        sensor ni de MQTT; corre de forma completamente independiente.
 *        Crear con xTaskCreate(led_bar_task, "led_bar_task", 2048, NULL, 3, NULL);
 */
void led_bar_task(void *pvParameters);

#ifdef __cplusplus
}
#endif
