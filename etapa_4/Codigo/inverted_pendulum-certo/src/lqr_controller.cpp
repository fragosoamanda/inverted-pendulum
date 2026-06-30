

#include "lqr_controller.h"
#include "ArduinoWrapper.h"
#include <math.h>
#include <string.h>


extern TIM_HandleTypeDef htim2;   // PWM motor 1
extern TIM_HandleTypeDef htim3;   // Encoder motor 1
extern TIM_HandleTypeDef htim5;   // Encoder motor 2
extern TIM_HandleTypeDef htim9;   // PWM motor 2
extern I2C_HandleTypeDef hi2c1;   // MPU6050
extern TIM_HandleTypeDef htim4;


static const q16_t K1 = Q16(-3.0061); // posicao do carro
static const q16_t K2 = Q16(-4.9994); // velocidade do carro
static const q16_t K3 = Q16( 50.9704);    // angulo
static const q16_t K4 = Q16( 8.0437);     // vel. angular


#define ENCODER_CPR         1320
#define WHEEL_DIAM_M        Q6(0.0654)
#define WHEEL_CIRCUM_M      Q6(0.20548)        
#define M_PER_COUNT         Q6(0.00015567)     


#define TS                  Q6(0.005)
#define THETA_MAX           Q6(0.5236)         // 30 graus em rad
#define FORCA_MAX           Q16(2.5)          
#define PWM_PERIOD          999


#define THETA_OFFSET_DEG    0.0f                
#define THETA_OFFSET_RAD    (THETA_OFFSET_DEG * 3.14159f / 180.0f)
#define THETA_OFFSET        Q6(THETA_OFFSET_RAD)

──
#define PWM_DEADBAND_RAW    2           
#define PWM_MIN             140         
#define PWM_BALANCE_MIN     130       


#define ALPHA            Q6(0.88)       
#define ONE_MINUS_ALPHA  Q6(0.12)
#define RAD_PER_DEG      Q6(0.017453)   


#define MPU_ADDR            (0x69 << 1)
#define MPU_PWR_MGMT_1      0x6B
#define MPU_GYRO_CONFIG     0x1B
#define MPU_ACCEL_CONFIG    0x1C
#define MPU_ACCEL_XOUT      0x3B
#define MPU_GYRO_XOUT       0x43
#define GYRO_SENS           131.0f
#define ACCEL_SENS          16384.0f


typedef struct {
    q6_t  pos;              
    q6_t  vel;              
    q6_t  theta;            
    q6_t  theta_dot;        
    q6_t  theta_dot_raw;    

    uint16_t enc1_prev;
    int32_t  enc2_prev;

    q16_t gyro_offset;      

    q16_t  u;               
    int16_t pwm;
    uint8_t ativo;

    uint32_t loop_count;
} LQR_State;

static LQR_State s;

static void     mpu_init(void);
static void     mpu_calibrate(void);
static void     mpu_read(q16_t *ax, q16_t *ay, q16_t *az, q16_t *gx, q16_t *gy, q16_t *gz);
static void     encoder_update(void);
static void     filtro_complementar(q16_t ax, q16_t ay, q16_t az, q16_t gy_dps);
static void     controle_lqr(void);
static void     motor_set(int16_t pwm);
static void     motor_parar(void);
static void     motor2_set(int16_t pwm);
static void     motor2_parar(void);
static void     debug_print(void);


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


    __HAL_TIM_SET_COUNTER(&htim3, 32768);
    s.enc1_prev = 32768;
    __HAL_TIM_SET_COUNTER(&htim5, 0x7FFFFFFF);
    s.enc2_prev = 0x7FFFFFFF;

    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
    HAL_TIM_Base_Start_IT(&htim4);

    motor_parar();
    motor2_parar();
    s.ativo = 1;

#ifndef DEBUG_MODE
    print("[LQR] MODO CONTROLE — motor ATIVO\r\n");
#else
    print("[LQR] MODO DEBUG — motor NAO acionado\r\n");
#endif

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
        debug_print();
    }
}


q6_t lqr_get_theta(void)     { return s.theta; }
q6_t lqr_get_theta_dot(void) { return s.theta_dot; }
q6_t lqr_get_pos(void)       { return s.pos; }
q6_t lqr_get_vel(void)       { return s.vel; }
q16_t lqr_get_u(void)        { return s.u; }


static void mpu_init(void)
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

static void mpu_calibrate(void)
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

static void mpu_read(q16_t *ax, q16_t *ay, q16_t *az, q16_t *gx, q16_t *gy, q16_t *gz)
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


static void encoder_update(void)
{
    int32_t raw1 = __HAL_TIM_GET_COUNTER(&htim3);
    int32_t delta1 = (int32_t)raw1 - (int32_t)s.enc1_prev;
    if (delta1 >  32768) delta1 -= 65536;
    if (delta1 < -32768) delta1 += 65536;

    s.pos += (q6_t)((int64_t)delta1 * M_PER_COUNT);
    s.vel = (q6_t)(((int64_t)delta1 * M_PER_COUNT << 25) / TS);
    s.enc1_prev = raw1;

    int32_t raw2 = (int32_t)__HAL_TIM_GET_COUNTER(&htim5);
    int32_t delta2 = raw2 - s.enc2_prev;
    s.enc2_prev = raw2;
    (void)delta2;
}


static void filtro_complementar(q16_t ax, q16_t ay, q16_t az, q16_t gy_dps)
{
    q6_t theta_acc = Q6(atan2f(Q16_TO_F(ax), Q16_TO_F(az)));
    q6_t gy_dps_q6 = (q6_t)(gy_dps << 9);      
    q6_t theta_dot_new = Q6_MUL(gy_dps_q6, RAD_PER_DEG);

    s.theta_dot = Q6_MUL(Q6(0.20f), s.theta_dot) + Q6_MUL(Q6(0.80f), theta_dot_new);

    s.theta = Q6_MUL(ALPHA, s.theta + Q6_MUL(s.theta_dot, TS))
              + Q6_MUL(ONE_MINUS_ALPHA, theta_acc);
}


static void controle_lqr(void)
{
    if (s.theta > THETA_MAX || s.theta < -THETA_MAX) {
        motor_parar();
        motor2_parar();
        return;
    }

    q6_t theta_err = s.theta - THETA_OFFSET;

  
    q16_t u_raw = Q16_MUL_KS(K1, s.pos)
                + Q16_MUL_KS(K2, s.vel)
                + Q16_MUL_KS(K3, theta_err)
                + Q16_MUL_KS(K4, s.theta_dot);
    s.u = -u_raw;

    if (s.u >  FORCA_MAX) s.u =  FORCA_MAX;
    if (s.u < -FORCA_MAX) s.u = -FORCA_MAX;

    s.pwm = (int16_t)(((int64_t)s.u * PWM_PERIOD) / FORCA_MAX);
    
    
    if (s.pwm != 0) {
        int16_t abs_pwm = (s.pwm >= 0) ? s.pwm : -s.pwm;
        if (abs_pwm < PWM_BALANCE_MIN) {
            s.pwm = (s.pwm > 0) ? PWM_BALANCE_MIN : -PWM_BALANCE_MIN;
        }
    }

    if (s.pwm >  PWM_PERIOD) s.pwm =  PWM_PERIOD;
    if (s.pwm < -PWM_PERIOD) s.pwm = -PWM_PERIOD;

#ifndef DEBUG_MODE
    motor_set(s.pwm);
    motor2_set(s.pwm);
#endif
}


static void motor_set(int16_t pwm)
{
    if (pwm > -PWM_DEADBAND_RAW && pwm < PWM_DEADBAND_RAW) {
        motor_parar();
        return;
    }

    uint32_t pwm_out;
    if (pwm > 0) {
        pwm_out = (pwm < PWM_MIN) ? PWM_MIN : (uint32_t)pwm;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET); 
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pwm_out);
    } else {
        int16_t abs_pwm = -pwm;
        pwm_out = (abs_pwm < PWM_MIN) ? PWM_MIN : (uint32_t)abs_pwm;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pwm_out);
    }
}

static void motor_parar(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
}


static void motor2_set(int16_t pwm)
{
    if (pwm > -PWM_DEADBAND_RAW && pwm < PWM_DEADBAND_RAW) {
        motor2_parar();
        return;
    }

    uint32_t pwm_out;
    if (pwm > 0) {
        pwm_out = (pwm < PWM_MIN) ? PWM_MIN : (uint32_t)pwm;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, pwm_out);
    } else {
        int16_t abs_pwm = -pwm;
        pwm_out = (abs_pwm < PWM_MIN) ? PWM_MIN : (uint32_t)abs_pwm;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, pwm_out);
    }
}

static void motor2_parar(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 0);
}


static void debug_print(void)
{
    char buf[128];
    snprintf(buf, sizeof(buf),
        "loop %lu pos=%.3f vel=%.3f th=%.2f deg thdot=%.2f deg/s u=%.3f pwm=%d\r\n",
        (unsigned long)s.loop_count,
        Q6_TO_F(s.pos),
        Q6_TO_F(s.vel),
        Q6_TO_F(s.theta) * 57.2958f,
        Q6_TO_F(s.theta_dot) * 57.2958f,
        Q16_TO_F(s.u),
        s.pwm
    );
    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}