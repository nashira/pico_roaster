#include "os.h"
#include <Arduino.h>

#define MIN_ALARM_SCHEDULE_US (20)

static alarm_pool_t *core0_alarm_pool;
static alarm_pool_t *core1_alarm_pool;
static queue_t core0_queue;
static queue_t core1_queue;

int64_t core0_alarm_handler(alarm_id_t id, void *user_data) {
    task_t *c = (task_t*)user_data;

    // Serial.print(id);
    // Serial.print(" ");
    // Serial.println(c->alarm_id);

    if (!queue_try_add(&core0_queue, c)) {
        Serial.println("core0_queue was full");
    }

  return c->period_us;
}

int64_t core1_alarm_handler(alarm_id_t id, void *user_data) {
    task_t *c = (task_t*)user_data;

    if (!queue_try_add(&core1_queue, c)) {
        Serial.println("core1_queue was full");
    }

  return c->period_us;
}

void core0_init() {
  core0_alarm_pool = alarm_pool_create_with_unused_hardware_alarm(128);
  queue_init(&core0_queue, sizeof(task_t), 32);
}

// must be called from core1
void core1_init() {
  core1_alarm_pool = alarm_pool_create_with_unused_hardware_alarm(128);
  queue_init(&core1_queue, sizeof(task_t), 32);
}

void core0_schedule(uint64_t delay_us, task_t *task) {
  delay_us = max(delay_us, MIN_ALARM_SCHEDULE_US);
  Serial.printf("core0 delay: %d\n", delay_us);
  Serial.printf("core0 period: %d\n", task->period_us);
  task->alarm_id = alarm_pool_add_alarm_in_us(core0_alarm_pool, delay_us, core0_alarm_handler, task, true);
  Serial.printf("core0 alarm id: %d\n", task->alarm_id);
}

void core1_schedule(uint64_t delay_us, task_t *task) {
  delay_us = max(delay_us, MIN_ALARM_SCHEDULE_US);
  Serial.printf("core1 delay: %d\n", delay_us);
  Serial.printf("core1 period: %d\n", task->period_us);
  task->alarm_id = alarm_pool_add_alarm_in_us(core1_alarm_pool, delay_us, core1_alarm_handler, task, true);
  Serial.printf("core1 alarm id: %d\n", task->alarm_id);
}

bool core0_cancel(alarm_id_t alarm_id) {
  Serial.printf("core0 cancel: %d\n", alarm_id);
  return alarm_pool_cancel_alarm(core0_alarm_pool, alarm_id);
}

bool core1_cancel(alarm_id_t alarm_id) {
  return alarm_pool_cancel_alarm(core1_alarm_pool, alarm_id);
}

void core0_execute() {
  task_t task;
  if (queue_try_remove(&core0_queue, &task)) {
    // uint64_t s = time_us_64();
    task.execute(task.user_data);
    // uint64_t e = time_us_64();
    // Serial.print(task.alarm_id);
    // Serial.printf(" elapsed us: %d\n", e - s);
  }
}

void core1_execute() {
  task_t task;
  if (queue_try_remove(&core1_queue, &task)) {
    task.execute(task.user_data);
  }
}
