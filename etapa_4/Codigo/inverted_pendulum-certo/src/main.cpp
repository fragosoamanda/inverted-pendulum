/**
 * main.cpp
 * Pendulo Invertido — Controle LQR
 * STM32F411 | Motor JGB37-520 | Encoder Hall 1320cnt/rev | MPU6050 | L298N
 *
 */

#include "init.h"
#include "ArduinoWrapper.h"
#include "lqr_controller.h"
#include "encoder.h"
#include "MPU6050/MPU6050_6Axis_MotionApps_V6_12.h"
#include "speed_controller.h"

//A0   A1
/**
Arrumar o Q31 e para fazer todas as contas no controle com ponto fixo e normalizado X
pegar os dados do enconder, converter em velocidade X, arrumado de novo 			X
usar apenas um timer para os 2 pwm		- falhei duas vezes
inicializar o ADC, ler a bateria00		falta conecção , codigo feito 			x
inicializar o timestamp 				contador de loops mais timestamp HAL_GetTick X

buffer circular 10 amostras de cada grandeza									X
filtrar o RPM para ter o valor medio,mas  com precisão em vez de a cada 100ms  X
relacionar a tensão com a velocidade e assim poder melhorar o controle				- conflito entre o adc e o encoder 2, e com o controle não é ncesario
decidir qual é o melhor jeito de ter as variaveis do DMP, Quaternion ou angulos ou outro	X
inicializar o dmp e a cada 5ms pegar os dados e colocar num buffer circular
organizar e comentar o codigo			-( parei de poluir o main ao menos)
O sinal do pwm parece errado de novo (do print)...		X
o controle não funcina para rpm negativo				X
A um problema no RPM que não será resolvido, a forma escolhida de calcular foi somando o tempo entre os pulsos, dessa forma não levando em conta o sinal, o sinal foi feito depois checando qual foi o sentido da "maioria" dos pulsos, e assim atribuindo o sinal, como não é esperado que o pendulo fique constantemente invertendo a polaridade não deve ser um problema grave

começar a parte de controle					X
criar o bloco do controle do motor, que ira fornecer o PWM necesario para o RPM ser atingido (setpoint de velocidade)	X e testado
LQR, o bloco que ira  forncer o RPM necesario para manter o angulo desejado (setpoint de angulo)

 */



// Variaveis do MPU6050 DMP


#define TIME_MS 5
#define TIME_SEC (TIME_MS / 1000.0) // but in fixed point we use rational 1/200
#define KP_1 30
#define KI_1 270
#define KD_1 0

#define KP_2 30
#define KI_2 270
#define KD_2 0
speed_controller_t motor1;
speed_controller_t motor2;

RingBuffer<int32_t, 20> rpm_m1;   // 20 amostras, int32_t
RingBuffer<int32_t, 20> pwm_m1;   // 20 amostras, int32_t

RingBuffer<int32_t, 20> rpm_m2;   // 20 amostras, int32_t
RingBuffer<int32_t, 20> pwm_m2;   // 20 amostras, int32_t
// Constantes
#define ADC_REF_VOLTAGE_MV  3300    // Tensão de referência do STM32 (3.3V)
#define ADC_MAX_VALUE       4095    // Resolução 12 bits (2^12 - 1)

// Função que retorna a tensão da bateria em milivolts (mV)

uint32_t read_battery_mV(void){
//     // 1. Inicia a conversão (software trigger)
//     HAL_ADC_Start(&hadc1);

//     // 2. Espera a conversão terminar (timeout de 100ms)
//     if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK) {
//         return 0; // Erro na conversão
//     }

//     // 3. Lê o valor cru do ADC (0 a 4095)
//     uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);

//     // 4. Converte para tensão no pino (em mV)
//     //    V_pino = (adc_raw * 3300) / 4095
//     uint32_t pin_mV = (adc_raw * ADC_REF_VOLTAGE_MV) / ADC_MAX_VALUE;

//     // 5. Aplica o fator do divisor resistivo (ex: 4x)
//     uint32_t battery_mV = pin_mV;

//     return battery_mV;
}

uint32_t bat;
uint32_t timestamp = 0;
uint32_t cycle_counter = 0;
int32_t setpoint_rpm= 0;


/**
 * Imprime uma linha com os dados dos dois motores.
 *
 * @param setpoint_rpm   alvo de RPM (×1000)
 * @param rpm1           RPM atual do motor 1 (×1000)
 * @param pwm1           PWM do motor 1 (-1000 … 1000)
 * @param rpm2           RPM atual do motor 2 (×1000)
 * @param pwm2           PWM do motor 2 (-1000 … 1000)
 */
void print_status_PWM(int32_t setpoint_rpm,
                      int32_t rpm1, int32_t pwm1,
                      int32_t rpm2, int32_t pwm2)
{
    char buf[160]; // um pouco maior para caber tudo

    // ── Setpoint ──
    int32_t  tgt_int  = setpoint_rpm / 1000;
    uint32_t tgt_frac = (uint32_t)abs(setpoint_rpm % 1000);

    // ── Motor 1 ──
    int32_t  rpm1_int  = rpm1 / 1000;
    uint32_t rpm1_frac = (uint32_t)abs(rpm1 % 1000);
    int32_t  pwm1_int  = pwm1;

    // ── Motor 2 ──
    int32_t  rpm2_int  = rpm2 / 1000;
    uint32_t rpm2_frac = (uint32_t)abs(rpm2 % 1000);
    int32_t  pwm2_int  = pwm2;

    snprintf(buf, sizeof(buf),
             "[%8lu] Cyc:%7lu | Target:%+4d.%03u RPM | "
             "M1:%+4d.%03u RPM PWM:%+4d | "
             "M2:%+4d.%03u RPM PWM:%+4d\r\n",
             timestamp, cycle_counter,
             (int)tgt_int, (unsigned int)tgt_frac,
             (int)rpm1_int, (unsigned int)rpm1_frac, (int)pwm1_int,
             (int)rpm2_int, (unsigned int)rpm2_frac, (int)pwm2_int);

    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}
void print_status_RPM(void) {
    char buf[200];

    int32_t rpm1_inst = enc_m1.rpm_instant;
    int32_t rpm1_filt = enc_m1.rpm_filtered;
    int32_t rpm1_slid = encoder_get_sliding_rpm(0);

    int32_t rpm2_inst = enc_m2.rpm_instant;
    int32_t rpm2_filt = enc_m2.rpm_filtered;
    int32_t rpm2_slid = encoder_get_sliding_rpm(1);

    snprintf(buf, sizeof(buf),
             "[%8lu] Cyc:%7lu "
             "M1:%+4d.%03u S%+4d.%03u  "
             "M2:%+4d.%03u S%+4d.%03u\r\n",
             timestamp, cycle_counter,
             (int)(rpm1_inst / 1000), (unsigned int)(abs(rpm1_inst % 1000)),
             (int)(rpm1_slid / 1000), (unsigned int)(abs(rpm1_slid % 1000)),
             (int)(rpm2_inst / 1000), (unsigned int)(abs(rpm2_inst % 1000)),
             (int)(rpm2_slid / 1000), (unsigned int)(abs(rpm2_slid % 1000))
    );
    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}

template<uint8_t N>
void print_rpm_buffer(const RingBuffer<int32_t, N> &buf, const char *label) {
    char line[256];
    int pos = 0;

    pos += snprintf(line + pos, sizeof(line) - pos, "%s: [", label);

    for (uint8_t i = 0; i < buf.size(); i++) {
        int32_t val = buf[i];                                // 0 = mais recente
        int32_t int_part = val / 1000;
        uint32_t frac_part = (uint32_t)abs(val % 1000);

        pos += snprintf(line + pos, sizeof(line) - pos,
                        "%s%+d.%03u",
                        (i > 0) ? ", " : "",                 // vírgula só a partir do 2.º elemento
                        int_part, frac_part);
    }

    pos += snprintf(line + pos, sizeof(line) - pos, "]\r\n");
    CDC_Transmit_FS((uint8_t*)line, (size_t)pos);
}

MPU6050 mpu(0x69);

uint8_t	 devStatus = 0;
uint16_t packetSize;
uint8_t	 fifoBuffer[64];
Quaternion	q;
VectorFloat gravity;
q31_t ypr[3];


typedef union {
	struct {
		bool		encoder_update				: 1;
		bool		control_loop				: 1;
		bool		dmpReady					: 1;
		bool		dmp_recive					: 1;
		uint8_t		unusedFlags					: 6;
	};
	uint8_t allFlags;
} systemFlags_t;
volatile systemFlags_t systemFlags;

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
		systemFlags.dmpReady   = true;
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

	MX_GPIO_Init();		  // 2. GPIO (restaura PA0/PA1 como AF TIM5 e PA2 como AF TIM9)??

	MX_I2C1_Init();
	MX_USART1_UART_Init();
	MX_USB_DEVICE_Init();
	MX_TIM2_Init();		  // PWM motor 1 E 2  (PB3) (PA2)
	MX_TIM3_Init();		  // Encoder 1	  (PA6, PA7)
	MX_TIM4_Init();		  // Timer controle 5ms
	MX_TIM5_Init();		  // Encoder 2	  (PA0, PA1)  — PA0 ja esta como AF_PP aqui
	MX_TIM9_Init();		  // PWM motor 2

	MX_TIM10_Init();		//para medir o tempo entre os pulsos para o RPM dos dois encoders

	// MX_ADC1_Init();
	HAL_Delay(20);

	// Depois de HAL_TIM_Base_Init(&htim4)
	dmp_init();

	HAL_Delay(100);

	print("\r\n==========================================\r\n");
	print("  PENDULO INVERTIDO — CONTROLE LQR\r\n");
	print("  STM32F411 | JGB37-520 | MPU6050\r\n");
	print("==========================================\r\n\r\n");

	speed_controller_init(&motor1,
						KP_1,
						KI_1,
						KD_1,
						-1000, 1000, 5);  // PWM min, max, dt=5ms
	speed_controller_init(&motor2,
						KP_2,
						KI_2,
						KD_2,
						-1000, 1000, 5);
    // encoder_init();
	lqr_init();



	print("\r\n[MAIN] Loop principal iniciado.\r\n");

	print("[MAIN] Controle rodando na interrupcao TIM4 (5ms).\r\n\r\n");

	//partida
		motor1_set(1000);
		motor2_set(1000);
		HAL_Delay(10);

	uint32_t cycle_encoder=0;
	while (1)
	{

		// if (systemFlags.dmpReady && mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
		//	mpu.dmpGetQuaternion(&q, fifoBuffer);
		//	mpu.dmpGetGravity(&gravity, &q);
		//	mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

		//	char buf[128];
		//	snprintf(buf, sizeof(buf),
		//		"[MAIN] DMP ypr: %.2f / %.2f / %.2f deg"
		//		" | LQR th=%.2f deg u=%.3f N\r\n",
		//		ypr[0] * 180.0f / 3.14159f,
		//		ypr[1] * 180.0f / 3.14159f,
		//		ypr[2] * 180.0f / 3.14159f,
		//		lqr_get_theta() * 180.0f / 3.14159f,
		//		lqr_get_u()
		//	);
		//	CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
		// }

		// if (systemFlags.dmp_recive) {
		//	mpu.dmpGetQuaternion(&q, fifoBuffer);
		//	mpu.dmpGetGravity(&gravity, &q);
		//	mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
		// }

		// motor1_set(10); // cade o minimo????
		// motor2_set(50); //200

		// motor1_set(900);
		// motor2_set(1000);

		// motor1_set(500);
		// motor2_set(500);

		// uint16_t cont = __HAL_TIM_GET_COUNTER(&htim5);
		// print("%u\n\r",cont);
        encoder_process_fast(); //conta o tempo entre pulso, execulta sempre que der
		if (systemFlags.encoder_update) {
			systemFlags.encoder_update=0;

			if ((cycle_counter-cycle_encoder)>=20) {
				cycle_encoder=cycle_counter;
				encoder_process_window();			 // a cada 100 ms, da a media lenta, mas confiavel dos valores
			}
			cycle_counter++;
			timestamp = HAL_GetTick();

			rpm_m1.push(encoder_get_sliding_rpm(0));
			rpm_m2.push(encoder_get_sliding_rpm(1));
			// bat = read_battery_mV();
			// print_status_RPM();
			// print_rpm_buffer(rpm_m1, "RPM M1");
		}

		if (systemFlags.control_loop) {
			systemFlags.control_loop = 0;
			// lqr_loop();

			// Leitura do RPM (média deslizante)
			// int32_t rpm_now = rpm_m1[0];
			// Setpoint para teste – mais tarde virá do LQR
			int32_t rpm_target = 25000;   // 30.000 → 30 RPM

			// Calcula o PWM
			int32_t pwm1 = speed_controller_update(&motor1, rpm_target, rpm_m1[0],rpm_m1[1]);
			int32_t pwm2 = speed_controller_update(&motor2, rpm_target, rpm_m2[0],rpm_m2[1]);

			// int32_t pwm1 = 400;
			// int32_t pwm2 = 400;
			pwm_m1.push(pwm1);
			pwm_m2.push(pwm2);

			// Aplica nos motores
			motor1_set(pwm1);
			motor2_set(pwm2);

			// Debug
			print_status_PWM(rpm_target, rpm_m1[0],pwm1,rpm_m2[0],pwm2);
		}
		if (systemFlags.control_loop) {
			// lqr_loop();
			// debug_print();
		//	q16_t ax, ay, az, gx, gy, gz;
		//	mpu_read(&ax, &ay, &az, &gx, &gy, &gz);
		//	encoder_update();

		//	q16_t gy_dps = gy - s.gyro_offset;
		//	filtro_complementar(ax, ay, az, gy_dps);
		//	controle_lqr();

			// s.loop_count++;
		//	if (s.loop_count % 10 == 0) {
		//		debug_print();
		//	}
		}
	}
}

void Error_Handler(void)
{
	__disable_irq();
	while (1) {
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
		HAL_Delay(500);
	}
}

 /**
  * @brief This function handles TIM4 global interrupt (controle LQR 5ms).
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM4) {

		systemFlags.encoder_update=true;
		systemFlags.control_loop=true;
	}
}
