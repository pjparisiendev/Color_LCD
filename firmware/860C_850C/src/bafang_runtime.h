#ifndef BAFANG_RUNTIME_H
#define BAFANG_RUNTIME_H

#include <stdint.h>

void bafang_runtime_init(void);
void bafang_runtime_tick(uint16_t elapsed_ms);
void bafang_runtime_receive_byte(uint8_t byte);

#endif
