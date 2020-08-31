#include <Arduino.h>
#include "controller.h"

Controller controller;
void setup() {
  controller.init();
}

void loop() {
  controller.doLoop();
}