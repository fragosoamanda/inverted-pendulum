/**
 * main.cpp
 * Pendulo Invertido — Controle LQR
 * STM32F411 | Motor JGB37-520 | Encoder Hall 1320cnt/rev | MPU6050 | L298N
 *
 * CORRECAO: MX_ADC1_Init() movido para ANTES de MX_GPIO_Init().
 * O HAL do ADC reconfigurava PA0 como entrada analogica, corrompendo
 * a configuracao AF_PP do TIM5_CH1 (encoder 2) feita pelo MX_GPIO_Init.
 * Com a nova ordem, MX_GPIO_Init() e MX_TIM5_Init() sobrescrevem
 * corretamente o pino PA0 para funcao alternativa do encoder.
 */

#include "init.h"
#include "ArduinoWrapper.h"
#include "lqr_controller.h"

// Variaveis do MPU6050 DMP
#include "MPU6050/MPU6050_6Axis_MotionApps_V6_12.h"

MPU6050 mpu(0x69);

bool     dmpReady  = false;
uint8_t  devStatus = 0;
uint16_t packetSize;
uint8_t  fifoBuffer[64];
Quaternion  q;
VectorFloat gravity;
float ypr[3];

static void dmp_init(void)
{
    print("[DMP] Testando conexao MPU6050...\r\n");
    mpu.initialize();

    while (!mpu.testConnection()) {
        HAL_I2C_MspDeInit(&hi2c1);
        __HAL_RCC_GPIOB_CLK_DISABLE();
        HAL_Delay(250);
        HAL_I2C_MspInit(&hi2c1);
        HAL_Delay(250);
        print("\r\n falha de comunicacao");
    }
    print("\r\n[DMP] MPU6050 conectado!\r\n");

    mpu.setXGyroOffset(30);
    mpu.setYGyroOffset(58);
    mpu.setZGyroOffset(21);
    mpu.setXAccelOffset(0);
    mpu.setYAccelOffset(0);
    mpu.setZAccelOffset(0);

    print("[DMP] Inicializando DMP...\r\n");
    devStatus = mpu.dmpInitialize();

    if (devStatus == 0) {
        mpu.CalibrateAccel(6);
        mpu.CalibrateGyro(6);
        mpu.PrintActiveOffsets();
        mpu.setDMPEnabled(true);
        dmpReady   = true;
        packetSize = mpu.dmpGetFIFOPacketSize();
        print("[DMP] DMP pronto!\r\n");
    } else {
        print("[DMP] ERRO ao inicializar DMP (code %d)\r\n", devStatus);
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    // ── CORRECAO: ADC primeiro, GPIO depois ──────────────────────────────
    // MX_ADC1_Init() chama internamente HAL_ADC_MspInit(), que pode
    // reconfigura PA0 como entrada analogica (ADC_CHANNEL_0).
    // Ao inicializar o ADC antes do GPIO, o MX_GPIO_Init() subsequente
    // restaura PA0/PA1 como AF_PP para o TIM5 (encoder 2), garantindo
    // que o pino esteja correto quando MX_TIM5_Init() for chamado.
    // ─────────────────────────────────────────────────────────────────────
    MX_ADC1_Init();       // 1. ADC (pode sobrescrever PA0 — nao importa ainda)
    MX_GPIO_Init();       // 2. GPIO (restaura PA0/PA1 como AF TIM5 e PA2 como AF TIM9)

    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_USB_DEVICE_Init();
    MX_TIM2_Init();       // PWM motor 1  (PB3)
    MX_TIM3_Init();       // Encoder 1    (PA6, PA7)
    MX_TIM4_Init();       // Timer controle 5ms
    MX_TIM5_Init();       // Encoder 2    (PA0, PA1)  — PA0 ja esta como AF_PP aqui
    MX_TIM9_Init();       // PWM motor 2  (PA2)

    HAL_Delay(20); //

    dmp_init();

    print("\r\n==========================================\r\n");
    print("  PENDULO INVERTIDO — CONTROLE LQR\r\n");
    print("  STM32F411 | JGB37-520 | MPU6050\r\n");
    print("==========================================\r\n\r\n");
    lqr_init();



    print("\r\n[MAIN] Loop principal iniciado.\r\n");
    print("[MAIN] Controle rodando na interrupcao TIM4 (5ms).\r\n\r\n");

    uint32_t last_print      = 0;
    uint32_t last_enc1_check = 0;
    uint32_t last_enc2_check = 0;

    while (1)
    {
        uint32_t now = HAL_GetTick();

        if (now - last_print >= 500) {
            last_print = now;

            if (dmpReady && mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetGravity(&gravity, &q);
                mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

                char buf[128];
                snprintf(buf, sizeof(buf),
                    "[MAIN] DMP ypr: %.2f / %.2f / %.2f deg"
                    " | LQR th=%.2f deg u=%.3f N\r\n",
                    ypr[0] * 180.0f / 3.14159f,
                    ypr[1] * 180.0f / 3.14159f,
                    ypr[2] * 180.0f / 3.14159f,
                    lqr_get_theta() * 180.0f / 3.14159f,
                    lqr_get_u()
                );
                CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
            }
        }

        if (now - last_enc1_check > 1000) {
            last_enc1_check = now;
            uint16_t enc1_val = __HAL_TIM_GET_COUNTER(&htim3);
            print("Encoder1: %u\r\n", enc1_val);
        }

        if (now - last_enc2_check > 1000) {
            last_enc2_check = now;
            uint32_t enc2_val = __HAL_TIM_GET_COUNTER(&htim5);
            print("Encoder2: %lu\r\n", enc2_val);
        }

        HAL_Delay(10);
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(100);
    }
}