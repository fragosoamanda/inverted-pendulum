// init.cpp
// Inicializacoes de todos os perifericos
//
// CORRECAO: MX_ADC1_Init() agora usa ADC_CHANNEL_4 (PA4) em vez de
// ADC_CHANNEL_0 (PA0). PA0 e PA1 sao usados pelo TIM5 como encoder 2;
// inicializar o ADC no canal 0 reconfigurava PA0 como entrada analogica,
// impedindo o TIM5 de funcionar e o motor 2 de ser acionado.

#include "init.h"
#include "main.h"
#include <stdio.h>
#include <errno.h>

// ─── DEFINICAO DOS HANDLES ──────────────────────────────
ADC_HandleTypeDef  hadc1;
I2C_HandleTypeDef  hi2c1;
RTC_HandleTypeDef  hrtc;
UART_HandleTypeDef huart1;
TIM_HandleTypeDef  htim2;   // PWM motor 1
TIM_HandleTypeDef  htim3;   // Encoder motor 1
TIM_HandleTypeDef  htim4;   // Timer controle
TIM_HandleTypeDef  htim5;   // Encoder motor 2
TIM_HandleTypeDef  htim9;   // PWM motor 2

TIM_HandleTypeDef  htim10;   // us counter

// ════════════════════════════════════════════════════════
//  SYSTEM CLOCK — 48 MHz (HSE 25MHz)
// ════════════════════════════════════════════════════════
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 192;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

// ════════════════════════════════════════════════════════
//  GPIO
//  LED: PC13
//  Motor 1 dir: PB4 (IN1), PB5 (IN2)
//  Motor 2 dir: PB8 (IN3), PB9 (IN4)
//  Encoder 1:   PA6 (TIM3_CH1), PA7 (TIM3_CH2)
//  Encoder 2:   PA0 (TIM5_CH1), PA1 (TIM5_CH2)
//  PWM motor 2: PA2 (TIM9_CH1)
// ════════════════════════════════════════════════════════
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // ── LED PC13 ─────────────────────────────────────────
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = LED_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

    // ── PA5 saida generica ───────────────────────────────
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ── PA0 / PA1 — TIM5_CH1 / TIM5_CH2 (Encoder 2) ────
    // IMPORTANTE: deve ser configurado DEPOIS de MX_ADC1_Init() para
    // garantir que o HAL do ADC nao sobrescreva estes pinos como
    // entrada analogica. Ver ordem de chamada em main.cpp.
    GPIO_InitStruct.Pin       = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ── PA2 — TIM9_CH1 (PWM motor 2) ────────────────────
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM9;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ── PA6 / PA7 — TIM3_CH1 / TIM3_CH2 (Encoder 1) ────
    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// ── PA4 — ADC1_CH4 (Leitura da bateria) ──────────────
	GPIO_InitStruct.Pin       = GPIO_PIN_4;
	GPIO_InitStruct.Mode      = GPIO_MODE_ANALOG;  // Modo analógico (essencial!)
	GPIO_InitStruct.Pull      = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ── PB4 / PB5 — IN1 / IN2 (direcao motor 1) ─────────
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin       = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // ── PB8 / PB9 — IN3 / IN4 (direcao motor 2) ─────────
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

// ════════════════════════════════════════════════════════
//  ADC1 — CORRECAO: canal movido para PA4 (ADC_CHANNEL_4)
//
//  O canal original era ADC_CHANNEL_0 (PA0), que conflita com
//  TIM5_CH1 (encoder 2). PA4 esta livre e nao interfere com
//  nenhum outro periferico usado neste projeto.
//  Se voce nao usa o ADC, pode simplesmente nao chamar
//  MX_ADC1_Init() em main.cpp.
// ════════════════════════════════════════════════════════
void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

    // CORRECAO: ADC_CHANNEL_4 = PA4 (nao conflita com encoders nem PWM)
    sConfig.Channel      = ADC_CHANNEL_4;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();
}

// ════════════════════════════════════════════════════════
//  I2C1 — PB6=SCL, PB7=SDA (MPU6050)
// ════════════════════════════════════════════════════════
void MX_I2C1_Init(void)
{
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 400000;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
}

// ════════════════════════════════════════════════════════
//  RTC
// ════════════════════════════════════════════════════════
void MX_RTC_Init(void)
{
    hrtc.Instance            = RTC;
    hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv   = 127;
    hrtc.Init.SynchPrediv    = 255;
    hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) Error_Handler();
}

// ════════════════════════════════════════════════════════
//  USART1 — PA9=TX, PA10=RX, 115200 bps
// ════════════════════════════════════════════════════════
void MX_USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

// ════════════════════════════════════════════════════════
//  TIM2 — PWM motor 1 (PB3 = TIM2_CH2)
// ════════════════════════════════════════════════════════
void MX_TIM2_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC   = {0};
    GPIO_InitTypeDef   GPIO_InitStruct = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 0;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 999;					//aumentei em 2x a frequencia para ver se diminuia a ondulação no motor
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) Error_Handler();

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) Error_Handler();
}

// ════════════════════════════════════════════════════════
//  TIM3 — Encoder motor 1 (PA6=TIM3_CH1, PA7=TIM3_CH2)
// ════════════════════════════════════════════════════════
void MX_TIM3_Init(void)
{
    TIM_Encoder_InitTypeDef sConfig    = {0};
    GPIO_InitTypeDef        GPIO_InitStruct = {0};

    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 0;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = 65535;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    sConfig.EncoderMode  = TIM_ENCODERMODE_TI12;
    sConfig.IC1Polarity  = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter    = 5;
    sConfig.IC2Polarity  = TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter    = 5;

    if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK) Error_Handler();

    __HAL_TIM_SET_COUNTER(&htim3, 32768);
}

// ════════════════════════════════════════════════════════
//  TIM9 — PWM motor 2 (PA2 = TIM9_CH1)
// ════════════════════════════════════════════════════════
void MX_TIM9_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC   = {0};
    GPIO_InitTypeDef   GPIO_InitStruct = {0};

    __HAL_RCC_TIM9_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM9;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    htim9.Instance               = TIM9;
    htim9.Init.Prescaler         = 0;
    htim9.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim9.Init.Period            = 999;					//aumentei em 2x a frequencia para ver se diminuia a ondulação no motor
    htim9.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim9) != HAL_OK) Error_Handler();

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
}

// ════════════════════════════════════════════════════════
//  TIM5 — Encoder motor 2 (PA0=TIM5_CH1, PA1=TIM5_CH2)
// ════════════════════════════════════════════════════════
void MX_TIM5_Init(void)
{
    TIM_Encoder_InitTypeDef sConfig    = {0};
    GPIO_InitTypeDef        GPIO_InitStruct = {0};

    __HAL_RCC_TIM5_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Reafirma a configuracao AF dos pinos mesmo que ja tenha sido
    // feita no MX_GPIO_Init() — garante estado correto.
    GPIO_InitStruct.Pin       = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    htim5.Instance               = TIM5;
    htim5.Init.Prescaler         = 0;
    htim5.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim5.Init.Period            = 0xFFFFFFFF;
    htim5.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    sConfig.EncoderMode  = TIM_ENCODERMODE_TI12;
    sConfig.IC1Polarity  = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter    = 5;
    sConfig.IC2Polarity  = TIM_ICPOLARITY_FALLING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter    = 5;

    if (HAL_TIM_Encoder_Init(&htim5, &sConfig) != HAL_OK) Error_Handler();

// HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);
//     __HAL_TIM_SET_COUNTER(&htim5, 0x7FFFFFFF);
}

// ════════════════════════════════════════════════════════
//  TIM4 — Timer de controle LQR (interrupcao a cada 5ms)
//  Clock TIM4 = APB1 * 2 = 24MHz * 2 = 48MHz
//  Periodo = 48.000.000 / (4799+1) / (49+1) = 200Hz = 5ms
// ════════════════════════════════════════════════════════
void MX_TIM4_Init(void)
{
    __HAL_RCC_TIM4_CLK_ENABLE();

    htim4.Instance               = TIM4;
    htim4.Init.Prescaler         = 4799;
    htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim4.Init.Period            = 49;
    htim4.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) Error_Handler();

    HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

/// ════════════════════════════════════════════════════════
//  TIM10 — Timer de precisão para RPM do Motor 1 (1µs)
//  Clock TIM10 = APB2 (geralmente 84MHz)
//  Prescaler = 83 -> 84MHz / 84 = 1MHz -> 1µs
//  Period = 0xFFFF (16 bits, estoura em 65.5ms)
// ════════════════════════════════════════════════════════
void MX_TIM10_Init(void)
{
    __HAL_RCC_TIM10_CLK_ENABLE();

    htim10.Instance               = TIM10;
    htim10.Init.Prescaler         = 47;          // Ajuste para 1µs (se APB2=84MHz)
    htim10.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim10.Init.Period            = 0xFFFF;
    htim10.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim10) != HAL_OK) Error_Handler();

    // NÃO habilita interrupção! Apenas leitura do contador.
    HAL_TIM_Base_Start(&htim10);
}
