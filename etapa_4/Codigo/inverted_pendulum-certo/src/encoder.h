#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include "RingBuffer.h"
// Parâmetros físicos
#define ENCODER_CPR         1320
#define WHEEL_DIAM_M        0.0654f
#define WHEEL_CIRCUM_M      0.20548f

// Metros por contagem de encoder (em formato Q25)
// M_PER_COUNT = WHEEL_CIRCUM_M / ENCODER_CPR
#define M_PER_COUNT_Q25     ((int32_t)((WHEEL_CIRCUM_M / ENCODER_CPR) * (1u << 25) + 0.5f))

/* Frequência da janela de aquisição de baixa resolução (100 ms) */
#define ENCODER_WINDOW_MS   100
#define ENCODER_WINDOW_HZ   (1000 / ENCODER_WINDOW_MS)   // 10 Hz

/* Tamanho do buffer circular para RPM (usado internamente no módulo) */
#define RPM_BUF_SIZE  10

// Estrutura que reúne as medições de um motor (acesso direto)
typedef struct {
    int32_t rpm_instant;    // RPM * 1000 (tempo entre pulsos)
    int32_t rpm_filtered;   // RPM * 1000 (filtro exponencial)
    int32_t rpm_low;        // RPM * 1000 (média a cada 100 ms)
} encoder_state_t;

// Buffers para RPM (10 amostras cada)
extern RingBuffer<int32_t, RPM_BUF_SIZE> rpm_buf_m1;
extern RingBuffer<uint16_t, RPM_BUF_SIZE> dt_buf_m1;
extern RingBuffer<int32_t, RPM_BUF_SIZE> rpm_buf_m2;
extern RingBuffer<uint16_t, RPM_BUF_SIZE> dt_buf_m2;
// Variáveis globais acessíveis por qualquer arquivo
extern encoder_state_t enc_m1;
extern encoder_state_t enc_m2;

// Funções de processamento
void encoder_init(void);
void encoder_process_fast(void);
void encoder_process_window(void);   // chamar a cada 100 ms

// Conversão RPM → velocidade linear (mm/s), opcional mas útil
int32_t encoder_rpm_to_mm_s(int32_t rpm_scaled);

int32_t encoder_get_sliding_rpm(uint8_t motor);
#endif
