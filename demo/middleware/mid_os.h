#pragma once

#include <stdint.h>

namespace os {

uint64_t tick_us();
static inline uint64_t tick_ms() {return tick_us() / 1E+3;}
static inline uint64_t tick_s() {return tick_us() / 1E+6;}

void delay_ms(uint32_t ms);

} /* namespace os */
