// lqr_controller.h
// Controlador LQR para pendulo invertido
// Motor: JGB37-520 12V | Encoder: 1320 cnt/rev | Roda: 65.4mm
// Ganhos K discretos (Ts=5ms): [-3.0061, -4.7601, 49.2508, 7.7704]

#ifndef LQR_CONTROLLER_H
#define LQR_CONTROLLER_H

#include "main.h"
#include "aux.h"

//constantes que vão ir para agum outro lugar ou ser deletadas

#define TS                  Q25(0.005)
#define THETA_MAX           Q25(0.5236)         // 30 graus em rad
#define FORCA_MAX           Q16(2.5)
#define PWM_PERIOD          999


#define MPU_ADDR            (0x69 << 1)
#define MPU_PWR_MGMT_1      0x6B
#define MPU_GYRO_CONFIG     0x1B
#define MPU_ACCEL_CONFIG    0x1C
#define MPU_ACCEL_XOUT      0x3B
#define MPU_GYRO_XOUT       0x43
#define GYRO_SENS           131.0f
#define ACCEL_SENS          16384.0f



// const q16_t K1 = Q16(-3.0061); // posicao do carro
// const q16_t K2 = Q16(-4.9994); // velocidade do carro
// const q16_t K3 = Q16( 50.9704);    // angulo
// const q16_t K4 = Q16( 8.0437);     // vel. angular

// ===================================================================
//  Estrutura de estados do LQR
// ===================================================================
typedef struct {
    // Estados do pêndulo (Q25)
    q31_t pos;               // posição linear (m)					±10 m ≡ ±1.0
    q31_t vel;               // velocidade linear (m/s)				±0.5 m/s ≡ ±1.0
    q31_t theta;             // ângulo de pitch (rad)				±90°    ≡ ±1.0  (radiano normalizado)
    q31_t theta_dot;         // velocidade angular de pitch (rad/s) ±2000°/s ≡ ±1.0 (rad/s normalizado)

    q31_t u;              // força (Q16)
    int32_t target_rpm;   // RPM × 1000
	int32_t rpm_m1;   // RPM × 1000
	int32_t rpm_m2;   // RPM × 1000
    int16_t pwm;
    uint32_t loop_count;
    uint8_t ativo;
} LQR_State;

// Inicialização
void lqr_init(LQR_State *s);

// Estimação de estado (sensores + encoder)
void encoder_data_update(LQR_State *s);
void filtro_complementar(LQR_State *s, q31_t theta_acc, q31_t gyro_y);

// Controlo
void controle_lqr(LQR_State *s);

// Debug
void debug_print_lqr(const LQR_State *s);

#endif // LQR_CONTROLLER_H
