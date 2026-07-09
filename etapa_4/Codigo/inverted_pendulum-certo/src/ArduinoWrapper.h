/*
 * ArduinoWrapper.h
 *
 *  Created on: Mar 27, 2013
 *      Author: zoellner
 */

#ifndef ARDUINOWRAPPER_H_
#define ARDUINOWRAPPER_H_



//Standard Libraries
#include <stm32f4xx_hal.h>
#include "USB_DEVICE/usb_device.h"
#include "usbd_cdc_if.h"
#include "string.h"
#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include <stdarg.h>

//TODO functions that need wrapper: millis(), Serial.print

#define PI 3.1416f
#define millis() HAL_GetTick()
#define I2CDEV_DEFAULT_WRITE_TIMEOUT     100
#define delay(x) HAL_Delay(x)


// extern UART_HandleTypeDef huart1;
float mapArduino(float val, float I_Min, float I_Max, float O_Min, float O_Max);

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
// No main.c ou em um header comum
void print(const char *fmt, ...);

#ifdef __cplusplus
}
#endif




#endif /* ARDUINOWRAPPER_H_ */
