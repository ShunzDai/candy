#include "mid_os.h"
#if defined __linux__
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#elif defined ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include <sys/time.h>
#elif defined __arm__
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#else /* platform */
#error "unsupported platform"
#endif /* platform */
#include <string.h>
#include <stdio.h>


namespace os {

tick_t tick_us() {
#if defined __linux__ || defined ESP_PLATFORM
  timeval tv;
  gettimeofday(&tv, nullptr);
  return (tick_t)(tv.tv_sec * 1000000 + tv.tv_usec);
#elif defined __arm__
  Tick_t m = uwTick;
  const Tick_t tms = SysTick->LOAD + 1;
  __IO Tick_t u = tms - SysTick->VAL;
  return (m * 1000 + (u * 1000) / tms);
#endif /* platform */
}

void delay_us(uint32_t us) {
#if defined __linux__
  usleep(us);
#elif defined ESP_PLATFORM
#else /* platform */
#error "unsupported platform"
#endif /* platform */
}

void delay_ms(uint32_t ms) {
#if defined __linux__
  usleep(ms * 1000);
#elif defined ESP_PLATFORM
  vTaskDelay(ms / portTICK_PERIOD_MS);
#elif defined __arm__
  vTaskDelay(ms / portTICK_PERIOD_MS);
#else /* platform */
#error "unsupported platform"
#endif /* platform */
}

timer_t::timer_t(callback_t callback, void *arg, const char name[])
#if defined __linux__
 : arg(arg), _callback(callback) {
  typedef void (*_function) (__sigval_t);
  sigevent sev;
  memset(&sev, 0, sizeof(sev));
  sev.sigev_value.sival_ptr = this;
  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_notify_function = (_function)(void *)timer_t::_callback_wrapper;
  sev.sigev_notify_attributes = nullptr;
  timer_create(CLOCK_MONOTONIC, &sev, (::timer_t *)&_handle);
}
#elif defined ESP_PLATFORM
 :
arg(arg),
_handle((void *)xTimerCreate(name, 1000, pdFALSE, this, (TimerCallbackFunction_t)_callback_wrapper)),
_callback(callback) {}
#else /* platform */
#error "unsupported platform"
#endif /* platform */

timer_t::~timer_t() {
#if defined __linux__
  timer_delete((::timer_t)_handle);
#elif defined ESP_PLATFORM
  xTimerDelete((TimerHandle_t)_handle, portMAX_DELAY);
#else /* platform */
#error "unsupported platform"
#endif /* platform */
}

void timer_t::_callback_wrapper(void *handle) {
#if defined __linux__
  auto timer = (timer_t *)(handle);
  timer->_callback(timer);
#elif defined ESP_PLATFORM
  auto timer = (timer_t *)(pvTimerGetTimerID((TimerHandle_t)handle));
  timer->_callback(timer);
#else /* platform */
#error "unsupported platform"
#endif /* platform */
}

void timer_t::start(uint32_t period, bool reload) {
#if defined __linux__
  itimerspec ts;
  memset(&ts, 0, sizeof(ts));
  ts.it_value.tv_sec = period / 1000;
  ts.it_value.tv_nsec = (period % 1000) * 1000 * 1000;
  ts.it_interval.tv_sec = reload ? ts.it_value.tv_sec : 0;
  ts.it_interval.tv_nsec = reload ? ts.it_value.tv_nsec : 0;
  timer_settime((::timer_t)_handle, 0, &ts, nullptr);
#elif defined ESP_PLATFORM
  vTimerSetReloadMode((TimerHandle_t)_handle, reload);
  xTimerChangePeriod((TimerHandle_t)_handle, pdMS_TO_TICKS(period), portMAX_DELAY);
#else /* platform */
#error "unsupported platform"
#endif /* platform */
}

void timer_t::stop() {
#if defined __linux__
  itimerspec ts;
  memset(&ts, 0, sizeof(ts));
  timer_settime((::timer_t)_handle, 0, &ts, nullptr);
#elif defined ESP_PLATFORM
  xTimerStop((TimerHandle_t)_handle, portMAX_DELAY);
#else /* platform */
#error "unsupported platform"
#endif /* platform */
}

bool timer_t::is_started() {
#if defined __linux__
  itimerspec ts;
  memset(&ts, 0, sizeof(ts));
  timer_gettime((::timer_t)_handle, &ts);
  return ts.it_value.tv_sec != 0 || ts.it_value.tv_nsec != 0;
#elif defined ESP_PLATFORM
  return xTimerIsTimerActive((TimerHandle_t)_handle);
#else /* platform */
#error "unsupported platform"
#endif /* platform */
}

task_t::task_t(callback_t callback, void* arg, uint32_t stack_size, uint32_t priority, const char name[])
#if defined __linux__
 : arg(arg), _callback(callback) {
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create((pthread_t *)&_handle, &attr, (void *(*)(void *))(void *)_callback_wrapper, this);
}
#elif defined ESP_PLATFORM
 : arg(arg), _callback(callback) {
  xTaskCreate((TaskFunction_t)_callback_wrapper, name, stack_size, arg, priority, (TaskHandle_t *)&_handle);
}
#else /* platform */
#error "unsupported platform"
#endif /* platform */

task_t::~task_t() {
#if defined __linux__
  pthread_join((pthread_t)_handle, nullptr);
#elif defined ESP_PLATFORM
  vTaskDelete((TaskHandle_t)_handle);
#else /* platform */
#error "unsupported platform"
#endif /* platform */
}

void task_t::_callback_wrapper(void *handle) {
#if defined __linux__
  auto task = (task_t *)(handle);
  task->_callback(task);
#elif defined ESP_PLATFORM
  auto task = (task_t *)(xTaskGetCurrentTaskHandle());
  task->_callback(task);
#else /* platform */
#error "unsupported platform"
#endif /* platform */
}

} /* namespace os */
