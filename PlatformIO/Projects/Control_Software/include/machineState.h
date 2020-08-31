#pragma once

#include <Arduino.h>
#include <SdFat.h>
class TargetSizeGenerator;
class HeavisideGenerator;
class SpeedController;

typedef enum {
        rotate = 1,
        setObject = 2,
        changeState = 3,
        fillingMode = 4,
        applyConfiguration = 5,
        startMeassurement = 6,
        calibratePreassure = 7,
        performHoming = 8,
        sendParameter = 9,
        loadConfiguration = 10,
        logDataIdle = 11,
        restartMeasurement = 12
    } MACHINE_COMMAND;

class MachineState
{
    public:
        static bool stateActionActive;
        static byte sdoDataBuffer[4];
        static byte sdoReceiveBuffer[4];
        static float currentPressure;
            // Pressure state observer
        static float estimatedPressure;
        static double currentTemperature;
        static int outputTimerID;
        static char actualState;
        static long minPosition;
        static long maxPosition;
        static long actualPosition;
        static bool polarityUp;

        static double pressureGradient;
        static double pressureOffset;

        static unsigned long timeOffset;

        static void updateTemperature();
        static void updatePressure();
        static void updateAndSendOutput();
        static void changeState();
        virtual void performStateAction() = 0;
        virtual ~MachineState(){};
        
};


class IdleState : public MachineState
{
    
    public:
       // IdleState(bool stateAction) : MachineState(stateAction){}
        static bool homing;
        void performStateAction() override;
        static void performHoming();
        static void calibratePressure();
        static void logDataStartStop();
};

class ManualState : public MachineState
{
    public:

        //static MACHINE_COMMAND actualCommand;
      //  volatile static CmdMessenger *cmdMessenger;
       // ManualState(bool stateAction) : MachineState(stateAction){}
        void performStateAction() override;
        static void rotateCallback();
        static void setObjectCallback();
};

class AutomaticState : public MachineState
{
    public:
    
    static int const intParameterNumber = 17;
    static int const boolParameterNumber = 5;
    static int const doubleParameterNumber = 2;
    static int const stringParameterNumber = 1;
    static unsigned int intParameters[intParameterNumber];
    static bool boolParameters[boolParameterNumber];
    static double doubleParameters[doubleParameterNumber];
    static char *stringParameters[stringParameterNumber];

    static HeavisideGenerator* targetSizeGenerator;
    static SpeedController* speedController;
    static unsigned int datalogTimerID;
    static File logFile;
    static char dataLogFileName[50];
    static double airVolume;
    static double deadVolume;
    static int zylinderRadius;
    static int currentLoop;

    //debug variables
    static int targetPressure_log;
    static double controlGain_log;
    static long targetSpeed_log;
    

    AutomaticState(){};
    void performStateAction() override;
    void static logData();
    static void fillingMode();
    static void applyConfiguration();
    static void startStopMeasurement();
    static void restartMeasurement();
};