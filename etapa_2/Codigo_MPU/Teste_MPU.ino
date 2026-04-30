/*
  MPU6050 DMP6 - Posição relativa ao ponto inicial

  Ao ligar, o sensor deve estar na posição "em pé reto".
  Após a calibração automática, os ângulos de yaw/pitch/roll
  são zerados em relação a essa posição inicial.

  Conexões necessárias:
    - SDA, SCL, 3.3V, GND
    - Pino INT do MPU6050 → pino 2 do Arduino
*/

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

/* Endereço I2C do MPU6050 */
MPU6050 mpu(0x69);
//MPU6050 mpu(0x69); // Use se AD0 estiver em HIGH

#define OUTPUT_READABLE_YAWPITCHROLL

int const INTERRUPT_PIN = 2;
bool blinkState;

/*---Variáveis de controle do DMP---*/
bool DMPReady = false;
uint8_t MPUIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint8_t FIFOBuffer[64];

/*---Variáveis de orientação---*/
Quaternion q;
VectorInt16 aa;
VectorInt16 gy;
VectorInt16 aaReal;
VectorInt16 aaWorld;
VectorFloat gravity;
float euler[3];
float ypr[3];

/*---Offset da posição inicial (referência = "em pé reto")---*/
float ypr_offset[3] = {0, 0, 0};
bool offsetCaptured = false;

/*---Interrupção do DMP---*/
volatile bool MPUInterrupt = false;
void DMPDataReady() {
  MPUInterrupt = true;
}

// ─── Captura a média de N leituras para definir o zero ───────────────────────
void captureOffset(int numSamples) {
  float sum[3] = {0, 0, 0};
  int count = 0;

  Serial.println(F("Capturando posição inicial (mantenha o sensor parado)..."));

  while (count < numSamples) {
    if (mpu.dmpGetCurrentFIFOPacket(FIFOBuffer)) {
      mpu.dmpGetQuaternion(&q, FIFOBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

      sum[0] += ypr[0];
      sum[1] += ypr[1];
      sum[2] += ypr[2];
      count++;
    }
  }

  ypr_offset[0] = sum[0] / numSamples;
  ypr_offset[1] = sum[1] / numSamples;
  ypr_offset[2] = sum[2] / numSamples;

  Serial.print(F("Offset capturado (rad) — yaw: "));
  Serial.print(ypr_offset[0]); Serial.print(F("  pitch: "));
  Serial.print(ypr_offset[1]); Serial.print(F("  roll: "));
  Serial.println(ypr_offset[2]);

  offsetCaptured = true;
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
    Wire.setClock(400000);
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
  #endif

  Serial.begin(115200);
  while (!Serial);

  Serial.println(F("Inicializando MPU6050..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);

  Serial.println(F("Testando conexão com MPU6050..."));
  if (mpu.testConnection() == false) {
    Serial.println(F("FALHA na conexão com MPU6050"));
    while (true);
  } else {
    Serial.println(F("Conexão com MPU6050 OK"));
  }

  //Serial.println(F("\nEnvie qualquer caractere para iniciar: "));
  //while (Serial.available() && Serial.read());
  //while (!Serial.available());
  //while (Serial.available() && Serial.read());

  Serial.println(F("Inicializando DMP..."));
  devStatus = mpu.dmpInitialize();

  /* Offsets do giroscópio/acelerômetro — use MPU6050_Zero para obter os seus */
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);

  if (devStatus == 0) {
    /* Calibração automática — mantenha o sensor na posição inicial durante este processo */
    Serial.println(F("Calibrando... mantenha o sensor EM PE RETO e imóvel."));
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    Serial.println(F("Offsets ativos:"));
    mpu.PrintActiveOffsets();

    Serial.println(F("Habilitando DMP..."));
    mpu.setDMPEnabled(true);

    Serial.print(F("Habilitando interrupção (pino "));
    Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
    Serial.println(F(")..."));
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), DMPDataReady, RISING);
    MPUIntStatus = mpu.getIntStatus();

    DMPReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize();
    Serial.println(F("DMP pronto!"));

    /* Aguarda o DMP estabilizar e captura a referência de zero */
    delay(1000);
    captureOffset(50); // Média de 50 leituras para definir o zero

    Serial.println(F("\nIniciando leitura. Angulos relativos à posição inicial:"));
  } else {
    Serial.print(F("Falha ao inicializar DMP (código "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

  pinMode(LED_BUILTIN, OUTPUT);
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  if (!DMPReady || !offsetCaptured) return;

  if (mpu.dmpGetCurrentFIFOPacket(FIFOBuffer)) {
    mpu.dmpGetQuaternion(&q, FIFOBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    /* Subtrai o offset para que a posição inicial seja 0, 0, 0 */
    float yaw   = (ypr[0] - ypr_offset[0]) * 180.0 / M_PI;
    float pitch = (ypr[1] - ypr_offset[1]) * 180.0 / M_PI;
    float roll  = (ypr[2] - ypr_offset[2]) * 180.0 / M_PI;

    Serial.print("ypr\t");
    Serial.print(yaw);
    Serial.print("\t");
    Serial.print(pitch);
    Serial.print("\t");
    Serial.println(roll);

    blinkState = !blinkState;
    digitalWrite(LED_BUILTIN, blinkState);
  }
}
