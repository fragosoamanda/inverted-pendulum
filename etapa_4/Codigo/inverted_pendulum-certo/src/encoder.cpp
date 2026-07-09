#include "encoder.h"
#include "stm32f4xx_hal.h"   // ajuste conforme sua MCU

// Referências aos timers já configurados (extern)
extern TIM_HandleTypeDef htim2;   // PWM motor 1
extern TIM_HandleTypeDef htim3;   // Encoder motor 1
extern TIM_HandleTypeDef htim5;   // Encoder motor 2
extern TIM_HandleTypeDef htim9;   // PWM motor 2
extern TIM_HandleTypeDef htim10;
/* ---------------- Variáveis globais ---------------- */
encoder_state_t enc_m1;
encoder_state_t enc_m2;
// No módulo encoder, adicione variáveis estáticas para as somas
static uint64_t sum_dt_m1 = 0;
static uint64_t sum_vdt_m1 = 0;

static uint64_t sum_dt_m2 = 0;
static uint64_t sum_vdt_m2 = 0;
// (mesmo para motor 2)

RingBuffer<int32_t, RPM_BUF_SIZE> rpm_buf_m1;
RingBuffer<uint16_t, RPM_BUF_SIZE> dt_buf_m1;
RingBuffer<int32_t, RPM_BUF_SIZE> rpm_buf_m2;
RingBuffer<uint16_t, RPM_BUF_SIZE> dt_buf_m2;

/* ---------------- Variáveis estáticas (internas) ---------------- */
static uint16_t last_enc1 = 0xFFFF, last_enc2 = 0xFFFF;
static uint16_t last_enc1_100ms = 0xFFFF, last_enc2_100ms = 0xFFFF;
static uint32_t last_pulse_ms_m1 = 0, last_pulse_ms_m2 = 0;
static uint16_t last_time_us_m1 = 0, last_time_us_m2 = 0;

/* ================================================================
 *  Constantes internas
 * ================================================================ */
#define RPM_FILTER_FACTOR      5       // 5% do valor novo, 95% do antigo
#define PULSE_TIMEOUT_MS       50      // tempo sem pulso para considerar parado

/* ---------------- Inicialização ---------------- */
void encoder_init(void)
{
    enc_m1 = (encoder_state_t){0};
    enc_m2 = (encoder_state_t){0};
}

/* ---------------- Processamento rápido (tempo entre pulsos) ---------------- */
static void process_motor_fast(uint8_t motor)
{
    encoder_state_t *enc = (motor == 0) ? &enc_m1 : &enc_m2;
    TIM_HandleTypeDef *htim_enc = (motor == 0) ? &htim3 : &htim5;
    uint16_t *last_enc = (motor == 0) ? &last_enc1 : &last_enc2;
    uint32_t *last_pulse_ms = (motor == 0) ? &last_pulse_ms_m1 : &last_pulse_ms_m2;
    uint16_t *last_time_us = (motor == 0) ? &last_time_us_m1 : &last_time_us_m2;

    uint16_t enc_now = __HAL_TIM_GET_COUNTER(htim_enc);
    uint32_t now_ms = HAL_GetTick();

    if (enc_now != *last_enc) {
        // PULSO DETECTADO
        int16_t diff = (int16_t)(enc_now - *last_enc);
        *last_enc = enc_now;
        *last_pulse_ms = now_ms;            // atualiza o instante do último pulso

        uint16_t time_now = __HAL_TIM_GET_COUNTER(&htim10);
        uint16_t dt_us = time_now - *last_time_us;
        *last_time_us = time_now;

        if (dt_us > 20) {   // ignora pulsos espúrios muito rápidos (<20 µs)
            uint64_t num = 60000000000ULL;
            uint64_t den = (uint64_t)ENCODER_CPR * dt_us;
            uint32_t rpm_mag = (uint32_t)(num / den);
            enc->rpm_instant = (diff > 0) ? (int32_t)rpm_mag : -(int32_t)rpm_mag;
		if (motor == 0) {
				// Remove o elemento mais antigo da soma se o buffer já estiver cheio
				if (dt_buf_m1.size() == RPM_BUF_SIZE) {
					sum_dt_m1 -= dt_buf_m1[dt_buf_m1.size() - 1];   // offset size-1 = mais antigo
				}
				sum_dt_m1 += dt_us;
				dt_buf_m1.push(dt_us);
				rpm_buf_m1.push(enc->rpm_instant);
			} else {
				if (dt_buf_m2.size() == RPM_BUF_SIZE) {
					sum_dt_m2 -= dt_buf_m2[dt_buf_m2.size() - 1];
				}
				sum_dt_m2 += dt_us;
				dt_buf_m2.push(dt_us);
				rpm_buf_m2.push(enc->rpm_instant);
			}
        }

        // se dt_us <= 20, mantém o último rpm_instant (evita picos irreais)
    } else if ((now_ms - *last_pulse_ms) > PULSE_TIMEOUT_MS) {
    enc->rpm_instant = 0;
    // Limpa os buffers para a média também ir a zero
    if (motor == 0) {
        rpm_buf_m1.clear();
        dt_buf_m1.clear();
		sum_dt_m1 = 0;
    } else {
        rpm_buf_m2.clear();
        dt_buf_m2.clear();
		sum_dt_m2 = 0;
    }
}


    // Filtro exponencial (sempre executado, com o rpm_instant atual)
    if (enc->rpm_filtered == 0) {
        enc->rpm_filtered = enc->rpm_instant;
    } else {
        enc->rpm_filtered = (enc->rpm_filtered * (100 - RPM_FILTER_FACTOR) +
                             enc->rpm_instant * RPM_FILTER_FACTOR) / 100;
    }
}

void encoder_process_fast(void)
{
    process_motor_fast(0);
    process_motor_fast(1);
}

/* ---------------- Processamento a cada 100 ms ---------------- */
void encoder_process_window(void)
{
    uint16_t enc1_now = __HAL_TIM_GET_COUNTER(&htim3);
    int16_t delta1 = (int16_t)(enc1_now - last_enc1_100ms);
    last_enc1_100ms = enc1_now;
    enc_m1.rpm_low = (int32_t)delta1 * 5000 / 11;

    uint16_t enc2_now = __HAL_TIM_GET_COUNTER(&htim5);
    int16_t delta2 = (int16_t)(enc2_now - last_enc2_100ms);
    last_enc2_100ms = enc2_now;
    enc_m2.rpm_low = (int32_t)delta2 * 5000 / 11;
}

/* ---------------- Conversão RPM (×1000) → mm/s ---------------- */
int32_t encoder_rpm_to_mm_s(int32_t rpm_scaled)
{
    // v = rpm * circunferência_mm / (60 * 1000)  => ajuste exato abaixo
    // WHEEL_CIRCUM_M = 0.20548 m = 205.48 mm → usamos 20548 (×100)
    // v_mm_s = (rpm_scaled * 20548) / 60000000
    // Simplificando: dividir numerador e denominador por 4 → 5137 / 15000000? Vamos recalcular:
    // rpm_scaled * 20548 / 60000000 = rpm_scaled * 5137 / 15000000
    // Ainda é um número grande, usaremos 64 bits.
    int64_t temp = (int64_t)rpm_scaled * 5137;
    return (int32_t)(temp / 1500000);   // note: 1500000 (milhões) para mm/s
}
int32_t encoder_get_sliding_rpm(uint8_t motor)
{
    uint64_t sum_dt = (motor == 0) ? sum_dt_m1 : sum_dt_m2;
    uint8_t  count  = (motor == 0) ? dt_buf_m1.size() : dt_buf_m2.size();
    RingBuffer<int32_t, RPM_BUF_SIZE> &rpm_buf = (motor == 0) ? rpm_buf_m1 : rpm_buf_m2;

    if (count == 0 || sum_dt == 0) return 0;

    // Magnitude (igual à anterior)
    uint64_t num = (uint64_t)count * 60000000000ULL;
    uint64_t den = (uint64_t)ENCODER_CPR * sum_dt;
    int32_t rpm_mag = (int32_t)(num / den);

    // Determinar o sinal predominante no buffer de RPM instantâneo
    int32_t sum_rpm = 0;
    for (uint8_t i = 0; i < count; i++) {
        sum_rpm += rpm_buf[i];       // rpm_buf já guarda valores com sinal
    }
    int32_t rpm_signed = (sum_rpm >= 0) ? rpm_mag : -rpm_mag;
    return rpm_signed;
}

void motor1_set(int16_t pwm)
{
    if (pwm >= 0) {
        // Sentido positivo
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint32_t)pwm);
    } else {
        // Sentido negativo
        int16_t abs_pwm = -pwm;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint32_t)abs_pwm);
    }
}

void motor2_set(int16_t pwm)
{
    if (pwm >= 0) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, (uint32_t)pwm);
    } else {
        int16_t abs_pwm = -pwm;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, (uint32_t)abs_pwm);
    }
}

void motor2_parar(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
}


void motor1_parar(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 0);
}
