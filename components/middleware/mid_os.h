#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <functional>

namespace os {

using tick_t = uint64_t;

tick_t tick_us();
static inline tick_t tick_ms() {return tick_us() / 1E+3;}
static inline tick_t tick_s() {return tick_us() / 1E+6;}

void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

/* @ref https://www.jianshu.com/p/aa96876ebabc */
class timer_t {
  public:
  void *arg;
  using callback_t = std::function<void(timer_t *)>;
  timer_t(callback_t callback, void *arg = nullptr, const char name[] = nullptr);
  ~timer_t();
  void start(uint32_t period, bool reload = false);
  void stop();
  bool is_started();
  private:
  void *_handle;
  callback_t _callback;
  static void _callback_wrapper(void *handle);
};

/* @ref https://www.jianshu.com/p/88fdd500cf44 */
class task_t {
  public:
  void *arg;
  using callback_t = std::function<void(task_t *)>;
  task_t(callback_t callback, void* arg = nullptr, uint32_t stack_size = 1024, uint32_t priority = 0, const char name[] = nullptr);
  ~task_t();
  private:
  void *_handle;
  callback_t _callback;
  static void _callback_wrapper(void *handle);
};

} /* namespace os */
