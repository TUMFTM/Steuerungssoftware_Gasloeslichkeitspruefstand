#include "machineState.h"
#include "controller.h"
#include "canMessageController.h"
#include "SimpleTimer.h"
#include <CmdMessenger.h>
#include <Adafruit_ADS1015.h>
#include <Arduino.h>
#include "targetSizeGenerator.h"
#include "SpeedController.h"


//############################## CALLBACK FUNCTIONS - MACHINE STATE ##############################//

byte MachineState::sdoDataBuffer[4] = {0,0,0,0};
byte MachineState::sdoReceiveBuffer[4] = {0,0,0,0};
bool MachineState::stateActionActive = false;
float MachineState::currentPressure = 0;
float MachineState::estimatedPressure = 0;
double MachineState::currentTemperature = 0;
unsigned long MachineState::timeOffset = 0;

char MachineState::actualState = 'i';
long MachineState::minPosition = 0;
long MachineState::maxPosition = 490187;
double MachineState::pressureGradient = 1/266.66; 
double MachineState::pressureOffset = -0.24;
long MachineState::actualPosition = 0;
bool MachineState::polarityUp = true;

int MachineState::outputTimerID;

void MachineState::changeState(){

    //Controller::timer.disable(AutomaticState::datalogTimerID);
    actualState = Controller::cmdMessenger.readCharArg();
    delete Controller::actualMachineState;
    Controller::timer.disable(AutomaticState::datalogTimerID);
    switch (actualState)
    {
    case 'i': Controller::actualMachineState = new IdleState(); break;
    case 'm': Controller::actualMachineState = new ManualState(); break;
    case 'a': Controller::actualMachineState = new AutomaticState(); //Controller::readConfigurationFromSDCard(); /*breaks cmdMessenger callback!!*/ break;
    default : break;
    }
}

void MachineState::updatePressure(){
    int16_t adc0 = Controller::analogDigitalConverter.readADC_SingleEnded(0);
    float sumPressure = 0;

    // for(int i = 0,sample = 10 ; i < sample; i++){
    // sumPressure += AutomaticState::pressureGradient * adc0 - 1 - AutomaticState::pressureOffset;
    // }
    currentPressure = AutomaticState::pressureGradient * adc0 - 1 - AutomaticState::pressureOffset;//sumPressure/10.;
}


void MachineState::updateTemperature(){
    int16_t adc1; // we read from the ADC, we have a sixteen bit integer as a result
    float Voltage = 0.0; //refCurrent = 0.0001;
    int thermistor_25 = 10000;

    adc1 = Controller::analogDigitalConverter.readADC_SingleEnded(2); // Read ADC value from ADS1115
    Voltage = adc1 * (6.144 / 32768); // Replace 5.0 with whatever the actual Vcc of your Arduino is
    float resistance = (10000 / (5.0 / Voltage - 1)); // Using Ohm's Law to calculate resistance of thermistor
    // float ln = log(resistance / thermistor_25); // Log of the ratio of thermistor resistance and resistance at 25 deg. C

    float ln = log(resistance);
    // T in K by Steinhart-Hart Eq.
    float kelvin = 1 / (0.001129186694 + 0.0002341156686 * ln + 0.00000008772979888 * ln * ln * ln);

    // float kelvin = 1 / (0.0033540170 + (0.00025617244 * ln) + (0.0000021400943 * ln * ln) + (-0.000000072405219 * ln * ln * ln)); // Using the Steinhart-Hart Thermistor Equation to calculate temp in K
    currentTemperature = kelvin - 273.15; // Converting Kelvin to Celcuis
    //currentTemperature = ln ;
}
// Is called from Controller::doLoop to update the sensor signals and send realtime data to GUI
void MachineState::updateAndSendOutput(){
    updatePressure();
    updateTemperature();
    //SEND REALTIME OUTPUT TO GUI
    Controller::cmdMessenger.sendCmdStart(sendParameter);
    // Send Timer and TargetSpeed only if measurement is started
    // During Measurement the estimated Pressure is sent to the GUI
        if(AutomaticState::stateActionActive){
        Controller::cmdMessenger.sendCmdArg(estimatedPressure);
        Controller::cmdMessenger.sendCmdArg(currentTemperature);
        Controller::cmdMessenger.sendCmdArg((millis()/1000 - timeOffset));
        Controller::cmdMessenger.sendCmdArg(AutomaticState::targetSpeed_log /100);    
    } else { // If no Measurement is started or its in manual mode the currentPressure is sent, as the estimatedPressure ist not calculated  because AutomaticState::performStateAction is not called
        Controller::cmdMessenger.sendCmdArg(currentPressure);
        Controller::cmdMessenger.sendCmdArg(currentTemperature);
        Controller::cmdMessenger.sendCmdArg("--");
        Controller::cmdMessenger.sendCmdArg("--");
    }   
    Controller::cmdMessenger.sendCmdEnd();
}
