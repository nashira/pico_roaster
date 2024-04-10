#ifndef _SENSORS_H_
#define _SENSORS_H_
#include "os.h"

#define VOLTAGE_PIN 27
#define VOLTAGE_GAIN 0.005f
#define ANGLE_GAIN 0.01f

#define SENSORS_CORE CORE1

void sensors_init();
float sensors_voltage();
float sensors_angle();

#endif