

#include "lqr_controller.h"
#include "ArduinoWrapper.h"
#include <math.h>
#include <string.h>
#include "aux.h"


#include "lqr_controller.h"
#include "speed_controller.h"     // <-- incluir o PI
#include "encoder.h"              // para obter o RPM atual

extern speed_controller_t motor1;   // definido no main.c


LQR_State s;


void lqr_init(void)
{
    memset(&s, 0, sizeof(LQR_State));
    s.ativo = 0;

    print("[LQR] Iniciando MPU6050...\r\n");
    mpu_init();

    print("[LQR] Calibrando giroscopio (mantenha parado)...\r\n");
    HAL_Delay(100);
    mpu_calibrate();
    print("[LQR] Offset gyro: %.4f deg/s\r\n", Q16_TO_F(s.gyro_offset));


    __HAL_TIM_SET_COUNTER(&htim3, 0);
    s.enc1_prev = 0;
    __HAL_TIM_SET_COUNTER(&htim5, 0);
    s.enc2_prev = 0;

    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
    HAL_TIM_Base_Start_IT(&htim4);

    motor1_parar();
    motor2_parar();
    s.ativo = 1;

    print("[LQR] Pronto!\r\n\r\n");
}


void lqr_loop(void)
{
    if (!s.ativo) return;

    q16_t ax, ay, az, gx, gy, gz;
    mpu_read(&ax, &ay, &az, &gx, &gy, &gz);
    encoder_update();
    q16_t gy_dps = gy - s.gyro_offset;
    filtro_complementar(ax, ay, az, gy_dps);
    controle_lqr();
	s.loop_count++;
	if (s.loop_count % 10 == 0) {
		char dbg[64];
		snprintf(dbg, sizeof(dbg), "ax=%d az=%d\r\n", (int)ax, (int)az);
		CDC_Transmit_FS((uint8_t*)dbg, strlen(dbg));
	}
}


// q25_t lqr_get_theta(void)     { return s.theta; }
// q25_t lqr_get_theta_dot(void) { return s.theta_dot; }
// q25_t lqr_get_pos(void)       { return s.pos; }
// q25_t lqr_get_vel(void)       { return s.vel; }
// q16_t lqr_get_u(void)        { return s.u; }


void mpu_init(void)
{
    uint8_t data;
    uint8_t who;
    HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR, 0x75, 1, &who, 1, 100);
    if (who != 0x69 && who != 0x68) {
        print("[LQR] ERRO: MPU6050 nao encontrado (who=0x%02X)\r\n", who);
        Error_Handler();
    }
    print("[LQR] MPU6050 encontrado (who=0x%02X)\r\n", who);

    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU_ADDR, MPU_PWR_MGMT_1, 1, &data, 1, 100);
    HAL_Delay(20);

    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU_ADDR, MPU_GYRO_CONFIG, 1, &data, 1, 100);
    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU_ADDR, MPU_ACCEL_CONFIG, 1, &data, 1, 100);
    HAL_Delay(20);
}

void mpu_calibrate(void)
{
    int64_t soma = 0;
    uint8_t buf[6];
    for (int i = 0; i < 400; i++) {
        HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR, MPU_GYRO_XOUT, 1, buf, 6, 100);
        int16_t gy_raw = (int16_t)(buf[2] << 8 | buf[3]);
        soma += ((int64_t)gy_raw << 16) / 131;
        HAL_Delay(3);
    }
    s.gyro_offset = (q16_t)(soma / 400);
}

void mpu_read(q16_t *ax, q16_t *ay, q16_t *az, q16_t *gx, q16_t *gy, q16_t *gz)
{
    uint8_t buf[14];
    HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR, MPU_ACCEL_XOUT, 1, buf, 14, 100);

    int16_t ax_raw = (int16_t)(buf[0]  << 8 | buf[1]);
    int16_t ay_raw = (int16_t)(buf[2]  << 8 | buf[3]);
    int16_t az_raw = (int16_t)(buf[4]  << 8 | buf[5]);
    int16_t gx_raw = (int16_t)(buf[8]  << 8 | buf[9]);
    int16_t gy_raw = (int16_t)(buf[10] << 8 | buf[11]);
    int16_t gz_raw = (int16_t)(buf[12] << 8 | buf[13]);

    *ax = (q16_t)(((int64_t)ax_raw << 16) / ACCEL_SENS);
    *ay = (q16_t)(((int64_t)ay_raw << 16) / ACCEL_SENS);
    *az = (q16_t)(((int64_t)az_raw << 16) / ACCEL_SENS);
    *gx = (q16_t)(((int64_t)gx_raw << 16) / GYRO_SENS);
    *gy = (q16_t)(((int64_t)gy_raw << 16) / GYRO_SENS);
    *gz = (q16_t)(((int64_t)gz_raw << 16) / GYRO_SENS);
}


void encoder_update(void)
{
    // Obtém o RPM médio deslizante (RPM × 1000)
    int32_t rpm_scaled = encoder_get_sliding_rpm(0);

    // Converte para velocidade linear em mm/s (inteiro)
    int32_t speed_mm_s = encoder_rpm_to_mm_s(rpm_scaled);

    // Converte para Q25 (m/s): 1 mm/s = 0,001 m/s
    // speed_q25 = (speed_mm_s << 25) / 1000
    int32_t speed_q25 = (int32_t)(((int64_t)speed_mm_s << 25) / 1000);

    // Atualiza a velocidade (usa a média deslizante para suavizar)
    s.vel = speed_q25;

    // Integra posição (metros): pos += vel * dt   (dt = 5 ms = 0,005 s)
    // dt_q25 = Q25(0.005) = 167773
    const int32_t DT_Q25 = 167773;   // 0.005 * 2^25
    s.pos += (int32_t)(((int64_t)s.vel * DT_Q25) >> 25);
}

void filtro_complementar(q16_t ax, q16_t ay, q16_t az, q16_t gy_dps)
{
    // ── Ângulo pelo acelerómetro (apenas pitch) ──
    // theta_acc = atan2(ax, az)  [rad]
    // Usamos aproximação com Q25 e uma tabela de atan2??
    // Para já, usamos floats apenas para calcular theta_acc e convertemos de volta.
    // (O FPU do STM32F4 é rápido; uma divisão flutuante a 200 Hz não é problema.)
    float ax_f = Q16_TO_F(ax);
    float az_f = Q16_TO_F(az);
    float theta_acc_f = atan2f(ax_f, az_f);    // resultado em radianos

    // converte para Q25
    int32_t theta_acc = (int32_t)(theta_acc_f * (float)(1 << 25));

    // ── Velocidade angular (gyro) ──
    // gy_dps está em Q16 (graus/s). Converter para rad/s (Q25):
    // gy_rads_q25 = gy_dps * (pi/180) * 2^9   (porque Q16 → Q25 precisa de <<9)
    const int32_t GYRO_TO_RAD = (int32_t)((3.1415926535f / 180.0f) * (float)(1 << 25) / 65536.0f);
    // Cálculo equivalente: (gy_dps << 9) * 0.0174533
    // int32_t gy_rads = (int32_t)(((int64_t)gy_dps * 392699) >> 16); // 0.0174533 * 2^25 ≈ 585042?
    // Método mais simples:
    int32_t gy_rads = (int32_t)(((int64_t)gy_dps << 25) / (180 * 128));
    // Vamos deixar claro:
    // 1 grau = pi/180 rad. gy_dps (Q16) representa graus/s * 65536.
    // gy_rads = gy_dps * (pi/180) * 2^(25-16) = gy_dps * 0.0174533 * 512 = gy_dps * 8.936
    // Multiplica por 9 e depois divide por 1?
    const int64_t GYRO_SCALE = (int64_t)(0.0174533f * 65536.0f * (1 << 9)); // pré-calculado
    // Para evitar float, definimos diretamente: 0.0174533 * 2^25 = 585042
    #define GYRO_Q16_TO_RAD_Q25(g) ((int32_t)(((int64_t)(g) * 585042LL) >> 16))

    int32_t gyro_rads = GYRO_Q16_TO_RAD_Q25(gy_dps);

    // ── Filtro complementar ──
    // alpha = 0.98 (confia mais no giroscópio a curto prazo)
    // theta = alpha*(theta + gyro*dt) + (1-alpha)*theta_acc
    const int32_t ALPHA_Q25 = 0x7D70A3D7;   // 0.98 * 2^25
    const int32_t ONE_MINUS_ALPHA_Q25 = 0x028F5C29; // 0.02 * 2^25
    const int32_t DT_Q25 = 167773;           // 5 ms em Q25

    // prevê com giroscópio
    int32_t theta_pred = s.theta + (int32_t)(((int64_t)gyro_rads * DT_Q25) >> 25);

    // mistura com acelerómetro
    s.theta = (int32_t)(((int64_t)ALPHA_Q25 * theta_pred + (int64_t)ONE_MINUS_ALPHA_Q25 * theta_acc) >> 25);

    // Velocidade angular (já filtrada) – guardamos a do giroscópio
    s.theta_dot = gyro_rads;
}

void controle_lqr(void)
{
    // if (s.theta > THETA_MAX || s.theta < -THETA_MAX) {
    //     motor1_parar();
    //     motor2_parar();
    //     return;
    // }

    q31_t theta_err = s.theta - THETA_OFFSET;

    // A lei de controlo original (não mexer)
    q31_t u_raw = Q16_MUL_KS(K1, s.pos)
                + Q16_MUL_KS(K2, s.vel)
                + Q16_MUL_KS(K3, theta_err)
                + Q16_MUL_KS(K4, s.theta_dot);
    s.u = -u_raw;

    if (s.u >  FORCA_MAX) s.u =  FORCA_MAX;
    if (s.u < -FORCA_MAX) s.u = -FORCA_MAX;

    // ── NOVA PARTE: converter força (Q16) → RPM desejado ──
    // Assumir que força máxima (2.5) corresponde a 130 RPM (sem carga)
    float u_float = Q16_TO_F(s.u);
    float target_rpm_float = (u_float / 2.5f) * 130.0f;
    int32_t target_rpm = (int32_t)(target_rpm_float * 1000.0f);  // RPM × 1000

    // Obter RPM atual (média deslizante)
    int32_t current_rpm = encoder_get_sliding_rpm(0);

    // Chamar o PI para obter o PWM
    int32_t pwm = speed_controller_update(&motor1, target_rpm,current_rpm,current_rpm);
	s.pwm = (int16_t)pwm;

    // Aplicar ao motor (usa as funções já existentes no projeto)
    motor1_set(pwm);
    // motor2_set(pwm);   // se houver dois motores
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


// void debug_print(void)
// {
    // char buf[128];
    // snprintf(buf, sizeof(buf),
    //     "loop %lu pos=%.3f vel=%.3f th=%.2f deg thdot=%.2f deg/s u=%.3f pwm=%d\r\n",
    //     (unsigned long)s.loop_count,
    //     Q25_TO_F(s.pos),
    //     Q25_TO_F(s.vel),
    //     Q25_TO_F(s.theta) * 57.2958f,
    //     Q25_TO_F(s.theta_dot) * 57.2958f,
    //     Q16_TO_F(s.u),
    //     s.pwm
    // );
    // CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
// }
void debug_print(void)
{
    char buf[200];

    // ── Converter posição (Q25) para mm (3 decimais) ──
    int32_t pos_int = s.pos >> 25;                     // parte inteira
    uint32_t pos_frac_raw = (s.pos >= 0) ? (s.pos & 0x1FFFFFF) : (-s.pos & 0x1FFFFFF);
    uint32_t pos_mm = (uint32_t)(((uint64_t)pos_frac_raw * 1000) >> 25); // 3 casas decimais

    // ── Converter velocidade (Q25) para mm/s ──
    int32_t vel_int = s.vel >> 25;
    uint32_t vel_frac_raw = (s.vel >= 0) ? (s.vel & 0x1FFFFFF) : (-s.vel & 0x1FFFFFF);
    uint32_t vel_mm = (uint32_t)(((uint64_t)vel_frac_raw * 1000) >> 25);

    // ── Converter ângulo (Q25) para graus (2 decimais) ──
    // factor = (180 / PI) * 2^25 ≈ 192038709
    const int64_t DEG_FACTOR = 192038709LL;
    int32_t theta_deg_q25 = (int32_t)(((int64_t)s.theta * DEG_FACTOR) >> 25);
    int32_t theta_int = theta_deg_q25 >> 25;
    uint32_t theta_frac_raw = (theta_deg_q25 >= 0) ? (theta_deg_q25 & 0x1FFFFFF) : (-theta_deg_q25 & 0x1FFFFFF);
    uint32_t theta_cdeg = (uint32_t)(((uint64_t)theta_frac_raw * 100) >> 25); // centésimos de grau

    // ── Converter vel. angular (Q25) para graus/s ──
    int32_t theta_dot_deg_q25 = (int32_t)(((int64_t)s.theta_dot * DEG_FACTOR) >> 25);
    int32_t theta_dot_int = theta_dot_deg_q25 >> 25;
    uint32_t theta_dot_frac_raw = (theta_dot_deg_q25 >= 0) ? (theta_dot_deg_q25 & 0x1FFFFFF) : (-theta_dot_deg_q25 & 0x1FFFFFF);
    uint32_t theta_dot_cdeg = (uint32_t)(((uint64_t)theta_dot_frac_raw * 100) >> 25);

    // ── Converter força (Q16) para 3 decimais ──
    int32_t u_int = s.u >> 16;
    uint32_t u_frac_raw = (s.u >= 0) ? (s.u & 0xFFFF) : (-s.u & 0xFFFF);
    uint32_t u_mm = (uint32_t)(((uint64_t)u_frac_raw * 1000) >> 16);

    // ── PWM ──
    int16_t pwm = s.pwm;

    snprintf(buf, sizeof(buf),
        "loop %lu pos=%d.%03u vel=%d.%03u th=%d.%02u deg thdot=%d.%02u deg/s u=%d.%03u pwm=%d\r\n",
        (unsigned long)s.loop_count,
        pos_int, pos_mm,
        vel_int, vel_mm,
        theta_int, theta_cdeg,
        theta_dot_int, theta_dot_cdeg,
        u_int, u_mm,
        pwm);

    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}
