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
#include "speed_controller.h"
#include "mpu_data.h"
#include "debug_print.h"
#include "MPU6050/MPU6050.h"
/**
Arrumar o Q31 e para fazer todas as contas no controle com ponto fixo e normalizado X
pegar os dados do enconder, converter em velocidade X, arrumado de novo 			X
usar apenas um timer para os 2 pwm		- falhei duas vezes
inicializar o ADC, ler a bateria00		falta conecção , codigo feito mas não será mais usado			x
inicializar o timestamp 				contador de loops mais timestamp HAL_GetTick X

buffer circular 10 amostras de cada grandeza									X
filtrar o RPM para ter o valor medio,mas  com precisão em vez de a cada 100ms  X
relacionar a tensão com a velocidade e assim poder melhorar o controle				- conflito entre o adc e o encoder 2, e com o controle não é ncesario
decidir qual é o melhor jeito de ter as variaveis do DMP, Quaternion ou angulos ou outro	X
inicializar o dmp e a cada 5ms pegar os dados e colocar num buffer circular -----
O dmp deu errado, por algum motivo os valores deram muito distantes do que era para ser, com seus angulos saturando com muito pouco movimento e muita inercia nos dados
usar o mpu e os dados que a biblioteca fornce em vez do dmp
organizar e comentar o codigo			-( parei de poluir o main ao menos)
O sinal do pwm parece errado de novo (do print)...		X
o controle não funcina para rpm negativo				X
A um problema no RPM que não será resolvido, a forma escolhida de calcular foi somando o tempo entre os pulsos, dessa forma não levando em conta o sinal, o sinal foi feito depois checando qual foi o sentido da "maioria" dos pulsos, e assim atribuindo o sinal, como não é esperado que o pendulo fique constantemente invertendo a polaridade não deve ser um problema grave

começar a parte de controle					X
criar o bloco do controle do motor, que ira fornecer o PWM necesario para o RPM ser atingido (setpoint de velocidade)	X e testado
Se parou de usar o dmp apos perceber que os dados estavam muito inconcisitentes (mudanças leves  provocavam alterações drasticas nos valores), e que a resposta tinha algum tipo de inercia estranha
o mpu foi usado com a biblioteca como antes, agora o testando mais profundamente e checando a viabilidade de usar cada angulo no controle, inicalmente os dados era bem instaveis e se percebeu que a derivação do yaw era muito maior que a esperada
então o foco foi a calibração do sensor, que por equanto deve ser feita manualmente e colocada no codigo, com ela se permitiu ter angulos mais estaveis e um yaw que derivasse o apenas alguns graus por minuto, permitindo testar se é viavel futuramente juntar rpm e yaw para dar direção ao pendulo
com o pendulo na horizontal, o angulo fica perto de 70°, uma diferença grande, mas que se espera não impactar no controle


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
LQR_State lqr;
speed_controller_t motor1;
speed_controller_t motor2;

extern MPU6050 mpu;
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

    return 11100;
}

uint32_t bat;
uint32_t timestamp = 0;
uint32_t cycle_counter = 0;

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

volatile systemFlags_t systemFlags;

int main(void)
{
	HAL_Init();
	SystemClock_Config();

	MX_GPIO_Init();		  // 2. GPIO

	MX_I2C1_Init();
	MX_USART1_UART_Init();
	MX_USB_DEVICE_Init();
	MX_TIM2_Init();		  // PWM motor 1 E 2  (PB3) (PA2)
	MX_TIM3_Init();		  // Encoder 1	  (PA6, PA7)
	MX_TIM4_Init();		  // Timer controle 5ms
	MX_TIM5_Init();		  // Encoder 2	  (PA0, PA1)
	MX_TIM9_Init();		  // PWM motor 2

	MX_TIM10_Init();		//para medir o tempo entre os pulsos dos dois encoders
	// MX_ADC1_Init();

	HAL_Delay(20);
	systemFlags.calibrate= 0;
	mpu_init();
	print_gyro_offsets();
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    __HAL_TIM_SET_COUNTER(&htim5, 0);

    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
    HAL_TIM_Base_Start_IT(&htim4);




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
	lqr_init(&lqr);



	print("\r\n[MAIN] Loop principal iniciado.\r\n");

	print("[MAIN] Controle rodando na interrupcao TIM4 (5ms).\r\n\r\n");

	//partida
		motor1_set(1000);
		motor2_set(1000);
		HAL_Delay(10);

	uint32_t cycle_encoder=0;
	while (1)
	{
		// motor1_set(500);
		// motor2_set(500);

		// uint16_t cont = __HAL_TIM_GET_COUNTER(&htim5);
		// print("%u\n\r",cont);
        encoder_process_fast(); //conta o tempo entre pulso, execulta sempre que der
		if (systemFlags.control_loop) {
			systemFlags.control_loop=0;

			if ((cycle_counter-cycle_encoder)>=20) {
				cycle_encoder=cycle_counter;
				encoder_process_window();			 // a cada 100 ms, da a media lenta, mas confiavel dos valores
			}
			cycle_counter++;
			lqr.loop_count=cycle_counter;
			timestamp = HAL_GetTick();

			rpm_m1.push(encoder_get_sliding_rpm(0));
			rpm_m2.push(encoder_get_sliding_rpm(1));
			lqr.rpm_m1 = rpm_m1[0];
			lqr.rpm_m2 = rpm_m1[0];
			mpu_update();
			// Leitura bruta do acelerómetro para comparar com o mpu

			// bat = read_battery_mV();
			// print_status_RPM();
			// print_rpm_buffer(rpm_m1, "RPM M1");


			// print_status_mpu();

			// ──────────────────────────────────────────────
			// 3. Estimação dos estados do pêndulo
			encoder_data_update(&lqr);                        // pos e vel
			filtro_complementar(&lqr, mpu_pitch, mpu_gyro_y);         // theta e theta_dot
			// ──────────────────────────────────────────────
			// 4. Controlo LQR → calcula target_rpm
			controle_lqr(&lqr);

			// ──────────────────────────────────────────────
			// 5. Controlo PI de velocidade (motor a motor)



			// Leitura do RPM (média deslizante)
			// int32_t rpm_now = rpm_m1[0];
			// Setpoint para teste – mais tarde virá do LQR
			// int32_t rpm_target = 25000;   // 30.000 → 30 RPM

			// Calcula o PWM (futuramente adiconar um outro controle aqui para o anglo yaw, assim permitir curvas, melhorar o qão reto o pendulo anda)
			int32_t pwm1 = speed_controller_update(&motor1, lqr.target_rpm, rpm_m1[0],rpm_m1[1]);
			int32_t pwm2 = speed_controller_update(&motor2, lqr.target_rpm, rpm_m2[0],rpm_m2[1]);

			// int32_t pwm1 = 400;
			// int32_t pwm2 = 400;
			lqr.pwm = pwm1;   // guarda um dos PWMs para debug
			pwm_m1.push(pwm1);
			pwm_m2.push(pwm2);

			// Aplica nos motores
			motor1_set(pwm1);
			motor2_set(pwm2);

			// Debug
				debug_print_lqr(&lqr);
			// print_status_PWM(rpm_target, rpm_m1[0],pwm1,rpm_m2[0],pwm2);
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
