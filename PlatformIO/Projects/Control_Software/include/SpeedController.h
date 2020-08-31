/*
Geschwindigkeitsregler
*/
#pragma once

#include "Arduino.h"

class SpeedController
{
private:
  double Polytropenexponent;
  double i_schnecke;
  double k_factor;
  double heightOld;
  double pressureOld;
  double i_spindel;
  double max_speed;
  double control_power;
  double unit_conversion_factor;
  double LimitSpeed(double INPUT_SPEED);
public:
  SpeedController(double POLYTROPENEXPONENT, int UNIT_CONVERSION_FACTOR);
  double CalcSpeed(double IST_DRUCK, double SOLL_DRUCK, double LUFTVOLUMEN, double);
  double EstimatePressure(double CURRENT_HEIGHT, double CURRENT_PRESSURE);
  long CalcPosition(double IST_DRUCK, double SOLL_DRUCK, long ACTUAL_POSITION, int V_FLUESSIGKEIT);

};