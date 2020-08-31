#include "SpeedController.h"
static int zaehler = 0; 
SpeedController::SpeedController(double POLYTROPENEXPONENT, int UNIT_CONVERSION_FACTOR) {
  // Typische Werte zum Initiasilieren
  // Control_gain=0.4
  // Polytropenexponent=1
  Polytropenexponent = POLYTROPENEXPONENT;
  i_schnecke = 6.25;
  i_spindel = 6;
  max_speed=120000; //Umdrehungen pro Minute /100
  control_power=1;
  unit_conversion_factor=UNIT_CONVERSION_FACTOR;
  heightOld=0;
  pressureOld=0;
  k_factor=0.01;
}



double SpeedController::EstimatePressure(double CURRENT_HEIGHT, double CURRENT_PRESSURE) {
        // For first loop initialization of old variables with current ones
        if (CURRENT_HEIGHT == 0)
        {
            heightOld = CURRENT_HEIGHT;
            pressureOld = CURRENT_PRESSURE;
        }
        double estPressureNew = pow((heightOld / CURRENT_HEIGHT) , Polytropenexponent) * pressureOld;
        
        // Correct estimated value if estimated Values differs over 1 bar from actual value
        if (abs(estPressureNew-CURRENT_PRESSURE)>1) {
          estPressureNew=CURRENT_PRESSURE;
        } else {
          estPressureNew = (1 - k_factor ) * estPressureNew + k_factor * CURRENT_PRESSURE;
        }

        pressureOld = estPressureNew; // Save pressure of the estimator as input for the next loop
        heightOld = CURRENT_HEIGHT; // Save height from current loop for the estimation in next loop
        return estPressureNew;
}


double SpeedController::CalcSpeed(double IST_DRUCK, double SOLL_DRUCK, double LUFTVOLUMEN, double Control_gain) {
  //Eingabeeinheiten: 
  //    Ist-Druck: Relativdruck (gegenüber Athmosphäre) in bar 
  //    Soll-Druck: Relativdruck (gegenüber Athmosphäre) in 0.1 bar
  //    Hoehe-Luftsäule: Millimiter
  
  
// Preprocessing der Eingabevariablen in richtiges Format
  double HOEHE_LUFTSAEULE=LUFTVOLUMEN*1000/(pow(25,2)*3.1415);
  double IST_DRUCK_ABS = IST_DRUCK+1;
  SOLL_DRUCK /= 10.;




/* DEBUG SECTION ----------------

  Serial.print("Control_gain: ");
  Serial.println(Control_gain);
  Serial.print("Polyexp: ");
  Serial.println(Polytropenexponent);
  Serial.print("i-schnekce: ");
  Serial.println(i_schnecke);
  Serial.print("i-spindel: ");
  Serial.println(i_spindel);

------------------------------*/



//Originaler Linearisierender Regler
  double speed = (-Control_gain*IST_DRUCK + Control_gain*SOLL_DRUCK)*HOEHE_LUFTSAEULE/(Polytropenexponent*IST_DRUCK_ABS*i_spindel*i_schnecke); //Umdrehungen pro sekunde
  
//Regler mit Exponentiell gewichteter Regelabweichung -> Schnelleres hinfahren des Reglers an den Solldruck  
 // double speed = (-Control_gain*IST_DRUCK + Control_gain*SOLL_DRUCK)/pow(abs(-Control_gain*IST_DRUCK + Control_gain*SOLL_DRUCK), 1-control_power); //Berechnung der Regelabweichtung mit Exponentialgewicht
  //speed *= HOEHE_LUFTSAEULE/(Polytropenexponent*IST_DRUCK_ABS*i_spindel*i_schnecke);

// Postprocessing: Einheitenkonversion und Limitierung   
  speed*=60.; //Umrechnung von U/s auf U/min
  speed*=unit_conversion_factor; //Umrechnung auf die Geschwindigkeitseinheit der Steuerung
  speed=LimitSpeed(speed);


/* DEBUG SECTION ----------------
   
  Serial.print("SPEED 0: ");
  Serial.println(speed);
  Serial.print("SPEED: ");
  Serial.println(speed);
  if(zaehler++ > 100){
  Serial.print("IST_DRUCK_ABS: ");
  Serial.println(IST_DRUCK_ABS);
  Serial.print("SOLL_DRUCK: ");
  Serial.println(SOLL_DRUCK);
  Serial.print("LUFTSÄULENHOEHE: ");
  Serial.println(HOEHE_LUFTSAEULE);
  Serial.print("SPEED: ");
  Serial.println(speed);  
    zaehler = 0;
  }
  
------------------------------*/

// Rückgabe der Sollgeschwindigkeit
  return speed; // Rückgabeeinheit: Umdrehung pro Minute
                //Bei einer positiven Rückgabegeschwindigkeit muss Verdichtet werden -> "Nach unten fahren"
}

double SpeedController::LimitSpeed(double INPUT_SPEED){
 // Umständlich aber hab gerade keine vordefiniete Funktion gefunden....
 // Eingabeeinheit=Umdrehungen pro Minute
 if (INPUT_SPEED>=0)
 {
   if (INPUT_SPEED>max_speed)
   {
     return max_speed;
   }
   else
   {
     return INPUT_SPEED;
   }
 } else
 {
      if (INPUT_SPEED<-max_speed)
   {
     return -max_speed;
   }
   else
   {
     return INPUT_SPEED;
   }
 }
}


long SpeedController::CalcPosition(double IST_DRUCK, double SOLL_DRUCK, long ACTUAL_POSITION, int V_FLUESSIGKEIT) {

//Lokale Variablen:
long Pos_Fluessigkeit;
long Pos_Luft;
long delta_Pos;
long newPos;


//Eingabeeinheiten: 
  //    Ist-Druck: Relativdruck (gegenüber Athmosphäre) in bar 
  //    Soll-Druck: Relativdruck (gegenüber Athmosphäre) in 0.1 bar
  //    Actual Position: Positionseinheit der Steuerung (Schritte)
  //    V_Fluessigkeit: in ml

//Umrechungen
IST_DRUCK+= 1.013;   // Umrechunng von Relativdruck in Absolutdruck
SOLL_DRUCK/= 10;  // Umrechnung von 0.1 bar in bar
SOLL_DRUCK+= 1.013; // Umrechunng von Relativdruck in Absolutdruck

Pos_Fluessigkeit=V_FLUESSIGKEIT*1000/(pow(25,2) * 3.14159) * 2083L; // TODO: Umrechnung von ml auf die Einheit der Steuerung, d.h. 10ml Flüssigkeit entspricht bspw. 10000 Schritte

Pos_Luft=ACTUAL_POSITION-Pos_Fluessigkeit; // Höhe der Luftsäule in Positionseinheit der Steuerung


//Regelgesetz:
delta_Pos=( pow((IST_DRUCK/SOLL_DRUCK),(1/Polytropenexponent)) - 1) * Pos_Luft;

// Errechne Zielposition:
newPos=ACTUAL_POSITION+delta_Pos;

return newPos; //Gib die neue errechnete Zielposition zurück.

}