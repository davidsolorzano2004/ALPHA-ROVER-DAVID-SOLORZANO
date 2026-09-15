#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "app_config.h"
#include "gps_sensor.h"

static const char *TAG = "GPS_SENSOR";
static gps_data_t g_latest_gps = {0};
static SemaphoreHandle_t g_gps_mutex = NULL;

extern float g_lat;
extern float g_lon;
extern float g_alt;
extern int   g_sats;
extern bool  g_gps_valid;

static float convert_nmea_to_decimal(float nmea_coord, char direction) {
    int degrees = (int)(nmea_coord / 100);
    float minutes = nmea_coord - (degrees * 100);
    float decimal = degrees + (minutes / 60.0f);
    if (direction == 'S' || direction == 'W') {
        decimal = -decimal;
    }
    return decimal;
}

static void parse_nmea_line(char *line) {
    if (strncmp(line, "$GPGGA", 6) == 0 || strncmp(line, "$GNGGA", 6) == 0) {
        char *tokens[15];
        int tok_count = 0;
        char *ptr = line;

        while (ptr && tok_count < 15) {
            tokens[tok_count++] = ptr;
            char *comma = strchr(ptr, ',');
            if (comma) {
                *comma = '\0';
                ptr = comma + 1;
            } else {
                ptr = NULL;
            }
        }

        if (tok_count >= 10) {
            int fix_quality = (strlen(tokens[6]) > 0) ? atoi(tokens[6]) : 0;
            int sats        = (strlen(tokens[7]) > 0) ? atoi(tokens[7]) : 0;

            if (g_gps_mutex && xSemaphoreTake(g_gps_mutex, pdMS_TO_TICKS(100))) {
                g_latest_gps.satellites = sats;
                g_sats = sats;

                if (fix_quality > 0 && strlen(tokens[2]) > 0 && strlen(tokens[4]) > 0) {
                    float lat_raw = atof(tokens[2]);
                    char ns       = tokens[3][0];
                    float lon_raw = atof(tokens[4]);
                    char ew       = tokens[5][0];
                    float alt     = (strlen(tokens[9]) > 0) ? atof(tokens[9]) : 0.0f;

                    g_latest_gps.latitude  = convert_nmea_to_decimal(lat_raw, ns);
                    g_latest_gps.longitude = convert_nmea_to_decimal(lon_raw, ew);
                    g_latest_gps.altitude  = alt;
                    g_latest_gps.valid     = true;
                } else {
                    g_latest_gps.valid     = false;
                }

                g_lat       = g_latest_gps.latitude;
                g_lon       = g_latest_gps.longitude;
                g_alt       = g_latest_gps.altitude;
                g_gps_valid = g_latest_gps.valid;

                xSemaphoreGive(g_gps_mutex);
            }
        }
    }
}

void gps_sensor_init(void) {
    g_gps_mutex = xSemaphoreCreateMutex();

    uart_config_t uart_config = {
        .baud_rate  = GPS_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART2 para GPS inicializado correctamente a 115200 baudios.");
}

bool gps_get_data(gps_data_t *data) {
    if (!data) return false;
    bool valid = false;
    if (g_gps_mutex && xSemaphoreTake(g_gps_mutex, pdMS_TO_TICKS(100))) {
        *data = g_latest_gps;
        valid = g_latest_gps.valid;
        xSemaphoreGive(g_gps_mutex);
    }
    return valid;
}

void gps_reading_task(void *pvParameters) {
    uint8_t rx_buf[256];
    char line_buf[256];
    int line_idx = 0;

    while (1) {
        int len = uart_read_bytes(GPS_UART_NUM, rx_buf, sizeof(rx_buf) - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = (char)rx_buf[i];
                if (c == '\n' || c == '\r') {
                    if (line_idx > 0) {
                        line_buf[line_idx] = '\0';
                        parse_nmea_line(line_buf);
                        line_idx = 0;
                    }
                } else if (line_idx < sizeof(line_buf) - 1) {
                    line_buf[line_idx++] = c;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}