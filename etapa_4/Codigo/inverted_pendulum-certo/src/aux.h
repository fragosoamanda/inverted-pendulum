#ifndef __AUX_H
#define __AUX_H

#include <stdbool.h>
/*
Q16.15 num int32:
bit 31         bit 16  bit 15         bit 0
[sinal | inteira 16 bits] . [fração 15 bits]

Q7.25 num int32:
bit 31         bit 25  bit 15         bit 0
[sinal | inteira 6 bits] . [fração 25 bits]

Q1.31 num int32:
bit 31                bit 30         bit 0
[sinal | inteira 0 bits] . [fração 31 bits]
*/

// ─── Q7.25 — sinais (theta, vel, pos) ──────────────────
typedef int32_t q25_t;
#define Q25_SCALE    (1L << 25)                           // 33554432
#define Q25(x)       ((q25_t)((x) * Q25_SCALE))            // literal → Q25.25
#define Q25_MUL(a,b) ((q25_t)(((int64_t)(a) * (b)) >> 25))// produto → Q25.25
#define Q25_DIV(a,b) ((q25_t)(((int64_t)(a) << 25) / (b)))// divisão → Q25.25
#define Q25_TO_F(x)  ((float)(x) / Q25_SCALE)             // só debug
#define Q25_TO_INT(x)((x) >> 25)                         // parte inteira

// ─── Q1.31 — grandezas normalizadas ──────────────────
typedef int32_t q31_t;

#define Q31_SCALE    (1UL << 31)                     		// 2³¹
#define Q31(x)       ((q31_t)((x) * Q31_SCALE))       			// literal → Q16.16
#define Q31_MUL(a,b) ((q31_t)(((int64_t)(a) * (b)) >> 31))
#define Q31_DIV(a,b) ((q31_t)(((int64_t)(a) << 31) / (b)))	// b != 0
#define Q31_TO_F(x)  ((float)(x) / Q31_SCALE)
#define Q31_TO_INT(x)((x) >> 31)                 			// inteiro (0 ou -1)

// Constante para 1.0 (máximo positivo)
#define Q31_ONE      ((Q31_t)0x7FFFFFFF)                   //maximo representavel 1-2^-31


// ─── Q16.16 — ganhos (K1..K4) ──────────────────────────
typedef int32_t q16_t;
#define Q16_SCALE    (1L << 16)                            // 2¹⁶
#define Q16(x)       ((q16_t)((x) * Q16_SCALE))           // literal → Q16.16
#define Q16_MUL(a,b) ((q16_t)(((int64_t)(a) * (b)) >> 16))// produto → Q16.16
#define Q16_DIV(a,b) ((q16_t)(((int64_t)(a) << 16) / (b)))// divisão → Q16.16
#define Q16_TO_F(x)  ((float)(x) / Q16_SCALE)             // só debug
#define Q16_TO_INT(x)((x) >> 16)                          // parte inteira

// ─── multiplicação cruzada ganho × sinal → Q25.25 ───────
#define Q31_MUL_KS(k, s) ((Q31_t)(((int64_t)(k) * (s)) >> 1))
#define Q25_MUL_KS(k, s) ((Q25_t)(((int64_t)(k) * (s)) >> 25))
#define Q16_MUL_KS(k, s) ((q16_t)(((int64_t)(k) * (s)) >> 16))

typedef union {
	struct {
		bool		encoder_update				: 1;
		bool		control_loop				: 1;
		bool		mpuReady					: 1;
		bool		mpu_recive					: 1;
		bool		calibrate					: 1;
		uint8_t		unusedFlags					: 3;
	};
	uint8_t allFlags;
} systemFlags_t;
extern volatile systemFlags_t systemFlags;

#endif /* __MAIN_H */
