#include "mpu_data.h"
#include "MPU6050/MPU6050.h"
#include "ArduinoWrapper.h"
#include "init.h"
#include <cmath>

MPU6050 mpu(0x69);

// ============================ VARIÁVEIS GLOBAIS ============================
q31_t mpu_pitch  = 0;
q31_t mpu_roll   = 0;
q31_t mpu_yaw    = 0;
q31_t mpu_gyro_x = 0;
q31_t mpu_gyro_y = 0;
q31_t mpu_gyro_z = 0;

RingBuffer<q31_t, MPU_BUF_SIZE> buf_pitch;
RingBuffer<q31_t, MPU_BUF_SIZE> buf_roll;
RingBuffer<q31_t, MPU_BUF_SIZE> buf_yaw;
RingBuffer<q31_t, MPU_BUF_SIZE> buf_gyro_x;
RingBuffer<q31_t, MPU_BUF_SIZE> buf_gyro_y;
RingBuffer<q31_t, MPU_BUF_SIZE> buf_gyro_z;

// ============================ CALIBRAÇÃO ============================
int16_t gyro_offsets[3] = {GYRO_OFFSET_X,GYRO_OFFSET_Y,GYRO_OFFSET_Z};   // X, Y, Z

void calibrate_gyro(void) {
    const int samples = 2048;          // potência de 2 (2^11)
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    int16_t gx, gy, gz;

    for (int i = 0; i < samples; i++) {
        mpu.getRotation(&gx, &gy, &gz);
        sum_x += gx;
        sum_y += gy;
        sum_z += gz;
        HAL_Delay(1);                  // 1 ms → total ~2 segundos
    }

    // Divisão por 2048 via deslocamento (>> 11)
    gyro_offsets[0] = (int16_t)(sum_x >> 11);
    gyro_offsets[1] = (int16_t)(sum_y >> 11);
    gyro_offsets[2] = (int16_t)(sum_z >> 11);
}

// ============================ INICIALIZAÇÃO ============================
void mpu_init(void)
{
    print("[MPU] Inicializando MPU6050...\r\n");
    mpu.initialize();

    int retries = 20;   // timeout ≈ 5 segundos
    while (!mpu.testConnection() && retries > 0) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        HAL_Delay(250);

        // Reset completo do barramento I²C
        HAL_I2C_MspDeInit(&hi2c1);
        __HAL_RCC_GPIOB_CLK_DISABLE();
        HAL_Delay(10);
        __HAL_RCC_GPIOB_CLK_ENABLE();
        HAL_I2C_MspInit(&hi2c1);

        // Reset do sensor via biblioteca (substitui o uso de MPU_ADDR)
        mpu.reset();
        HAL_Delay(100);
        mpu.initialize();

        retries--;
    }

    if (!mpu.testConnection()) {
        print("\r\n[MPU] ERRO: MPU6050 nao responde.\r\n");
        Error_Handler();
        return;
    }

    print("\r\n[MPU] Conectado!\r\n");


    // Configuração dos fundos de escala e filtro
    mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_2000);   // ±2000°/s margem de segurança e resolução mais do que suficiente
    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);     // ±2g
    mpu.setDLPFMode(MPU6050_DLPF_BW_98);                // 98 Hz (gyro), 94 Hz (accel)

    // Calibração do giroscópio (offset)
    print("[MPU] Calibrando giroscopio...\r\n");
if (systemFlags.calibrate) {
        print("[MPU] Calibrando giroscopio...\r\n");
        calibrate_gyro();
        systemFlags.calibrate= false;   // não recalibra até pedido explícito
    } else {
        // Usar offsets hardcoded
        gyro_offsets[0] = GYRO_OFFSET_X;
        gyro_offsets[1] = GYRO_OFFSET_Y;
        gyro_offsets[2] = GYRO_OFFSET_Z;
        print("[MPU] Usando offsets hardcoded.\r\n");
    }
    print("[MPU] Offsets: X=%d Y=%d Z=%d\r\n",
          gyro_offsets[0], gyro_offsets[1], gyro_offsets[2]);

    print("[MPU] Pronto.\r\n\r\n");
}

// ============================ ATUALIZAÇÃO RÁPIDA ============================
void mpu_update(void)
{
    int16_t ax, ay, az, gx, gy, gz;

    // Lê uma amostra completa (acel + giro)
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Remove offsets do giroscópio
    gx -= gyro_offsets[0];
    gy -= gyro_offsets[1];
    gz -= gyro_offsets[2];

    // --- Velocidades angulares em Q31 ---
    // Com fundo de escala ±2000°/s, um deslocamento de 16 bits
    // transforma directamente o valor raw em Q31 (32767 → ≈0.9999)
    mpu_gyro_x = (q31_t)((int32_t)gx << 16);
    mpu_gyro_y = (q31_t)((int32_t)gy << 16);
    mpu_gyro_z = (q31_t)((int32_t)gz << 16);

    // --- Ângulos de inclinação pelo acelerómetro ---
    // Converte para g
    float ax_g = ax / 16384.0f;   // ±2g → 16384 LSB/g
    float ay_g = ay / 16384.0f;
    float az_g = az / 16384.0f;

    // Pitch (rotação em torno de X) – ±90° → ±1 Q31
    float pitch_rad = atan2f(ax_g, az_g);
    mpu_pitch = (q31_t)(pitch_rad * (2.0f / (float)M_PI) * 2147483648.0f);

    // Roll (rotação em torno de Y) – ±180° → ±1 Q31
    float roll_rad = atan2f(ay_g, az_g);
    mpu_roll  = (q31_t)(roll_rad * (1.0f / (float)M_PI) * 2147483648.0f);

    // Yaw – integrado do giroscópio Z (deriva inevitável sem uma referência como a de um magnetómetro)
    static float yaw_deg = 0.0f;
    float gyro_z_dps = gz / 16.4f;               // LSB → °/s (±2000)
    yaw_deg += gyro_z_dps * 0.005f;               // dt = 5 ms
    // Normaliza para Q31 (±180° = ±1)
    mpu_yaw = (q31_t)((yaw_deg / 180.0f) * 2147483648.0f);

    // Alimenta buffers
    buf_pitch.push(mpu_pitch);
    buf_roll.push(mpu_roll);
    buf_yaw.push(mpu_yaw);
    buf_gyro_x.push(mpu_gyro_x);
    buf_gyro_y.push(mpu_gyro_y);
    buf_gyro_z.push(mpu_gyro_z);
}
