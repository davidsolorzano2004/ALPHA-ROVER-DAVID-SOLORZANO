#include "dht11_sensor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

// Margen de espera para cada fase del protocolo DHT11 (en microsegundos).
// El protocolo real usa pulsos de hasta ~80us (ack del sensor) y ~70us
// (bit en '1'); 200us da margen holgado frente al valor original de 100us,
// que quedaba casi sin margen frente a esos tiempos reales.
#define DHT_TIMEOUT_US 200

static portMUX_TYPE dht11_mux = portMUX_INITIALIZER_UNLOCKED;

// Espera (bloqueante, en microsegundos) a que el pin alcance 'nivel_esperado'.
// Devuelve ESP_FAIL si se agota DHT_TIMEOUT_US sin lograrlo.
static esp_err_t dht11_esperar_nivel(gpio_num_t pin, int nivel_esperado)
{
    int timeout = 0;
    while (gpio_get_level(pin) != nivel_esperado) {
        if (++timeout > DHT_TIMEOUT_US) {
            return ESP_FAIL;
        }
        esp_rom_delay_us(1);
    }
    return ESP_OK;
}

esp_err_t dht11_sensor_read(gpio_num_t pin, int *temperatura, int *humedad)
{
    uint8_t datos[5] = {0, 0, 0, 0, 0};
    esp_err_t err = ESP_OK;

    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(pin, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    // A partir de aquí el timing es crítico (microsegundos): se entra en
    // sección crítica para que el scheduler y las interrupciones de
    // Wi-Fi/MQTT no interrumpan la lectura a mitad de un pulso, que es
    // la causa más común de fallos intermitentes de checksum en el DHT11.
    taskENTER_CRITICAL(&dht11_mux);

    if (err == ESP_OK) err = dht11_esperar_nivel(pin, 0); // el sensor responde bajando la línea
    if (err == ESP_OK) err = dht11_esperar_nivel(pin, 1); // fin del ack en bajo (~80us)
    if (err == ESP_OK) err = dht11_esperar_nivel(pin, 0); // fin del ack en alto (~80us)

    for (int i = 0; i < 40 && err == ESP_OK; i++) {
        err = dht11_esperar_nivel(pin, 1); // fin del pulso bajo de ~50us (inicio de bit)
        if (err != ESP_OK) break;

        uint32_t t_inicio = esp_timer_get_time();
        err = dht11_esperar_nivel(pin, 0); // duración del pulso alto = valor del bit
        if (err != ESP_OK) break;
        uint32_t duracion = esp_timer_get_time() - t_inicio;

        if (duracion > 40) {
            datos[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    taskEXIT_CRITICAL(&dht11_mux);

    if (err != ESP_OK) {
        return ESP_FAIL;
    }

    if (datos[4] == ((datos[0] + datos[1] + datos[2] + datos[3]) & 0xFF)) {
        *humedad = datos[0];
        *temperatura = datos[2];
        return ESP_OK;
    }

    return ESP_FAIL;
}