// lqr_controller.h
// Controlador LQR para pendulo invertido
// Motor: JGB37-520 12V | Encoder: 1320 cnt/rev | Roda: 65.4mm
// Ganhos K discretos (Ts=5ms): [-3.0061, -4.7601, 49.2508, 7.7704]

#ifndef LQR_CONTROLLER_H
#define LQR_CONTROLLER_H

#include "main.h"
#include "aux.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TIM_HandleTypeDef htim2;   // PWM motor 1
extern TIM_HandleTypeDef htim3;   // Encoder motor 1
extern TIM_HandleTypeDef htim5;   // Encoder motor 2
extern TIM_HandleTypeDef htim9;   // PWM motor 2
extern I2C_HandleTypeDef hi2c1;   // MPU6050
extern TIM_HandleTypeDef htim4;

extern TIM_HandleTypeDef htim10;		// RPM


// #define ENCODER_CPR         1320
// #define WHEEL_DIAM_M        Q25(0.0654)
// #define WHEEL_CIRCUM_M      Q25(0.20548)
// #define M_PER_COUNT         Q25(0.00015567)


#define TS                  Q25(0.005)
#define THETA_MAX           Q25(0.5236)         // 30 graus em rad
#define FORCA_MAX           Q16(2.5)
#define PWM_PERIOD          999


#define THETA_OFFSET_DEG    0.0f
#define THETA_OFFSET_RAD    (THETA_OFFSET_DEG * 3.14159f / 180.0f)
#define THETA_OFFSET        Q25(THETA_OFFSET_RAD)


#define PWM_DEADBAND_RAW    2
#define PWM_MIN             0
#define PWM_BALANCE_MIN     130


#define ALPHA            Q25(0.88)
#define ONE_MINUS_ALPHA  Q25(0.12)
#define RAD_PER_DEG      Q25(0.017453)


#define MPU_ADDR            (0x69 << 1)
#define MPU_PWR_MGMT_1      0x6B
#define MPU_GYRO_CONFIG     0x1B
#define MPU_ACCEL_CONFIG    0x1C
#define MPU_ACCEL_XOUT      0x3B
#define MPU_GYRO_XOUT       0x43
#define GYRO_SENS           131.0f
#define ACCEL_SENS          16384.0f



// ─── MODO DE OPERACAO ───────────────────────────────────

const q16_t K1 = Q16(-3.0061); // posicao do carro
const q16_t K2 = Q16(-4.9994); // velocidade do carro
const q16_t K3 = Q16( 50.9704);    // angulo
const q16_t K4 = Q16( 8.0437);     // vel. angular

typedef struct {
    q31_t  pos;
    q31_t  vel;
    q31_t  theta;
    q31_t  theta_dot;
    q31_t  theta_dot_raw;

    uint16_t enc1_prev;
    int32_t  enc2_prev;

    q16_t gyro_offset;

    q16_t  u;
    int16_t pwm;
    uint8_t ativo;

    uint32_t loop_count;
} LQR_State;


// Comente DEBUG_MODE para ativar o motor
// Deixe definido para testar sem ligar o driver
//#define DEBUG_MODE

void lqr_init(void);
void lqr_loop(void);   // chamada pela interrupcao do TIM4



void    mpu_init(void);
void    mpu_calibrate(void);
void    mpu_read(q16_t *ax, q16_t *ay, q16_t *az, q16_t *gx, q16_t *gy, q16_t *gz);
void    encoder_update(void);
void    filtro_complementar(q16_t ax, q16_t ay, q16_t az, q16_t gy_dps);
void    controle_lqr(void);
void    motor1_set(int16_t pwm);
void    motor1_parar(void);
void    motor2_set(int16_t pwm);
void    motor2_parar(void);
void    debug_print(void);

// Estados acessiveis externamente (para debug)
// q6_t lqr_get_theta(void);
// q6_t lqr_get_theta_dot(void);
// q6_t lqr_get_pos(void);
// q6_t lqr_get_vel(void);
// q16_t lqr_get_u(void);

#ifdef __cplusplus
}
#endif

#endif // LQR_CONTROLLER_H
