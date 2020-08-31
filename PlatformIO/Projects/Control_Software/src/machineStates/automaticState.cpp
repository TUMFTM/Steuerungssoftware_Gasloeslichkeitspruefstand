#include "machineState.h"
#include "controller.h"
#include "canMessageController.h"
#include <Adafruit_ADS1015.h>
#include <CmdMessenger.h>
#include "targetSizeGenerator.h"
#include "SpeedController.h"
#include <SimpleTimer.h>
#include <RTClib.h>
#include <SdFat.h>

//############################## OBJECT FUNCTIONS - AUTOMATIC STATE ##############################//


HeavisideGenerator* AutomaticState::targetSizeGenerator = new HeavisideGenerator();
SpeedController* AutomaticState::speedController = new SpeedController(1.1, Controller::conversionFactor);

double AutomaticState::airVolume = 0;
double AutomaticState::deadVolume = 25.7; // in ml
int AutomaticState::zylinderRadius = 25; // (in mm)

unsigned int AutomaticState::intParameters[AutomaticState::intParameterNumber];
double AutomaticState::doubleParameters[AutomaticState::doubleParameterNumber];
bool AutomaticState::boolParameters[AutomaticState::boolParameterNumber];
char* AutomaticState::stringParameters[AutomaticState::stringParameterNumber];

File AutomaticState::logFile;
unsigned int AutomaticState::datalogTimerID = 0;

char AutomaticState::dataLogFileName[50];

int AutomaticState::currentLoop = 1;


//LOG_VARIABLES

double AutomaticState::controlGain_log = 0;
int AutomaticState::targetPressure_log = 0;
long AutomaticState::targetSpeed_log = 0;
// Pressure Observer
// float AutomaticState::currentPressure = 0;

//############################## LOOP FUNCTION for Pressure Controller - AUTOMATIC STATE ##############################//
// Function performStateAction is called from Controller::doLoop function
void AutomaticState::performStateAction(){
    Serial.print("CurrentLoop: ");
    Serial.println(currentLoop);
    Serial.print("Int Paramt: ");
    Serial.println(intParameters[intParameterNumber - 3]);

    if(stateActionActive && currentLoop <= intParameters[intParameterNumber - 3]){
        // Update current Position
        Controller::canMessageCtrl.sendSdoUploadMessage(MOTOR_ID, 4, ACTUAL_POSITION, 0, sdoReceiveBuffer);    
        //convert byteArray of actual position to number
        byteArrayToNumeric(sdoReceiveBuffer, 4, MachineState::actualPosition);

        double height = actualPosition/2083.; // 2083 factor for 1 rotation per 1mm
        double heightair = height - (intParameters[intParameterNumber-2] - deadVolume) / (1000 * 3.14 * pow(zylinderRadius,2));
      
        int targetPressure = targetSizeGenerator->getTargetSize(millis());
        long stirrerSpeed = targetSizeGenerator->getStirrerSpeed(millis());
        double controlGain = targetSizeGenerator->getControlGain(millis());
       
       // Get pressure estimation Value from state observer
        estimatedPressure = speedController->EstimatePressure(heightair, currentPressure);

        airVolume = ((height * pow(zylinderRadius,2) * 3.14) / 1000) + deadVolume - intParameters[intParameterNumber-2];

        // Using sensorpressor for speed controller 
        // long speed = speedController->CalcSpeed(currentPressure, targetPressure, airVolume, controlGain);

        // Using estimated Pressure for Speed Controller
        long speed = speedController->CalcSpeed(estimatedPressure, targetPressure, airVolume, controlGain);
        targetSpeed_log = speed;
        targetPressure_log = targetPressure;
        controlGain_log = controlGain;
        unsigned long currentTime = millis()/1000;

        if(speed < 0){
            speed *= -1;
            numericToByteArray(sdoDataBuffer, 4, 0);
            Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 1, 0x607E, 0, sdoDataBuffer); // sets polarity
        } else {
            numericToByteArray(sdoDataBuffer, 4, 0x40);
            Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 1, 0x607E, 0, sdoDataBuffer); // sets polarity
        }
        
        numericToByteArray(sdoDataBuffer, 4, stirrerSpeed);
        Controller::canMessageCtrl.sendSdoDownloadMessage(STIRRER_ID, 4, TARGET_VELOCITY, 0, sdoDataBuffer);
        numericToByteArray(sdoDataBuffer, 4, speed);

        if(actualPosition > minPosition || actualPosition < maxPosition) {
            Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, TARGET_VELOCITY, 0, sdoDataBuffer);
        } else {
            Controller::canMessageCtrl.setControlword(MOTOR_ID, SWITCH_ON);
        }
        
        if(currentTime >= targetSizeGenerator->timepoints[4]){
            currentLoop++;
            stateActionActive = false;
            Controller::cmdMessenger.sendCmdStart(MACHINE_COMMAND::restartMeasurement);
            Controller::cmdMessenger.sendCmdArg(currentLoop);
            Controller::cmdMessenger.sendCmdEnd();
        }
    }
}

//############################## CALLBACK FUNCTIONS - AUTOMATIC STATE ##############################//

//CMDMESSENGER CALLBACKS
void AutomaticState::fillingMode(){

    char driveOut = Controller::cmdMessenger.readCharArg(); // driveOutOil = o , driveOutAir = a, stop = s
    Controller::canMessageCtrl.setModeOfOperation(MOTOR_ID, 1);
    double height = 0;
    long targetPos = 0;
    double deltaVolume = 0;
    if(driveOut == 'o'){
        
        intParameters[intParameterNumber-2] = Controller::cmdMessenger.readInt16Arg(); //read OilAmount (in 0.1 ml) 
        deltaVolume = intParameters[intParameterNumber-2] - deadVolume;
        deltaVolume = (deltaVolume < 0) ? 0 : deltaVolume;
        height = ((deltaVolume*1000) / (pow(zylinderRadius,2) * 3.14159)) + 5;
        targetPos = (long) (height * 2083L); //angenommen 2000 entspricht einer umdrehung = 0.96 mm kolben verschiebung
        numericToByteArray(sdoDataBuffer, 4, targetPos);
        Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, TARGET_POSITION, 0, sdoDataBuffer);
        Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::OPERATION_ENABLE);
        sdoDataBuffer[0] = 0b00011111; //starting positioning
        sdoDataBuffer[1] = 0; //starting positioning
        Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 2, CONTROLWORD, 0, sdoDataBuffer);

    } else if(driveOut == 'a') {

        intParameters[intParameterNumber-1] = Controller::cmdMessenger.readInt16Arg(); //read OilAmount (in 0.1 ml) 
        deltaVolume = intParameters[intParameterNumber-1] - deadVolume + intParameters[intParameterNumber-2];
        deltaVolume = (deltaVolume < 0) ? 0 : deltaVolume;
        height = ((deltaVolume*1000) / (pow(25,2) * 3.14159));
        targetPos = (long) (height * 2083L); //angenommen 2000 entspricht einer umdrehung = 0.96 mm kolben verschiebung
        numericToByteArray(sdoDataBuffer, 4, targetPos);
        Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, TARGET_POSITION, 0, sdoDataBuffer);
        Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::OPERATION_ENABLE);
        sdoDataBuffer[0] = 0b00011111; //starting positioning
        sdoDataBuffer[1] = 0; //starting positioning
        Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 2, CONTROLWORD, 0, sdoDataBuffer);
    } else {
        Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::SWITCH_ON);
    }
}

void AutomaticState::applyConfiguration(){

    for(int i = 0; i < intParameterNumber; i++){
        intParameters[i] = Controller::cmdMessenger.readInt16Arg();
    }

    for(int i = 0; i < boolParameterNumber; i++){
        boolParameters[i] = Controller::cmdMessenger.readBoolArg();
    }

    for(int i = 0; i < doubleParameterNumber; i++){
        doubleParameters[i] = Controller::cmdMessenger.readDoubleArg();
    }

    for(int i = 0; i < stringParameterNumber; i++){
        stringParameters[i] = Controller::cmdMessenger.readStringArg();
    }

    delete targetSizeGenerator;
    targetSizeGenerator = new HeavisideGenerator();
    
    delete speedController;
    speedController = new SpeedController(doubleParameters[doubleParameterNumber - 1], Controller::conversionFactor);
    
    Controller::timer.deleteTimer(datalogTimerID);
    unsigned long sampleRate = 1000L*10L/intParameters[10];
    datalogTimerID = Controller::timer.setInterval(sampleRate, AutomaticState::logData); // eingabe * 1000/10 für samples pro 10 sek.
    Controller::timer.disable(datalogTimerID);
    //AUSGABE

    // for(int i = 0; i < intParameterNumber; i++){ // -3 because last 2 parameters are set in fillingMode
    //     Serial.println(intParameters[i]);
    // }
    // for(int i = 0; i < doubleParameterNumber; i++){
    //     Serial.println(doubleParameters[i]);
    // }
    // for(int i = 0; i < boolParameterNumber; i++){
    //     Serial.println(boolParameters[i]);
    // }
    // for(int i = 0; i < stringParameterNumber; i++){
    //     Serial.println(stringParameters[i]);
    // }
    
}

void AutomaticState::startStopMeasurement(){
    bool start = Controller::cmdMessenger.readBoolArg();
    if(start){
        //set-up filename
        DateTime currentDate = Controller::rtc.now();
     //   snprintf(dataLogFileName, 50, "datalog_%d.%d.%d_%d-%d.csv", currentDate.day(),
      //              currentDate.month(), currentDate.year(), currentDate.hour(), currentDate.minute());
        snprintf(dataLogFileName, 50, "%d.%d.%d_%d-%d_datalog.csv", currentDate.year(),
            currentDate.month(), currentDate.day(), currentDate.hour(), currentDate.minute());

        char datalogConfigName[50];
        // snprintf(datalogConfigName, 50, "datalogConfig_%d.%d.%d_%d-%d.csv", currentDate.day(),
        //     currentDate.month(), currentDate.year(), currentDate.hour(), currentDate.minute());
        snprintf(datalogConfigName, 50, "%d.%d.%d_%d-%d_datalogConfig.csv", currentDate.year(),
            currentDate.month(), currentDate.day(), currentDate.hour(), currentDate.minute());

        //set-up filename end
        if(Controller::SD.exists(dataLogFileName)){
            Controller::SD.remove(dataLogFileName);
            Controller::SD.remove(datalogConfigName);
        }
        char headerConfig[2048];
        snprintf(headerConfig, 2048, "TempZeit: %d, TempDruck: %d, vorkZeit: %d, vorkDruck: %d, versZeit: %d, versDruck: %d, nachkZeit: %d, nachkDruck: %d, entspZeit: %d, entspDruck: %d, sampleRate: %d, ruehDrehz.: %d, ruehrPauseW.Verd.: %d, ruehrPauseW.Ent: %d, Fluess.Vol.: %d, LuftVol.: %d, ruehrW.Temp: %d, ruehrW.Vork: %d, ruehrW.Vers: %d, ruehrW.Nachk: %d, ruehrW.Ent: %d, Fluessigkeit: %s, reglVerst.: ",
        AutomaticState::intParameters[0], AutomaticState::intParameters[1], AutomaticState::intParameters[2], AutomaticState::intParameters[3],AutomaticState::intParameters[4],
        AutomaticState::intParameters[5],AutomaticState::intParameters[6],AutomaticState::intParameters[7],AutomaticState::intParameters[8],AutomaticState::intParameters[9],
        AutomaticState::intParameters[10],AutomaticState::intParameters[11],AutomaticState::intParameters[12],AutomaticState::intParameters[13],AutomaticState::intParameters[15],
        AutomaticState::intParameters[16], AutomaticState::boolParameters[0], AutomaticState::boolParameters[1], AutomaticState::boolParameters[2],
        AutomaticState::boolParameters[3],AutomaticState::boolParameters[4], AutomaticState::stringParameters[0]);

        File configFile = Controller::SD.open(datalogConfigName, FILE_WRITE);
        configFile.print(headerConfig);
        configFile.print(AutomaticState::doubleParameters[0]);
        configFile.print(", polyExp: ");
        configFile.println(doubleParameters[1]);
        configFile.close();

        File datalogFile = Controller::SD.open(dataLogFileName, FILE_WRITE);
        datalogFile.println("Time (ms), Pressure (bar), Temperature (°C), Position (mm), AirVolume (ml), Target Pressure (bar), Target Speed, Estimated Pressure (bar)");
        datalogFile.close();


        Controller::timer.restartTimer(datalogTimerID);
        Controller::timer.enable(datalogTimerID);
        Controller::canMessageCtrl.setModeOfOperation(MOTOR_ID, 3);
        numericToByteArray(sdoDataBuffer, 4, 0);
        Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, TARGET_VELOCITY, 0, sdoDataBuffer);

        numericToByteArray(sdoDataBuffer, 4, 30000);
        Controller::canMessageCtrl.sendSdoDownloadMessage(MOTOR_ID, 4, PROFILE_DECELERATION, 0, sdoDataBuffer);

        //START MAIN_MOTOR
        Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::SWITCH_ON);
        Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::OPERATION_ENABLE);

        //START STIRRER
        Controller::canMessageCtrl.setControlword(STIRRER_ID, MOTOR_STATE::SWITCH_ON);
        Controller::canMessageCtrl.setControlword(STIRRER_ID, MOTOR_STATE::OPERATION_ENABLE);
        
        currentLoop = 1;
        stateActionActive = true;
        
        timeOffset = millis()/1000;
        delete targetSizeGenerator;
        targetSizeGenerator = new HeavisideGenerator();
        
        delete speedController;
        speedController = new SpeedController(doubleParameters[doubleParameterNumber - 1], Controller::conversionFactor);
 

    } else {
        Controller::timer.disable(datalogTimerID);
        Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::SWITCH_ON);
        Controller::canMessageCtrl.setControlword(STIRRER_ID, MOTOR_STATE::SWITCH_ON);
        stateActionActive = false;
    }
}

void AutomaticState::restartMeasurement() {
    currentLoop = Controller::cmdMessenger.readInt16Arg();
}


//POLL (SOFTWARE) TIMER CALLBACKS

void AutomaticState::logData(){
    // Serial.println(dataLogFileName);
    AutomaticState::logFile = Controller::SD.open(dataLogFileName, FILE_WRITE);
  // if the file is available, write to it:
    logFile.print(millis()+ timeOffset);
    logFile.print(",");
    logFile.print(currentPressure);
    logFile.print(",");
    logFile.print(currentTemperature);
    logFile.print(",");
    logFile.print(actualPosition/2083.);
    logFile.print(",");
    logFile.print(airVolume);
    logFile.print(",");
    logFile.print(targetPressure_log);
    logFile.print(",");
    logFile.print(targetSpeed_log);
    logFile.print(",");
    logFile.println(estimatedPressure);
    logFile.close();
    
    // Old working Version of logging
    // logFile.print(millis()+ timeOffset);
    // logFile.print(",");
    // logFile.print(currentPressure);
    // logFile.print(",");
    // logFile.print(currentTemperature);
    // logFile.print(",");
    // logFile.print(actualPosition/2083.);
    // logFile.print(",");
    // logFile.print(airVolume);
    // logFile.print(",");
    // logFile.print(targetPressure_log);
    // logFile.print(",");
    // logFile.print(targetSpeed_log);
    // logFile.print(",");
    // logFile.println(controlGain_log);
    // logFile.close();
}