#ifndef _SENSORS_H_
#define _SENSORS_H_
#include "os.h"

#define VOLTAGE_PIN 28
#define VOLTAGE_GAIN 0.001f

void sensors_init();
float sensors_voltage();
float sensors_angle();

#endif