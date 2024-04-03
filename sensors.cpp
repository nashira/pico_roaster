#include "sensors.h"
#include <AS5600.h>


static AS5600L as5600(AS5600_DEFAULT_ADDRESS, &Wire1);
static float voltage;
static float angle;

void sensors_read_as5600(void *data);
static task_t sensors_read_as5600_task = {
  .execute = sensors_read_as5600,
  .period_us = ftous(5)
};
void sensors_read_voltage(void *user_data);
static task_t sensors_read_voltage_task = {
  .execute = sensors_read_voltage,
  .period_us = ftous(5)
};

void sensors_init() {
  Wire1.setSDA(26);
  Wire1.setSCL(27);
  Wire1.setClock(1000000);
  Wire1.begin();

  as5600.begin();
  as5600.setDirection(AS5600_COUNTERCLOCK_WISE);

  analogReadResolution(12);

  core1_schedule(0, &sensors_read_as5600_task);
  core1_schedule(0, &sensors_read_voltage_task);
}

void sensors_read_as5600(void *data) {
  angle = as5600.readAngle() * 360.f / 4095.f;
}

void sensors_read_voltage(void *user_data) {
  const float conversion_factor = (2180.0f + 14830.0f) / 2180.0f * 3.23f / (1 << 12);
  float reading = analogRead(VOLTAGE_PIN) * conversion_factor - 0.03f;
  voltage += (reading - voltage) * VOLTAGE_GAIN;
}

float sensors_voltage() {
  return voltage;
}

float sensors_angle() {
  return angle;
}
