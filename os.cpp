#include "os.h"
#include <Arduino.h>

#define MIN_ALARM_SCHEDULE_US (20)

static alarm_pool_t *core0_alarm_pool;
static alarm_pool_t *core1_alarm_pool;
static queue_t core0_queue;
static queue_t core1_queue;

void core0_init();
void core0_schedule(uint64_t delay_us, task_t *task);
bool core0_cancel(alarm_id_t alarm_id);
void core0_execute();

void core1_init();
void core1_schedule(uint64_t delay_us, task_t *task);
bool core1_cancel(alarm_id_t alarm_id);
void core1_execute();

int64_t core0_alarm_handler(alarm_id_t id, void *user_data) {
    task_t *c = (task_t*)user_data;
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

void core_init(uint8_t core) {
  if (core == CORE0) {
    core0_init();
  } else if (core == CORE1) {
    core1_init();
  }
}

void task_schedule(uint8_t core, uint64_t delay_us, task_t *task) {
  if (core == CORE0) {
    core0_schedule(delay_us, task);
  } else if (core == CORE1) {
    core1_schedule(delay_us, task);
  }
  Serial.printf("core%d delay: %d, ", core, delay_us);
  Serial.printf("period: %d, ", task->period_us);
  Serial.printf("alarm id: %d\n", task->alarm_id);
}

bool task_cancel(uint8_t core, alarm_id_t alarm_id) {
  Serial.printf("core%d cancel: %d\n", core, alarm_id);
  if (core == CORE0) {
    return core0_cancel(alarm_id);
  } else if (core == CORE1) {
    return core1_cancel(alarm_id);
  }
  return false;
}

void task_execute(uint8_t core) {
  if (core == CORE0) {
    core0_execute();
  } else if (core == CORE1) {
    core1_execute();
  }
}

// must be called from core0
void core0_init() {
  core0_alarm_pool = alarm_pool_create_with_unused_hardware_alarm(32);
  queue_init(&core0_queue, sizeof(task_t), 32);
}

// must be called from core1
void core1_init() {
  core1_alarm_pool = alarm_pool_create_with_unused_hardware_alarm(32);
  queue_init(&core1_queue, sizeof(task_t), 32);
}

void core0_schedule(uint64_t delay_us, task_t *task) {
  delay_us = max(delay_us, MIN_ALARM_SCHEDULE_US);
  task->alarm_id = alarm_pool_add_alarm_in_us(core0_alarm_pool, delay_us, core0_alarm_handler, task, true);
}

void core1_schedule(uint64_t delay_us, task_t *task) {
  delay_us = max(delay_us, MIN_ALARM_SCHEDULE_US);
  task->alarm_id = alarm_pool_add_alarm_in_us(core1_alarm_pool, delay_us, core1_alarm_handler, task, true);
}

bool core0_cancel(alarm_id_t alarm_id) {
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
