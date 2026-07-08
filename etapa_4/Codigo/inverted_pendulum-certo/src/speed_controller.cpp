#include "speed_controller.h"

void speed_controller_init(speed_controller_t *ctrl, int32_t Kp, int32_t Ki, int32_t Kd,
                           int32_t pwm_min, int32_t pwm_max, uint32_t dt_ms)
{
    ctrl->Kp = Kp;
    ctrl->Ki = Ki;
    ctrl->Kd = Kd;
    ctrl->integral = 0;
    ctrl->last_error = 0;
    ctrl->pwm_min = pwm_min;
    ctrl->pwm_max = pwm_max;
    ctrl->dt_ms = dt_ms;
}

int32_t speed_controller_update(speed_controller_t *ctrl,
                                int32_t setpoint_rpm,
                                int32_t current_rpm,
                                int32_t previous_rpm)
{
    int32_t error = setpoint_rpm - current_rpm;

    // --- Proporcional ---
    int32_t P = (ctrl->Kp * error) / 1000;

    // --- Integral (atualiza sempre, mas limita a saturação) ---
    ctrl->integral += ((int64_t)ctrl->Ki * error * ctrl->dt_ms) / 1000;
    int32_t I = (int32_t)(ctrl->integral / 1000);

    // --- Derivativo (sobre a medição) ---
    int32_t D = 0;
    if (ctrl->dt_ms > 0 && previous_rpm != 0) {
        int32_t diff = current_rpm - previous_rpm;
        D = (ctrl->Kd * diff) / ((int32_t)ctrl->dt_ms * 1000);
    }

    int32_t output = P + I - D;

    // --- Back‑calculation anti‑windup (simétrico) ---
    if (output > ctrl->pwm_max) {
        // A saída ideal seria maior, mas limitamos ao máximo.
        // Recalcula o integral de forma a que P + I_new - D = pwm_max
        I = ctrl->pwm_max - P + D;
        ctrl->integral = (int64_t)I * 1000;
        output = ctrl->pwm_max;
    } else if (output < ctrl->pwm_min) {
        I = ctrl->pwm_min - P + D;
        ctrl->integral = (int64_t)I * 1000;
        output = ctrl->pwm_min;
    }

    return output;
}

void speed_controller_reset(speed_controller_t *ctrl)
{
    ctrl->integral = 0;
    ctrl->last_error = 0;
}
