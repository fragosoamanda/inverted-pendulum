#include "lqr_controller.h"
#include "encoder.h"
#include "ArduinoWrapper.h"
#include <math.h>
#include <string.h>

// ===================================================================
//  Ganhos LQR (Q16) – originais, precisam de re‑sintonia
// ===================================================================
// static const q16_t K1 = Q16(-3.0061);
// static const q16_t K2 = Q16(-4.9994);
// static const q16_t K3 = Q16( 50.9704);
// static const q16_t K4 = Q16( 8.0437);

// Ganhos originais (float): -3.0061, -4.9994, 50.9704, 8.0437
// Convertidos para Q31: ganho_float * 2^31 / 2^16 = ganho_float * 32768
#define K1_Q31  (-98500)      // -3.0061 * 32768 ≈ -98500
#define K2_Q31  (-163800)     // -4.9994 * 32768 ≈ -163800
#define K3_Q31  (1670000)     //  50.9704 * 32768 ≈ 1670000
#define K4_Q31  (263600)      //  8.0437  * 32768 ≈ 263600

// ===================================================================
//  Constantes de normalização e tempo
// ===================================================================
#define RPM_MAX              60000        // 100 RPM ×1000
// #define THETA_MAX_Q31        Q31(1.0f / 3.0f)   // 30° na escala ±90° ≡ ±1
#define THETA_MAX_Q31        Q31(0.99999998f)   // ou Q31(0.333333f)
#define ALPHA_Q31            Q31(0.98f)
#define ONE_MINUS_ALPHA_Q31  Q31(0.02f)
#define DT_Q31               Q31(0.005f)

// Fator de integração da posição:
// vel está em ±0.5 m/s, pos em ±10 m, dt = 5 ms
// factor = (0.5/10) * 0.005 = 0.00025
// 0.00025 * 2^31 = 536871
// Alternativamente: (DT_Q31 / 20) => 10737418 / 20 = 536870.9
#define POS_INTEG_FACTOR_Q31  (DT_Q31 / 20)

// ===================================================================
void lqr_init(LQR_State *s) {
    memset(s, 0, sizeof(LQR_State));
    s->ativo = 1;
    print("[LQR] Pronto (Q31, pitch ±90°, gyro ±2000°/s, vel ±0.5 m/s, pos ±10 m)\r\n");
}

// ===================================================================
void encoder_data_update(LQR_State *s) {
    int32_t rpm = encoder_get_sliding_rpm(0);
    int32_t speed_mm_s = encoder_rpm_to_mm_s(rpm);   // mm/s

    // Normalizar velocidade: ±0.5 m/s ≡ ±1.0 (500 mm/s)
    s->vel = (q31_t)(((int64_t)speed_mm_s << 31) / 500);

    // Integrar posição (com wrap‑around natural de Q31)
    s->pos += Q31_MUL(s->vel, (q31_t)POS_INTEG_FACTOR_Q31);
}

// ===================================================================
void filtro_complementar(LQR_State *s, q31_t theta_acc, q31_t gyro_y) {
    // Previsão
    q31_t delta = Q31_MUL(gyro_y, (q31_t)DT_Q31);
    q31_t theta_pred = s->theta + delta;

    // Fusão
    q31_t term1 = Q31_MUL((q31_t)ALPHA_Q31, theta_pred);
    q31_t term2 = Q31_MUL((q31_t)ONE_MINUS_ALPHA_Q31, theta_acc);
    s->theta = term1 + term2;

    s->theta_dot = gyro_y;
}

// ===================================================================
void controle_lqr(LQR_State *s) {
    if (s->theta > (q31_t)THETA_MAX_Q31 || s->theta < -(q31_t)THETA_MAX_Q31) {
        s->u = 0;
        s->target_rpm = 0;
        return;
    }

    q31_t theta_err = s->theta;

    // Lei de controlo: todos os operandos em Q31 → resultado Q31
    q31_t u_raw = Q31_MUL((q31_t)K1_Q31, s->pos)
                + Q31_MUL((q31_t)K2_Q31, s->vel)
                + Q31_MUL((q31_t)K3_Q31, theta_err)
                + Q31_MUL((q31_t)K4_Q31, s->theta_dot);

    s->u = -u_raw;

    // Força (Q31) → RPM desejado (×1000)
    s->target_rpm = (int32_t)(((int64_t)s->u * (int64_t)RPM_MAX) >> 31);
}
// ===================================================================
void debug_print_lqr(const LQR_State *s) {
    char buf[200];

    // Ângulo (graus * 100) – ±90° ≡ ±1.0
    int32_t th_cdeg = (int32_t)(((int64_t)s->theta * 9000) >> 31);
    int32_t th_int = th_cdeg / 100;
    uint32_t th_frac = abs(th_cdeg) % 100;

    // Vel. angular (graus/s * 100) – ±2000°/s ≡ ±1.0
    int32_t thdot_cdeg = (int32_t)(((int64_t)s->theta_dot * 200000) >> 31);
    int32_t thdot_int = thdot_cdeg / 100;
    uint32_t thdot_frac = abs(thdot_cdeg) % 100;

    // Velocidade (mm/s) – ±0.5 m/s ≡ ±1.0
    int32_t vel_mm_s = (int32_t)(((int64_t)s->vel * 500) >> 31);
    int32_t vel_int = vel_mm_s / 1000;
    uint32_t vel_frac = abs(vel_mm_s) % 1000;

    // Posição (mm) – ±10 m ≡ ±1.0
    int32_t pos_mm = (int32_t)(((int64_t)s->pos * 10000) >> 31);
    int32_t pos_int = pos_mm / 1000;
    uint32_t pos_frac = abs(pos_mm) % 1000;

    // RPM alvo
    int32_t tgt_int = s->target_rpm / 1000;
    uint32_t tgt_frac = abs(s->target_rpm) % 1000;

    // RPM actual (formata com 3 inteiros e 3 decimais)
    int32_t cur1_int = s->rpm_m1 / 1000;
    uint32_t cur1_frac = abs(s->rpm_m1) % 1000;

    // RPM actual (formata com 3 inteiros e 3 decimais)
    int32_t cur2_int = s->rpm_m2 / 1000;
    uint32_t cur2_frac = abs(s->rpm_m2) % 1000;

    snprintf(buf, sizeof(buf),
             "Cyc %6lu th=%+6d.%02d thdot=%+7d.%02d "
             "vel=%+4d.%03u pos=%+4d.%03u "
             "tgtRPM=%+4d.%03u RPM_m1=%+4d.%03u RPM_m2=%+4d.%03u PWM=%+4d\r\n",
             (unsigned long)s->loop_count,
             th_int, th_frac,
             thdot_int, thdot_frac,
             vel_int, vel_frac,
             pos_int, pos_frac,
             tgt_int, tgt_frac,
             cur1_int, cur1_frac,
			 cur2_int, cur2_frac,
             s->pwm);

    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}
