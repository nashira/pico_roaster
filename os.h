#ifndef _OS_H_
#define _OS_H_

#include <pico.h>
#include <pico/time.h>
#include <pico/util/queue.h>

#define ftous(f) (1'000'000/f)

typedef struct {
    void(*execute)(void *user_data);
    void *user_data;
    int64_t period_us;
    alarm_id_t alarm_id;
} task_t;

void core0_init();
void core0_schedule(uint64_t delay_us, task_t *task);
bool core0_cancel(alarm_id_t alarm_id);
void core0_execute();

void core1_init();
void core1_schedule(uint64_t delay_us, task_t *task);
bool core1_cancel(alarm_id_t alarm_id);
void core1_execute();

#endif
