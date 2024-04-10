#ifndef _OS_H_
#define _OS_H_

#include <pico.h>
#include <pico/time.h>
#include <pico/util/queue.h>

#define ftous(f) (1'000'000/f)
#define CORE0 0
#define CORE1 1

typedef struct {
    void(*execute)(void *user_data);
    void *user_data;
    int64_t period_us;
    alarm_id_t alarm_id;
} task_t;

void core_init(uint8_t core);
void task_schedule(uint8_t core, uint64_t delay_us, task_t *task);
bool task_cancel(uint8_t core, alarm_id_t alarm_id);
void task_execute(uint8_t core);

#endif
