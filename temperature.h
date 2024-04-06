#ifndef _TEMPERATURE_H_
#define _TEMPERATURE_H_
#include "os.h"

#define GAIN 0.005
#define INIT_DELAY_US 500'000
#define SAMPLE_DELAY_US ftous(10)
#define DEG_C_PER_MIN 20.0
#define PROBE_1_PIN 1
#define PROBE_2_PIN 4

float temperature1();
float temperature2();
float temperature1_dx();
float temperature2_dx();

void temperature_init();

#endif
