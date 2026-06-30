//
// Created by hu on 2020/7/11.
//
#include "ArduinoWrapper.h"

float mapArduino(float val, float I_Min, float I_Max, float O_Min, float O_Max)
{
    return (((val - I_Min) * ((O_Max - O_Min) / (I_Max - I_Min))) + O_Min);
}
int __io_putchar(int ch) {
    static uint8_t data;       // buffer estático – válido enquanto a transmissão assíncrona ocorre
    data = (uint8_t)ch;

    // Aguarda até que o driver USB esteja pronto para aceitar o byte
    while (CDC_Transmit_FS(&data, 1) == USBD_BUSY) {
        HAL_Delay(1);
    }
    return ch;
}


int _write(int file, char *ptr, int len)
{
    for(int i = 0; i < len; i++) {
        __io_putchar(ptr[i]);
    }
    return len;
}

extern "C" int fputc(int ch, FILE *f) {
    (void)f;
    return __io_putchar(ch);
}
void print(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        CDC_Transmit_FS((uint8_t*)buf, len);
    }
}
