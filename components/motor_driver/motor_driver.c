#include "motor_driver.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "MOTOR";

#define USAR_M2   1     // 0 = un solo motor (M1) | 1 = segundo motor en M2A/M2B

// Timer y canales propios, distintos de los de la luz (TIMER_0 / CHANNEL_0)
#define MOTOR_MODE   LEDC_LOW_SPEED_MODE
#define MOTOR_TIMER  LEDC_TIMER_3
#define MOTOR_CH1    LEDC_CHANNEL_6
#define MOTOR_CH2    LEDC_CHANNEL_7

// Pulso RC en microsegundos (modo RC: 1000-2000 us, 1500 = parado)
#define US_STOP   1500
#define US_FWD    1750  // sube hasta 2000 para más velocidad
#define US_BWD    1250  // baja hasta 1000 para más velocidad

// Pausa en parado antes de invertir el sentido de un motor
#define PAUSA_MS  500

// 50 Hz = 20000 us por periodo; 14 bits = 16384 cuentas
#define US_A_DUTY(us)  ((us) * 16384 / 20000)

static int cur1 = US_STOP, cur2 = US_STOP;   // pulso actual de cada motor

static void aplicar(int us1, int us2) {
    ledc_set_duty(MOTOR_MODE, MOTOR_CH1, US_A_DUTY(us1));
    ledc_update_duty(MOTOR_MODE, MOTOR_CH1);
#if USAR_M2
    ledc_set_duty(MOTOR_MODE, MOTOR_CH2, US_A_DUTY(us2));
    ledc_update_duty(MOTOR_MODE, MOTOR_CH2);
#endif
    cur1 = us1;
    cur2 = us2;
}

void motor_init(gpio_num_t id1, gpio_num_t id2) {
    ledc_timer_config_t timer = {
        .speed_mode      = MOTOR_MODE,
        .timer_num       = MOTOR_TIMER,
        .duty_resolution = LEDC_TIMER_14_BIT,
        .freq_hz         = 50,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    const gpio_num_t pines[2] = {id1, id2};
    const ledc_channel_t canales[2] = {MOTOR_CH1, MOTOR_CH2};
    for (int i = 0; i < 1 + USAR_M2; i++) {
        ledc_channel_config_t ch = {
            .speed_mode = MOTOR_MODE,
            .channel    = canales[i],
            .timer_sel  = MOTOR_TIMER,
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = pines[i],
            .duty       = US_A_DUTY(US_STOP),   // arranca en neutro
            .hpoint     = 0
        };
        ledc_channel_config(&ch);
    }
    ESP_LOGI(TAG, "Motores listos (RoboClaw modo RC, neutro %d us)", US_STOP);
}

void motor_mover(motor_dir_t dir) {
    int us1 = US_STOP, us2 = US_STOP;

    switch (dir) {
        case MOTOR_ADELANTE:  us1 = US_FWD;  us2 = US_FWD;  break;
        case MOTOR_ATRAS:     us1 = US_BWD;  us2 = US_BWD;  break;
        case MOTOR_IZQUIERDA: us1 = US_BWD;  us2 = US_FWD;  break; // M1 atrás, M2 adelante
        case MOTOR_DERECHA:   us1 = US_FWD;  us2 = US_BWD;  break; // M1 adelante, M2 atrás
        default: break;                                            // MOTOR_PARAR
    }

    // Si algún motor cambia de sentido (adelante <-> atrás), primero para y espera
    if ((cur1 - US_STOP) * (us1 - US_STOP) < 0 ||
        (cur2 - US_STOP) * (us2 - US_STOP) < 0) {
        ESP_LOGI(TAG, "Parando antes de invertir...");
        aplicar(US_STOP, US_STOP);
        vTaskDelay(pdMS_TO_TICKS(PAUSA_MS));
    }

    aplicar(us1, us2);
}