#include "debug_print.h"
#include "encoder.h"          // enc_m1, enc_m2, encoder_get_sliding_rpm
#include "mpu_data.h"
#include "RingBuffer.h"
#include "ArduinoWrapper.h"   // CDC_Transmit_FS, print
#include <stdio.h>
#include <stdlib.h>
#include "MPU6050/MPU6050.h"
// Variáveis globais externas (definidas no main.c ou noutro local)
extern uint32_t timestamp;
extern uint32_t cycle_counter;
extern MPU6050 mpu;
// ─────────────────────────────────────────────────
// print_status_PWM – igual à que já tens
// ─────────────────────────────────────────────────
void print_status_PWM(int32_t setpoint_rpm,
                      int32_t rpm1, int32_t pwm1,
                      int32_t rpm2, int32_t pwm2)
{
    char buf[160];

    int32_t  tgt_int  = setpoint_rpm / 1000;
    uint32_t tgt_frac = (uint32_t)abs(setpoint_rpm % 1000);

    int32_t  rpm1_int  = rpm1 / 1000;
    uint32_t rpm1_frac = (uint32_t)abs(rpm1 % 1000);
    int32_t  pwm1_int  = pwm1;

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

// ─────────────────────────────────────────────────
// print_status_RPM – igual à que já tens
// ─────────────────────────────────────────────────
void print_status_RPM(void)
{
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
// ============================================================
// Impressão de status (formato inteiro, sem float)
// ============================================================
void print_status_mpu(void) {
   // pitch, roll, yaw em centésimos de grau
    int32_t pitch_cdeg = (int32_t)(((int64_t)mpu_pitch * 90 * 100) >> 31);
    int32_t roll_cdeg  = (int32_t)(((int64_t)mpu_roll  * 180 * 100) >> 31);
    int32_t yaw_cdeg   = (int32_t)(((int64_t)mpu_yaw   * 180 * 100) >> 31);

    char buf[128];
    snprintf(buf, sizeof(buf),
             "[%8lu] Cyc:%7lu | Pitch:%+4d.%02d Roll:%+4d.%02d Yaw:%+4d.%02d\r\n",
             timestamp, cycle_counter,
             (int)(pitch_cdeg / 100), (int)(abs(pitch_cdeg) % 100),
             (int)(roll_cdeg  / 100), (int)(abs(roll_cdeg)  % 100),
             (int)(yaw_cdeg   / 100), (int)(abs(yaw_cdeg)   % 100));

    CDC_Transmit_FS((uint8_t*)buf, (uint16_t)strlen(buf));
}

// ============================================================
// Impressão do buffer de pitch (em graus)
// ============================================================
void print_buffer_mpu(void) {
    char line[512];
    int pos = 0;
    pos += snprintf(line + pos, sizeof(line) - pos, "Pitch buf: [");

    for (uint8_t i = 0; i < buf_pitch.size(); i++) {
        q31_t val = buf_pitch[i];
        int32_t cdeg = (int32_t)(((int64_t)val * 90 * 100) >> 31);
        int32_t int_part = cdeg / 100;
        uint32_t frac_part = (cdeg >= 0) ? (cdeg % 100) : (-cdeg % 100);

        pos += snprintf(line + pos, sizeof(line) - pos,
                        "%s%+d.%02u",
                        (i > 0) ? ", " : "",
                        (int)int_part, (unsigned int)frac_part);
    }
    pos += snprintf(line + pos, sizeof(line) - pos, "]\r\n");
    CDC_Transmit_FS((uint8_t*)line, (uint16_t)pos);
}
void print_angle_comparison(void)
{
    int16_t ax_raw, ay_raw, az_raw;
    mpu.getAcceleration(&ax_raw, &ay_raw, &az_raw);

    float ax_g = ax_raw / 16384.0f;   // ±2g → ±16384
    float ay_g = ay_raw / 16384.0f;
    float az_g = az_raw / 16384.0f;

    // Inclinação usando apenas o eixo X (pitch original)
    float pitch_acc_deg = atan2f(ax_g, az_g) * 180.0f / (float)M_PI;
    int32_t pitch_acc_cdeg = (int32_t)(pitch_acc_deg * 100.0f);

    // Inclinação total: ângulo entre o eixo Z e a vertical
    float total_incl_rad = atan2f(sqrtf(ax_g*ax_g + ay_g*ay_g), az_g);
    int32_t total_incl_cdeg = (int32_t)(total_incl_rad * 180.0f / (float)M_PI * 100.0f);

    // Pitch do DMP (Q31 → centésimos de grau)
    int32_t pitch_mpu_cdeg = (int32_t)(((int64_t)mpu_pitch * 90 * 100) >> 31);

    char buf[160];
    snprintf(buf, sizeof(buf),
             "Acc:%+5d.%02d  Tot:%+5d.%02d  DMP:%+5d.%02d\r\n",
             pitch_acc_cdeg / 100, abs(pitch_acc_cdeg) % 100,
             total_incl_cdeg / 100, abs(total_incl_cdeg) % 100,
             pitch_mpu_cdeg / 100, abs(pitch_mpu_cdeg) % 100);
    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}
void print_gyro_offsets(void) {
    extern int16_t gyro_offsets[3];
    char buf[80];
    snprintf(buf, sizeof(buf),
             "Gyro offsets: X=0d%04X Y=0d%04X Z=0d%04X\r\n",
             (uint16_t)gyro_offsets[0],
             (uint16_t)gyro_offsets[1],
             (uint16_t)gyro_offsets[2]);
    CDC_Transmit_FS((uint8_t*)buf, (uint16_t)strlen(buf));
}
