#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"            // <-- NECESARIO PARA UART_NUM_2
#include "esp_adc/adc_oneshot.h"

// ============================================================================
// CONFIGURACIÓN DE RED Y BROKER MQTT
// ============================================================================
#define WIFI_SSID "David"          
#define WIFI_PASS "26082005"       
#define BROKER_URI "mqtt://broker.hivemq.com"

// Tópicos MQTT generales
#define TOPIC_PUB "taller2/dispositivoA/datos"
#define TOPIC_SUB "taller2/dispositivoB/datos"

// Tópicos ADC y Micrófono
#define TOPIC_PUB_ADC1 "taller2/dispositivoA/analog1"
#define TOPIC_PUB_ADC2 "taller2/dispositivoA/analog2"
#define TOPIC_PUB_MIC  "taller2/dispositivoA/microfono"

// Tópicos DHT11
#define TOPIC_PUB_TEMP "taller2/dispositivoA/temperatura"
#define TOPIC_PUB_HUM  "taller2/dispositivoA/humedad"
#define TOPIC_PUB_JSON "taller2/dispositivoA/dht11"

// Periodos de publicación (ms)
#define PUBLISH_PERIOD_MS     5000
#define ADC_PUBLISH_PERIOD_MS 2000
#define DHT_PUBLISH_PERIOD_MS 3000
#define DHT_RETRY_DELAY_MS 500 

// ============================================================================
// CONFIGURACIÓN DE PINES Y PARÁMETROS DE SENSORES
// ============================================================================
#define DHT_PIN GPIO_NUM_16            

#define ADC1_CHANNEL_ENTRADA_1 ADC_CHANNEL_6   // GPIO34
#define ADC1_CHANNEL_ENTRADA_2 ADC_CHANNEL_7   // GPIO35
#define ADC1_CHANNEL_MIC       ADC_CHANNEL_4   // GPIO32 

#define ADC_NUM_MUESTRAS 64
#define MIC_NUM_MUESTRAS 200

#define MIC_UMBRAL_SILENCIO_MV   150
#define MIC_UMBRAL_BAJO_MV      350
#define MIC_UMBRAL_MEDIO_MV     700

// BARRA DE LEDs
#define LED1_GPIO GPIO_NUM_4
#define LED2_GPIO GPIO_NUM_18
#define LED3_GPIO GPIO_NUM_23
#define LED4_GPIO GPIO_NUM_21
#define LED5_GPIO GPIO_NUM_22

#define LED_SEQUENCE_DELAY_MS 300

// ============================================================================
// CONFIGURACIÓN DE GPS U-BLOX LEA-6 (UART2) Y TELEMETRÍA
// ============================================================================
#define GPS_UART_NUM           UART_NUM_2
#define GPS_TX_PIN             GPIO_NUM_5   // ESP32 TX2 -> GPS RXD
#define GPS_RX_PIN             GPIO_NUM_17  // ESP32 RX2 <- GPS TXD
#define GPS_BAUD_RATE          115200
#define GPS_BUF_SIZE           1024

#define TOPIC_PUB_TELEMETRIA   "taller2/dispositivoA/telemetria"
#define TELEMETRY_PUB_PERIOD_MS 3000