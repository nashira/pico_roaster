
#include "os.h"
#include "temperature.h"
#include "display.h"
#include "network.h"
#include "sensors.h"

void setup() {
  Serial.begin();
  while(!Serial);

  core0_init();
  
  network_init();
  display_init();

  while(true) {
    core0_execute();
    network_serve_clients();
  }
}

void setup1() {
  core1_init();

  temperature_init();
  sensors_init();

  while(true) core1_execute();
}

void loop() {
  // nop
}
