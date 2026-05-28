// init.h
// Declaracoes de handles e funcoes de inicializacao

#ifndef INIT_H
#define INIT_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

// ─── HANDLES ────────────────────────────────────────────
extern ADC_HandleTypeDef  hadc1;
extern I2C_HandleTypeDef  hi2c1;
extern RTC_HandleTypeDef  hrtc;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef  htim2;   // PWM motor 1 (PB3 = TIM2_CH2)
extern TIM_HandleTypeDef  htim3;   // Encoder motor 1 (PA6=TIM3_CH1, PA7=TIM3_CH2)
extern TIM_HandleTypeDef  htim4;   // Timer controle (interrupcao 5ms)
extern TIM_HandleTypeDef  htim5;   // Encoder motor 2 (PA0=TIM5_CH1, PA1=TIM5_CH2)
extern TIM_HandleTypeDef  htim9;   // PWM motor 2 (PA2 = TIM9_CH1)

// ─── FUNCOES DE INICIALIZACAO ───────────────────────────
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_ADC1_Init(void);
void MX_I2C1_Init(void);
void MX_RTC_Init(void);
void MX_USART1_UART_Init(void);
void MX_TIM2_Init(void);
void MX_TIM3_Init(void);
void MX_TIM4_Init(void);
void MX_TIM5_Init(void);
void MX_TIM9_Init(void);

#ifdef __cplusplus
}
#endif

#endif // INIT_H