#ifndef SPEED_CONTROLLER_H
#define SPEED_CONTROLLER_H

#include <stdint.h>

typedef struct {
    // Ganhos do PI
    int32_t Kp;               // ganho proporcional  (escala 1000)
    int32_t Ki;               // ganho integral      (escala 1000)
	int32_t Kd;               // ganho proporcional  (escala 1000)

    // Estado interno
    int64_t integral;         // acumulador do erro * dt (escala rpm*1000 * ms)
    int32_t last_error;       // último erro (rpm*1000)

    // Limites do PWM
    int32_t pwm_min;          // PWM mínimo (ex: 0)
    int32_t pwm_max;          // PWM máximo (ex: 1000)

    // Tempo de amostragem (ms)
    uint32_t dt_ms;           // intervalo entre chamadas (5 ms)
} speed_controller_t;

// Inicializa o controlador com ganhos e limites
void speed_controller_init(speed_controller_t *ctrl, int32_t Kp, int32_t Ki,int32_t Kd,
                           int32_t pwm_min, int32_t pwm_max, uint32_t dt_ms);

// Executa um passo do PI: recebe setpoint (rpm*1000) e medição (rpm*1000)
// Retorna o PWM (0-1000)
int32_t speed_controller_update(speed_controller_t *ctrl, int32_t setpoint_rpm, int32_t current_rpm, int32_t last_rpm);

// Reseta o integrador
void speed_controller_reset(speed_controller_t *ctrl);

#endif
