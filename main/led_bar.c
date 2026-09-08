#include "led_bar.h"
#include "app_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Los 5 GPIOs de los LEDs, en orden de secuencia (índice 0 = primero en encenderse).
static const gpio_num_t LED_BAR_PINES[5] = {
    LED1_GPIO,
    LED2_GPIO,
    LED3_GPIO,
    LED4_GPIO,
    LED5_GPIO,
};

#define LED_BAR_CANTIDAD (sizeof(LED_BAR_PINES) / sizeof(LED_BAR_PINES[0]))

void led_bar_init(void)
{
    for (int i = 0; i < LED_BAR_CANTIDAD; i++) {
        gpio_set_direction(LED_BAR_PINES[i], GPIO_MODE_OUTPUT);
        gpio_set_level(LED_BAR_PINES[i], 0); // arrancan todos apagados
    }
}

void led_bar_task(void *pvParameters)
{
    int indice_actual = 0;

    while (1) {
        // Apaga todos y enciende solo el LED de la posición actual.
        for (int i = 0; i < LED_BAR_CANTIDAD; i++) {
            gpio_set_level(LED_BAR_PINES[i], (i == indice_actual) ? 1 : 0);
        }

        indice_actual = (indice_actual + 1) % LED_BAR_CANTIDAD;

        vTaskDelay(pdMS_TO_TICKS(LED_SEQUENCE_DELAY_MS));
    }
}
