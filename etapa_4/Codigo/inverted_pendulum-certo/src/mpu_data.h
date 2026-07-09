#ifndef MPU_DATA_H
#define MPU_DATA_H

#include <stdint.h>
#include "RingBuffer.h"
#include "aux.h"          // q31_t, Q31_TO_F, etc.

#define MPU_BUF_SIZE 20

// Offsets determinados experimentalmente
#define GYRO_OFFSET_X   -65
#define GYRO_OFFSET_Y     27
#define GYRO_OFFSET_Z     26

/*
 * =====================================================================
 *  NORMALIZAÇÃO DOS VALORES Q31
 * =====================================================================
 *
 *  ÂNGULOS:
 *    mpu_pitch  → ±90°  ≡ ±1.0  (Q31)
 *    mpu_roll   → ±180° ≡ ±1.0  (Q31)
 *    mpu_yaw    → ±180° ≡ ±1.0  (Q31) anglo do pendulo em torno do exio z (usado para corrigir variações e garantir que o pendulo fique reto)
 *										Como a deriva é inevitável sem uma referência como a de um magnetómetro, poderá não ser usado
 *    Pitch (inclinação frente/trás) é o ângulo fundamental para o equilíbrio.

    Roll (inclinação lateral) normalmente não é controlado ativamente – a estrutura é simétrica e o movimento lateral é pequeno ou corrigido indiretamente pelo equilíbrio.

    Yaw (rotação em torno do eixo vertical) é útil se quiseres fazer curvas ou corrigir a orientação, mas não é essencial para o equilíbrio básico. Pode ser ignorado se o objetivo for apenas manter‑se de pé e andar em linha reta.
 *   VELOCIDADES ANGULARES:
 *    mpu_gyro_x → ±2000°/s ≡ ±1.0  (Q31)   (derivada do roll)
 *    mpu_gyro_y → ±2000°/s ≡ ±1.0  (Q31)   (derivada do pitch)
 *    mpu_gyro_z → ±2000°/s ≡ ±1.0  (Q31)   (derivada do yaw)
 *.
 * =====================================================================
 */
// --- Ângulos (Q31 normalizados) ---
extern q31_t mpu_pitch;      // pitch      ±90°  = ±1
extern q31_t mpu_roll;       // roll       ±180° = ±1
extern q31_t mpu_yaw;        // yaw        ±180° = ±1
// --- Velocidades angulares (Q31 normalizadas) --
extern q31_t mpu_gyro_x;
extern q31_t mpu_gyro_y;
extern q31_t mpu_gyro_z;


// Buffers circulares
extern RingBuffer<q31_t, MPU_BUF_SIZE> buf_pitch;
extern RingBuffer<q31_t, MPU_BUF_SIZE> buf_roll;
extern RingBuffer<q31_t, MPU_BUF_SIZE> buf_yaw;
extern RingBuffer<q31_t, MPU_BUF_SIZE> buf_gyro_x;
extern RingBuffer<q31_t, MPU_BUF_SIZE> buf_gyro_y;
extern RingBuffer<q31_t, MPU_BUF_SIZE> buf_gyro_z;


void mpu_update(void);
void mpu_init(void);
#endif
