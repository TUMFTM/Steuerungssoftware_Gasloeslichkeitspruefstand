#include "canMessageController.h"
CanMessage::CanMessage(){
}

void CanMessage::init(){
    mcpCan = MCP_CAN(53); // CS of CAN_Module, connected to pin 53 on MEGA!
    
    // mcpCan.init_Mask(0, 0, 0x7FF00FF); // set 11-bit to 1 to 'mask' canID
    // mcpCan.init_Mask(1, 0, 0x7FF00FF); // also for mask register 2
    // mcpCan.init_Filt(0, 0, 0b10111111111000000001000000); //set all filters to SDOReceive Service + NodeId: 0x7F and actual_position
    // mcpCan.init_Filt(1, 0, 0b10111111111000000001000000);
    // mcpCan.init_Filt(2, 0, 0b10111111111000000001000000);
    // mcpCan.init_Filt(3, 0, 0b10111111111000000001000000);
    // mcpCan.init_Filt(4, 0, 0b10111111111000000001000000);
    // mcpCan.init_Filt(5, 0, 0b10111111111000000001000000);
    // Serial.println((unsigned long) (MessageType::SDOReceive | STIRRER_ID) | 0x64, BIN);
    while (CAN_OK != mcpCan.begin(CAN_1000KBPS)) {            
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
    }
    Serial.println("CAN BUS Shield init ok!");
    //initModeAndUnits(STIRRER_ID);
    setControlword(MOTOR_ID, UNKNOWN);
    setControlword(STIRRER_ID, UNKNOWN);
    Serial.println("motor switched on!");
    Serial.println("STIRRER switched on!");

}

void CanMessage::initModeAndUnits(byte deviceId){
    /*
    sendSdoDownloadMessage(deviceId, 1, TARGET_VELOCITY, 0, 3); //set Mode to profile velocity;
    sendSdoDownloadMessage(deviceId, 1, 0x2063, 0, 0x01); // set user definded Unit for acceleration: (0x2063/0x2064) * 0x60C5
    sendSdoDownloadMessage(deviceId, 1, 0x2064, 0, 60); // set user definded Unit for acceleration: (0x2063/0x2064) * 0x60C5
    */
}

// void CanMessage::sendCanMessage(INT8U dwa, INT8U daw, INT8U daw){

// }

void CanMessage::sendSdoUploadMessage(byte deviceId, byte dataLength, word objIndex, byte subIndex, byte data[]){
    for(int i = 0; i < dataLength; i++){
        data[i] = 0;
    }
    delay(2);
    const int sdoDataSize = 8;
    byte sdoData[sdoDataSize] = {0x40, (byte) objIndex, (byte) (objIndex >> 8), subIndex};
    
    mcpCan.sendMsgBuf(MessageType::SDO + deviceId, 0, sdoDataSize, sdoData);
    unsigned char len = 0;
    sdoData[1] = 0;
    delay(2); // both delays are needed to give enough time for module to receive all CAN_Messages
    while(mcpCan.checkReceive() == CAN_MSGAVAIL){
        mcpCan.readMsgBuf(&len, sdoData);
        if((objIndex & 0x00FF) == sdoData[1] && subIndex == sdoData[3]){
            break;
        }
    }
    for(int i = 0; i < dataLength; i++){
        data[i] = sdoData[4 + i];
    }
}

 void CanMessage::sendSdoDownloadMessage(byte deviceId, byte dataLength, word objIndex, byte subIndex, byte data[]){
     delay(2);
     const int sdoDataSize = 8;
     byte sdoData[sdoDataSize] = {SDO_downloadLengthMapping[dataLength - 1], (byte) objIndex, (byte) (objIndex >> 8), subIndex};
     for(int i = 0; i < dataLength; i++){
         sdoData[4 + i] = data[i];
     }
   
     mcpCan.sendMsgBuf(MessageType::SDO + deviceId, 0, sdoDataSize, sdoData);
}

// void CanMessage::sendNmtMessage(byte deviceId, NMT_COMMANDS cmd){
//     byte dataBuffer[2] = {cmd, deviceId};
//     mcpCan.sendMsgBuf(0, 0, 2, dataBuffer);
// }


void CanMessage::setControlword(byte deviceId, MOTOR_STATE controlWord){
    byte data[2] = {0x06, 0x00};
    switch (controlWord)
    {
    case READY_TO_SWITCH_ON: sendSdoDownloadMessage(deviceId, 2, CONTROLWORD, 0, data); // controlword -> ready to switch on
        break;
    case SWITCH_ON: data[0] = 0x07; sendSdoDownloadMessage(deviceId, 2, CONTROLWORD, 0, data); // controlword -> switch on
        break;
    case OPERATION_ENABLE: data[0] = 0x0F; sendSdoDownloadMessage(deviceId, 2, CONTROLWORD, 0, data); // controlword -> operation enable
        break;
    case HALT: data[0] = 0x0F; data[1] = 0x01; sendSdoDownloadMessage(deviceId, 2, CONTROLWORD, 0, data); // controlword -> HALT
        break;
    default:
    sendSdoDownloadMessage(deviceId, 2, CONTROLWORD, 0, data); // controlword -> ready to switch on
    data[0] = 0x07;
    delay(100);
    sendSdoDownloadMessage(deviceId, 2, CONTROLWORD, 0, data); // controlword -> switch on
    // data[0] = 0x0F;
    // delay(100);
    // sendSdoDownloadMessage(deviceId, 2, CONTROLWORD, 0, data, false); // controlword -> operation enable
    }
}

void CanMessage::setModeOfOperation(byte deviceId, byte mode){
    byte data[2] = {mode, 0};
    sendSdoDownloadMessage(deviceId, 1, MODE_OF_OPERATION, 0, data); // MODE_OF_OPERATION -> set
}


