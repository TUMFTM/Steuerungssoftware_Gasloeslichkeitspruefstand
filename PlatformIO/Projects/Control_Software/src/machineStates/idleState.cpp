#include "machineState.h"
#include "controller.h"
#include "canMessageController.h"
#include <Adafruit_ADS1015.h>
#include <CmdMessenger.h>
#include <SimpleTimer.h>
#include <RTClib.h>
#include <SdFat.h>

const double homingVoltage = 2.46;
bool engineOn = false;
bool IdleState::homing = false;

//############################## OBJECT FUNCTIONS - IDLE STATE ##############################//

void IdleState::performStateAction(){
    if(homing){
        if (analogRead(0) * (5.0/1024) >= homingVoltage){
            homing = false;
            Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::HALT); // stops motor;
            Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::SWITCH_ON);
            
            numericToByteArray(sdoDataBuffer, 4, 0x23);
            Controller::canMessageCtrl.setModeOfOperation(MOTOR_ID, 6); //change to homing
            
            delay(1000); // wait till the motor stops
            Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 1 ,HOMING_METHOD, 0, sdoDataBuffer);
            
            sdoDataBuffer[0] = 0b00010111; //starting homing
            Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 2, CONTROLWORD, 0, sdoDataBuffer);
            Controller::canMessageCtrl.setModeOfOperation(MOTOR_ID, 3);
        }
        else if(analogRead(0) * (5.0/1024) > 0.7) {
            Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, TARGET_VELOCITY, 0 , sdoDataBuffer);
        } 
    }
    //Serial.println(analogRead(0)*(5.0/1024));
}


//############################## CALLBACK FUNCTIONS - IDLE STATE ##############################//

void IdleState::performHoming(){
   
    Controller::canMessageCtrl.setModeOfOperation(MOTOR_ID, 3); //change to profile velocity
    numericToByteArray(sdoDataBuffer, 4, 0x40);
    Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 1, 0x607E, 0, sdoDataBuffer); // sets polarity

    delay(100);
    numericToByteArray(sdoDataBuffer, 4, 200*Controller::conversionFactor);
    
    Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, TARGET_VELOCITY, 0 , sdoDataBuffer);
    numericToByteArray(sdoDataBuffer, 4, 50*Controller::conversionFactor);

    homing = true;
    Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::OPERATION_ENABLE); // starts motor;

}


void IdleState::calibratePressure(){
    pressureOffset = pressureGradient * Controller::analogDigitalConverter.readADC_SingleEnded(0) - 1; // y = m*x+b 
}

void IdleState::logDataStartStop(){
    const bool activateDataLogging = Controller::cmdMessenger.readBoolArg();
    if(activateDataLogging){
    AutomaticState::timeOffset = millis();
    DateTime currentDate = Controller::rtc.now();
    snprintf(AutomaticState::dataLogFileName, 100, "datalog_IDLE_%d.%d.%d_%d-%d.csv", currentDate.day(),
                    currentDate.month(), currentDate.year(), currentDate.hour(), currentDate.minute());
    char datalogConfigName[50];
    snprintf(datalogConfigName, 50, "datalogConfigIdle_%d.%d.%d_%d-%d.csv", currentDate.day(),
        currentDate.month(), currentDate.year(), currentDate.hour(), currentDate.minute());



    if(Controller::SD.exists(AutomaticState::dataLogFileName)){
        Controller::SD.remove(AutomaticState::dataLogFileName);
        Controller::SD.remove(datalogConfigName);
    }
    // prepare string for measurementConfig
    char headerConfig[2048];    
    snprintf(headerConfig, 2048, "TempZeit: %d, TempDruck: %d, vorkZeit: %d, vorkDruck: %d, versZeit: %d, versDruck: %d, nachkZeit: %d, nachkDruck: %d, entspZeit: %d, entspDruck: %d, sampleRate: %d, ruehDrehz.: %d, ruehrPauseW.Verd.: %d, ruehrPauseW.Ent: %d, Fluess.Vol.: %d, LuftVol.: %d, ruehrW.Temp: %d, ruehrW.Vork: %d, ruehrW.Vers: %d, ruehrW.Nachk: %d, ruehrW.Ent: %d, Fluessigkeit: %s, reglVerst.: ",
    AutomaticState::intParameters[0], AutomaticState::intParameters[1], AutomaticState::intParameters[2], AutomaticState::intParameters[3],AutomaticState::intParameters[4],
    AutomaticState::intParameters[5],AutomaticState::intParameters[6],AutomaticState::intParameters[7],AutomaticState::intParameters[8],AutomaticState::intParameters[9],
    AutomaticState::intParameters[10],AutomaticState::intParameters[11],AutomaticState::intParameters[12],AutomaticState::intParameters[13],AutomaticState::intParameters[14],
    AutomaticState::intParameters[15], AutomaticState::boolParameters[0], AutomaticState::boolParameters[1], AutomaticState::boolParameters[2],
    AutomaticState::boolParameters[3],AutomaticState::boolParameters[4], AutomaticState::stringParameters[0]);

    File configFile = Controller::SD.open(datalogConfigName, FILE_WRITE);
    configFile.print(headerConfig);
    configFile.print(AutomaticState::doubleParameters[0]);
    configFile.print(", polyExp: ");
    configFile.println(AutomaticState::doubleParameters[1]);
    configFile.close();
    // end of measurement config
    File datalogFile = Controller::SD.open(AutomaticState::dataLogFileName, FILE_WRITE);
    datalogFile.println("Time, Pressure, Temperature, Position (mm), AirVolume");
    datalogFile.close();

    Controller::timer.restartTimer(AutomaticState::datalogTimerID);
    Controller::timer.enable(AutomaticState::datalogTimerID);
    } else{
        AutomaticState::logFile.close();
        Controller::timer.disable(AutomaticState::datalogTimerID);
    }
}

