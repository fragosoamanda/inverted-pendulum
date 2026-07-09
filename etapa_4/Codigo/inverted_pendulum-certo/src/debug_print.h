#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include <stdint.h>

// Imprime o estado actual dos dois motores (setpoint, RPM, PWM)
void print_status_PWM(int32_t setpoint_rpm,
                      int32_t rpm1, int32_t pwm1,
                      int32_t rpm2, int32_t pwm2);

// Imprime o RPM instantâneo e sliding dos dois motores
void print_status_RPM(void);

void print_status_mpu(void);
void print_buffer_mpu(void);
void print_angle_comparison(void);
void print_gyro_offsets(void);
#endif // DEBUG_PRINT_H
