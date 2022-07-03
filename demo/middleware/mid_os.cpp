#include "mid_os.h"
#if defined __linux__
#include <unistd.h>
#elif defined ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#error "unsupported platform"
#endif /* platform */
#include <stdio.h>
#include <sys/time.h>

namespace os {

uint64_t tick_us() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return (uint64_t)(tv.tv_sec * 1000000 + tv.tv_usec);
}

void delay_ms(uint32_t ms) {
#if defined __linux__
  usleep(ms * 1000);
#elif defined ESP_PLATFORM
  vTaskDelay(ms / portTICK_PERIOD_MS);
#else
#error "unsupported platform"
#endif /* platform */
}

} /* namespace os */
