#include "temperature.h"
#include <Adafruit_MAX31855.h>
#include <CircularBuffer.hpp>

static Adafruit_MAX31855 probe1(PROBE_1_PIN, &SPI1);
static Adafruit_MAX31855 probe2(PROBE_2_PIN, &SPI1);

static CircularBuffer<float,30> buffer1;
static CircularBuffer<float,30> buffer2;

void on_temperature_init(void *data);
static task_t on_temperature_init_task = {
    .execute = on_temperature_init,
};

void read_probes(void *data);
static task_t read_probes_task = {
    .execute = read_probes,
    .period_us = -SAMPLE_DELAY_US
};

float temperature1() {
  return buffer1.last();
}

float temperature2() {
  return buffer2.last();
}

float temperature1_dx() {
  return (buffer1.last() - buffer1.first()) * DEG_C_PER_MIN;
}

float temperature2_dx() {
  return (buffer2.last() - buffer2.first()) * DEG_C_PER_MIN;
}

void temperature_init() {
  core1_schedule(INIT_DELAY_US, &on_temperature_init_task);
}

void on_temperature_init(void *data) {
  probe1.begin();
  probe2.begin();

  float temp1 = probe1.readCelsius();
  float temp2 = probe2.readCelsius();
  for(int i = 0; i < 30; i++) {
    buffer1.push(temp1);
    buffer2.push(temp2);
  }

  core1_schedule(0, &read_probes_task);
}

void read_probes(void *data) {
  float t1 = probe1.readCelsius();
  float t2 = probe2.readCelsius();
  if (isnan(t1) || isinf(t1)) {
    t1 = buffer1.last();
  } else {
    t1 = buffer1.last() + (t1 - buffer1.last()) * GAIN;
  }
  if (isnan(t2) || isinf(t2)) {
    t2 = buffer2.last();
  } else {
    t2 = buffer2.last() + (t2 - buffer2.last()) * GAIN;
  }

  buffer1.push(t1);
  buffer2.push(t2);
}
