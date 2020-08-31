#pragma once

#include <Arduino.h>
#include <mcp_can.h>
typedef enum {
    TARGET_VELOCITY = 0x60FF,
    TARGET_POSITION = 0x607A,
    ACTUAL_POSITION = 0x6064,
    HALT_REACTION = 0x605D,
    MODE_OF_OPERATION = 0x6060,
    CONTROLWORD = 0x6040,
    HOMING_METHOD = 0x6098,
    PROFILE_ACCELERATION = 0x6083,
    PROFILE_DECELERATION = 0x6084,
    HOMING_SPEED = 0x6099,
    SOFTWARE_POSITION_LIMIT = 0x607D,
} DICTIONARY_OBJECT;

typedef enum {
    STIRRER_ID = 0x7F,
    MOTOR_ID = 0x01
} DEVICE_IDS;

    typedef enum {
        NMT = 0x0,
        SDO = 0x600,
        SDOReceive = 0x580
    } MessageType;

    typedef enum {
        READY_TO_SWITCH_ON = 0,
        SWITCH_ON,
        OPERATION_ENABLE,
        HALT,
        QUICK_STOP = 2,
        UNKNOWN = 100,
    } MOTOR_STATE;

class CanMessage
{
    byte SDO_downloadLengthMapping[4] = {0x2F, 0x2B, 0x27, 0x23};
    byte SDO_uploadLengthMapping[4] = {0x4F, 0x4B, 0x47, 0x43};
    private:
        MCP_CAN mcpCan;
    public:
        CanMessage();
        void init();
        void initModeAndUnits(byte deviceId);
        void sendSdoUploadMessage(byte deviceId, byte dataLength, word objIndex, byte subIndex, byte data[]);
        void sendSdoDownloadMessage(byte deviceId, byte dataLength, word objIndex, byte subIndex, byte data[]);
   //     void sendNmtMessage(byte, NMT_COMMANDS);
        void setControlword(byte deviceId, MOTOR_STATE controlWord);
        void setModeOfOperation(byte deviceId, byte mode);
        void setAcceleration(byte device, byte acc);
};



// HELPER FUNCTIONS

template <class T, class P>
void numericToByteArray(T resultArray[], int resultArraySize, P num){
    for(int i = 0; i < resultArraySize; i++){
        resultArray[i] = 0;
    }

    int convertArraySize = ( (int) sizeof(P) > resultArraySize) ? resultArraySize : (int) sizeof(P);
    for(int i = 0; i < convertArraySize; num >>= 8, i++) {
        resultArray[i] = num;
    }
}

template <class T, class P>
void byteArrayToNumeric(T byteArray[], int arrayLength, P& num){

    int sizeofDataType = ((int) sizeof(P) > arrayLength) ? arrayLength : (int) sizeof(P);
    num = byteArray[arrayLength - 1];
    for(int i = arrayLength - 2; i >= arrayLength - sizeofDataType; i--) { 
        num <<= 8;
        num |= byteArray[i];
    }
}

template <class T>
void printArray(T* array, int len){
  for(int i = 0; i < len; i++){
    Serial.print(array[i]); Serial.print(" ");
  }
  Serial.println();
}
