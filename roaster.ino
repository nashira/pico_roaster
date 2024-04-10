
#include "os.h"
#include "temperature.h"
#include "display.h"
#include "network.h"
#include "sensors.h"

void setup() {
  Serial.begin();

  core_init(CORE0);
  
  network_init();
  display_init();

  while(true) {
    task_execute(CORE0);
    network_serve_clients();
  }
}

void setup1() {
  core_init(CORE1);

  temperature_init();
  sensors_init();

  while(true) task_execute(CORE1);
}

void loop() {
  // nop
}
