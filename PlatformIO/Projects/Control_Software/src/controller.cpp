#include "controller.h"
#include "canMessageController.h"
#include "machineState.h"
#include "targetSizeGenerator.h"
#include "SpeedController.h"

#include <CmdMessenger.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <SimpleTimer.h>
#include <SPI.h>
#include <SdFat.h>
#include <RTClib.h>

#define FILLARRAY(a,n) a[0]=n, memcpy( ((char*)a)+sizeof(a[0]), a, sizeof(a)-sizeof(a[0]));
/* initializing static variables */
const int chipSelect = 10;

MachineState* Controller::actualMachineState = new IdleState();
CmdMessenger Controller::cmdMessenger(Serial);
CanMessage Controller::canMessageCtrl;
Adafruit_ADS1115 Controller::analogDigitalConverter;
int Controller::conversionFactor = 100;

SdFat Controller::SD;

RTC_PCF8523 Controller::rtc;
SimpleTimer Controller::timer;




// Controller method definitions
void Controller::init(){
//    configure cmdMessenger and callback functions
    Controller::cmdMessenger.attach(MACHINE_COMMAND::rotate, ManualState::rotateCallback);
    Controller::cmdMessenger.attach(MACHINE_COMMAND::setObject, ManualState::setObjectCallback);
    Controller::cmdMessenger.attach(MACHINE_COMMAND::changeState, MachineState::changeState);
    Controller::cmdMessenger.attach(MACHINE_COMMAND::fillingMode, AutomaticState::fillingMode);
    Controller::cmdMessenger.attach(MACHINE_COMMAND::applyConfiguration, AutomaticState::applyConfiguration);
    Controller::cmdMessenger.attach(MACHINE_COMMAND::startMeassurement, AutomaticState::startStopMeasurement);
    Controller::cmdMessenger.attach(MACHINE_COMMAND::calibratePreassure, IdleState::calibratePressure);
    Controller::cmdMessenger.attach(MACHINE_COMMAND::performHoming, IdleState::performHoming);
    Controller::cmdMessenger.attach(MACHINE_COMMAND::logDataIdle, IdleState::logDataStartStop);
    Controller::cmdMessenger.attach(MACHINE_COMMAND::restartMeasurement, AutomaticState::restartMeasurement);
    cmdMessenger.printLfCr();

    //AutomaticMode parameter setting the default params:   

    //configure dataLogger callback functions and sampleRate per minute
    AutomaticState::datalogTimerID = Controller::timer.setInterval(1000L, AutomaticState::logData);
    //MachineState::outputTimerID = Controller::timer.setInterval(500, MachineState::updateAndSendOutput);
    timer.disable(AutomaticState::datalogTimerID);
    //timer.disable(MachineState::outputTimerID);

    Serial.begin(9600);
    analogDigitalConverter.begin();
    
    // // initialize SD CARD

    while (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    delay(1000);
    }

    Serial.println("card initialized.");
    
    while(!rtc.begin()){
      Serial.println("Can't find RTC");
      delay(1000);
    }
    
    delay(2);
    if (! rtc.initialized() || rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    // // READ Configuration and send it to GUI
     //readConfigurationFromSDCard();

    // //CAN-Controller INITIALIZATION
    canMessageCtrl.init();
    
     //setting software position limit
    numericToByteArray(MachineState::sdoDataBuffer, 4, 0);
    Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, SOFTWARE_POSITION_LIMIT, 1, MachineState::sdoDataBuffer); // MIN position
    numericToByteArray(MachineState::sdoDataBuffer, 4, MachineState::maxPosition); // sets max position
    Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, SOFTWARE_POSITION_LIMIT, 2, MachineState::sdoDataBuffer); //MAX position
   
    //setting velocity and acceleration
    numericToByteArray(MachineState::sdoDataBuffer, 4, 200);
    Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, TARGET_VELOCITY, 0, MachineState::sdoDataBuffer);
    Controller::canMessageCtrl.sendSdoDownloadMessage(STIRRER_ID, 4, TARGET_VELOCITY, 0, MachineState::sdoDataBuffer);
    numericToByteArray(MachineState::sdoDataBuffer, 4, 30000);
    Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, PROFILE_ACCELERATION, 0, MachineState::sdoDataBuffer);
    Controller::canMessageCtrl.sendSdoDownloadMessage(STIRRER_ID, 4, PROFILE_ACCELERATION, 0, MachineState::sdoDataBuffer);
    Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, PROFILE_DECELERATION, 0, MachineState::sdoDataBuffer);
    Controller::canMessageCtrl.sendSdoDownloadMessage(STIRRER_ID, 4, PROFILE_DECELERATION, 0, MachineState::sdoDataBuffer);


    // MOTOR UNIT *100
    numericToByteArray(MachineState::sdoDataBuffer, 4, 6000);
    Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, 0x2062, 0, MachineState::sdoDataBuffer);
    //setts profile velocity for position mode
    // numericToByteArray(MachineState::sdoDataBuffer, 4, 50000);
    // Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, 0x6081, 0, MachineState::sdoDataBuffer);

    Controller::canMessageCtrl.setModeOfOperation(MOTOR_ID, 3); //change operation mode of Motor to profile velocity
    Controller::canMessageCtrl.setModeOfOperation(STIRRER_ID, 3); //change operation mode of stirrer to profile velocity
    
    numericToByteArray(MachineState::sdoDataBuffer, 4, 64);
    Controller::canMessageCtrl.sendSdoDownloadMessage(STIRRER_ID, 1, 0x607E, 0, MachineState::sdoDataBuffer); // sets polarity
}

// Looping of measurements, called from main.cpp in function loop
static int counter = 0;
void Controller::doLoop(){
    Controller::cmdMessenger.feedinSerialData();
    MachineState::updateAndSendOutput();
    //update actual position
    Controller::canMessageCtrl.sendSdoUploadMessage(MOTOR_ID, 4, ACTUAL_POSITION, 0, MachineState::sdoReceiveBuffer);
    byteArrayToNumeric(MachineState::sdoReceiveBuffer,4, MachineState::actualPosition);
    timer.run();
    // Run the Controller. Performstateaction is defined in 
    actualMachineState->performStateAction();
}

void Controller::readConfigurationFromSDCard(){
  
  if(!SD.exists("config.txt")){
    return;
  }

  File configFile = SD.open("config.txt", FILE_READ);
  unsigned int sumParams = AutomaticState::intParameterNumber + AutomaticState::boolParameterNumber + AutomaticState::doubleParameterNumber;
  char* configurationString;  //= "1,5,1,1,1,10,1,20,1,50,1,20,1,10,1,1,1,1,0,0,0,0.04,1.1,0,0,0,0,0;"; //comment OUT and comment in next line for testing

  //configFile.readStringUntil(';').toCharArray(configurationString, sumParams *2 -1); 
  configFile.readBytesUntil(';', configurationString, sumParams *2 - 1);
  
  char* stringTokenIndex = strtok(configurationString, ",");
  Controller::cmdMessenger.sendCmdStart(MACHINE_COMMAND::loadConfiguration);

  for(int i = 0; i < AutomaticState::intParameterNumber; i++){
    AutomaticState::intParameters[i] = atoi(stringTokenIndex);
    stringTokenIndex = strtok(NULL, ",");
    Controller::cmdMessenger.sendCmdArg(AutomaticState::intParameters[i]);
  }
  for(int i = 0; i < AutomaticState::boolParameterNumber; i++){
    AutomaticState::boolParameters[i] = (atoi(stringTokenIndex) == 0) ? 0: 1;
    stringTokenIndex = strtok(NULL, ",");
    Controller::cmdMessenger.sendCmdArg(AutomaticState::boolParameters[i]);
  }
    for(int i = 0; i < AutomaticState::doubleParameterNumber; i++){
    AutomaticState::doubleParameters[i] = atof(stringTokenIndex);
    stringTokenIndex = strtok(NULL, ",");
    Controller::cmdMessenger.sendCmdArg(AutomaticState::doubleParameters[i]);
  }
    Controller::cmdMessenger.sendCmdEnd();
    configFile.close();
   // configFile.close();
        
      //AUSGABE
    // for(int i = 0; i < AutomaticState::intParameterNumber-3; i++){ // -3 because last 2 parameters are set in fillingMode
    //     Serial.println(AutomaticState::intParameters[i]);
    // }
    // for(int i = 0; i < AutomaticState::doubleParameterNumber; i++){
    //     Serial.println(AutomaticState::doubleParameters[i]);
    // }
    // for(int i = 0; i < AutomaticState::boolParameterNumber; i++){
    //     Serial.println(AutomaticState::boolParameters[i]);
    // }
  
}

