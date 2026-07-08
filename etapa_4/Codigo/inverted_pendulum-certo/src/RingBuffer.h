#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <string.h>

template<typename T, uint8_t N>
class RingBuffer {
private:
    T buffer[N];
    uint8_t head;       // próxima posição de escrita
    uint8_t count;      // número de elementos armazenados (até N)
    int64_t sum;        // soma acumulada (para média rápida)

public:
    RingBuffer() : head(0), count(0), sum(0) {
        memset(buffer, 0, sizeof(buffer));
    }

    // Adiciona um novo valor (o mais recente)
    void push(T value) {
        if (count == N) {
            // Remove o valor mais antigo da soma
            sum -= buffer[head];
        } else {
            count++;
        }
        buffer[head] = value;
        sum += value;
        head = (head + 1) % N;
    }

    // Acesso indexado: 0 = mais recente, 1 = penúltimo, ..., count-1 = mais antigo
    T operator[](uint8_t index) const {
        if (index >= count) return 0;
        uint8_t newest = (head == 0) ? (N - 1) : (head - 1);
        int8_t pos = newest - index;
        if (pos < 0) pos += N;
        return buffer[pos];
    }

    // Número de elementos armazenados
    uint8_t size() const { return count; }

    // Média aritmética dos valores armazenados
    T average() const {
        if (count == 0) return 0;
        return static_cast<T>(sum / count);
    }

    // Zera o buffer
    void clear() {
        memset(buffer, 0, sizeof(buffer));
        head = 0;
        count = 0;
        sum = 0;
    }
};
#include <stdio.h>      // snprintf
#include <stdlib.h>     // abs

// Declaração externa da transmissão USB (ex.: CDC_Transmit_FS)
// extern void CDC_Transmit_FS(uint8_t *data, size_t len);

#endif
