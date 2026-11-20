#pragma once

#include "driver/gpio.h"

typedef enum {
    MOTOR_PARAR,
    MOTOR_ADELANTE,
    MOTOR_ATRAS,
    MOTOR_IZQUIERDA,
    MOTOR_DERECHA
} motor_dir_t;

// id1 -> S1 del RoboClaw (M1A/M1B) | id2 -> S2 del RoboClaw (M2A/M2B)
void motor_init(gpio_num_t id1, gpio_num_t id2);

// Mueve el rover. Si algún motor invierte su sentido, para primero y espera.
void motor_mover(motor_dir_t dir);