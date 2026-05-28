// lqr_controller.h
// Controlador LQR para pendulo invertido
// Motor: JGB37-520 12V | Encoder: 1320 cnt/rev | Roda: 65.4mm
// Ganhos K discretos (Ts=5ms): [-3.0061, -4.7601, 49.2508, 7.7704]

#ifndef LQR_CONTROLLER_H
#define LQR_CONTROLLER_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif
// coisas a serem feitas, parar de usar float, usar ponto fixo
/*
Q16.15 num int32:
bit 31         bit 16  bit 15         bit 0
[sinal | inteira 16 bits] . [fração 15 bits]

Q6.25 num int32:
bit 31         bit 6  bit 15         bit 0
[sinal | inteira 6 bits] . [fração 25 bits]
*/

// ─── Q6.25 — sinais (theta, vel, pos) ──────────────────
typedef int32_t q6_t;
#define Q6_SCALE    (1L << 25)                           // 33554432
#define Q6(x)       ((q6_t)((x) * Q6_SCALE))            // literal → Q6.25
#define Q6_MUL(a,b) ((q6_t)(((int64_t)(a) * (b)) >> 25))// produto → Q6.25
#define Q6_DIV(a,b) ((q6_t)(((int64_t)(a) << 25) / (b)))// divisão → Q6.25
#define Q6_TO_F(x)  ((float)(x) / Q6_SCALE)             // só debug
#define Q6_TO_INT(x)((x) >> 25)                         // parte inteira

// ─── Q16.16 — ganhos (K1..K4) ──────────────────────────
typedef int32_t q16_t;
#define Q16_SCALE    (1L << 16)                            // 65536
#define Q16(x)       ((q16_t)((x) * Q16_SCALE))           // literal → Q16.16
#define Q16_MUL(a,b) ((q16_t)(((int64_t)(a) * (b)) >> 16))// produto → Q16.16
#define Q16_DIV(a,b) ((q16_t)(((int64_t)(a) << 16) / (b)))// divisão → Q16.16
#define Q16_TO_F(x)  ((float)(x) / Q16_SCALE)             // só debug
#define Q16_TO_INT(x)((x) >> 16)                          // parte inteira

// ─── multiplicação cruzada ganho × sinal → Q6.25 ───────
#define Q6_MUL_KS(k, s) ((q6_t)(((int64_t)(k) * (s)) >> 16))
#define Q16_MUL_KS(k, s) ((q6_t)(((int64_t)(k) * (s)) >> 25))
// ─── MODO DE OPERACAO ───────────────────────────────────
// Comente DEBUG_MODE para ativar o motor
// Deixe definido para testar sem ligar o driver
//#define DEBUG_MODE

// ─── FUNCOES PUBLICAS ───────────────────────────────────
void lqr_init(void);
void lqr_loop(void);   // chamada pela interrupcao do TIM4

// Estados acessiveis externamente (para debug)
q6_t lqr_get_theta(void);
q6_t lqr_get_theta_dot(void);
q6_t lqr_get_pos(void);
q6_t lqr_get_vel(void);
q16_t lqr_get_u(void);

#ifdef __cplusplus
}
#endif

#endif // LQR_CONTROLLER_H