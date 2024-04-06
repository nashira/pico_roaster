#include "sensors.h"
#include <AS5600.h>

static AS5600L as5600(AS5600_DEFAULT_ADDRESS, &Wire);
static float voltage;
static float angle;

void sensors_read_as5600(void *data);
static task_t sensors_read_as5600_task = {
  .execute = sensors_read_as5600,
  .period_us = -ftous(1000)
};
void sensors_read_voltage(void *user_data);
static task_t sensors_read_voltage_task = {
  .execute = sensors_read_voltage,
  .period_us = -ftous(1000)
};

void sensors_init() {
  Wire.setSDA(20);
  Wire.setSCL(21);
  Wire.setClock(1'000'000);
  Wire.begin();

  as5600.begin();
  as5600.setDirection(AS5600_COUNTERCLOCK_WISE);

  analogReadResolution(12);

  core1_schedule(0, &sensors_read_as5600_task);
  core1_schedule(0, &sensors_read_voltage_task);
}

void sensors_read_as5600(void *data) {
  // gauge has a 360˚ range of 0 to 20 inh2o
  // with a dead range of 0 0.4 
  float reading = as5600.readAngle() * 20.f / 4096.f;
  if (reading < 0.5) reading = 0.f;
  angle += (reading - angle) * ANGLE_GAIN;
}

void sensors_read_voltage(void *user_data) {
  const float conversion_factor = (2180.0f + 14850.0f) / 2180.0f * 3.3f / (1 << 12);
  float reading = analogRead(VOLTAGE_PIN) * conversion_factor - 0.098f;
  voltage += (reading - voltage) * VOLTAGE_GAIN;
}

float sensors_voltage() {
  return voltage;
}

float sensors_angle() {
  return angle;
}
