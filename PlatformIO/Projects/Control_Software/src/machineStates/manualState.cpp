#include "machineState.h"
#include "controller.h"
#include "canMessageController.h"
#include <Adafruit_ADS1015.h>
#include <CmdMessenger.h>
#include <SimpleTimer.h>

//############################## OBJECT FUNCTIONS - MANUAL STATE ##############################//

//AUSGABE MUSS BESCHRÄNKT WERDEN DA SONST DIE UART MODULE ÜBERFORDERT SIND!
static int count = 0;
void ManualState::performStateAction()
{   
    if(count++ > 20000){
     Serial.println("Actual Position: ");
     Serial.println(MachineState::actualPosition);
     count = 0;
    }

    if(((MachineState::actualPosition <= MachineState::minPosition) && !polarityUp) || ((MachineState::actualPosition >= MachineState::maxPosition) && polarityUp)){
        Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::SWITCH_ON);
    }
    
}

//############################## CALLBACK FUNCTIONS - MANUAL STATE ##############################//

void ManualState::rotateCallback(){
    char direction = Controller::cmdMessenger.readCharArg();
    polarityUp = (direction == 'u') ? true : (direction == 'd') ? false : polarityUp;
    DEVICE_IDS DEVICE_ID = (Controller::cmdMessenger.readInt16Arg() == 127) ? STIRRER_ID : MOTOR_ID;
    Serial.println(DEVICE_ID);
    numericToByteArray(sdoDataBuffer, 4, 0);
    
    //moving up
    if(direction == 'u' && (actualPosition < MachineState::maxPosition ||  DEVICE_ID == STIRRER_ID)){ 
        Serial.println("PROBE");
        Controller::canMessageCtrl.sendSdoDownloadMessage(DEVICE_ID, 1, 0x607E, 0, sdoDataBuffer); // sets polarity
        delay(100);
        Controller::canMessageCtrl.setControlword(DEVICE_ID, MOTOR_STATE::OPERATION_ENABLE); // starts motor;
        //Controller::canMessageCtrl.setControlword(STIRRER_ID, MOTOR_STATE::OPERATION_ENABLE); // starts STIRRER;
        
    }
    //moving down
    else if (direction == 'd' && (actualPosition > MachineState::minPosition || DEVICE_ID == STIRRER_ID))
    {
        numericToByteArray(sdoDataBuffer, 4, 0x40);
        Controller::canMessageCtrl.sendSdoDownloadMessage(DEVICE_ID, 1, 0x607E, 0, sdoDataBuffer); // sets polarity
        //Controller::canMessageCtrl.sendSdoDownloadMessage(STIRRER_ID, 1, 0x607E, 0, sdoDataBuffer); // sets polarity
        delay(100);
        Controller::canMessageCtrl.setControlword(DEVICE_ID, MOTOR_STATE::OPERATION_ENABLE); // starts motor;
        //Controller::canMessageCtrl.setControlword(STIRRER_ID, MOTOR_STATE::OPERATION_ENABLE); // starts STIRRER;
    } else {
        Controller::canMessageCtrl.setControlword(MOTOR_ID, MOTOR_STATE::SWITCH_ON); // stops motor;
        Controller::canMessageCtrl.setControlword(STIRRER_ID, MOTOR_STATE::SWITCH_ON); // stops motor;
    } 
}

void ManualState::setObjectCallback(){
    char objectToSet = Controller::cmdMessenger.readCharArg(); // gets a char, which object to set
    unsigned short dataLength = Controller::cmdMessenger.readInt16Arg();
    byte dataBuffer[4];
    unsigned short element = 0;
    int DEVICE_ID = 0;
        element = Controller::cmdMessenger.readInt16Arg();
        dataBuffer[0] = (byte) element;
        dataBuffer[1] = (byte) (element >> 8);
        DEVICE_ID = Controller::cmdMessenger.readInt16Arg();
    if(dataLength > 1){
        element = Controller::cmdMessenger.readInt16Arg();
        Serial.println(element);
        dataBuffer[2] = (byte) element;
        dataBuffer[3] = (byte) (element >> 8);
        //DEVICE_ID = Controller::cmdMessenger.readInt16Arg();
        // Serial.println(DEVICE_ID);
    }
    Serial.println(dataBuffer[0]);
    Serial.println(dataBuffer[1]);
    Serial.println(dataBuffer[2]);
    Serial.println(dataBuffer[3]);
    

    switch (objectToSet)
    {
    case 'v': Controller::canMessageCtrl.sendSdoDownloadMessage(DEVICE_ID, dataLength, TARGET_VELOCITY, 0, dataBuffer);
    // dataBuffer[1] = dataBuffer[2] = dataBuffer[3] = 0;
    // dataBuffer[0] = 0xFF;
    // Controller::canMessageCtrl.sendSdoDownloadMessage(STIRRER_ID, dataLength, TARGET_VELOCITY, 0, dataBuffer); 
    break;
    case 'a': Controller::canMessageCtrl.sendSdoDownloadMessage(DEVICE_ID, dataLength, PROFILE_ACCELERATION, 0, dataBuffer);
              Controller::canMessageCtrl.sendSdoDownloadMessage(DEVICE_ID, dataLength, PROFILE_DECELERATION, 0, dataBuffer);
    break;
    }
}

