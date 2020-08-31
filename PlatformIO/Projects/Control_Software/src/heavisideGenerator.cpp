#include "targetSizeGenerator.h"
#include "machineState.h"

HeavisideGenerator::HeavisideGenerator(
// int t_temp,int t_precon, int t_test,int t_postcon, int t_expan,
// int p_temp,int p_precon, int p_test,int p_postcon, int p_expan,
// bool stirr_temp, bool stirr_precon, bool stirr_test, bool stirr_postcon, bool stirr_expan,
// bool pauseStirrDurComp, bool pauseStirrDurExp, int StirrPauseComp, int StirrPauseExp, int StirringSpeed
) 
{

//Notiz:
// Ich habe den Konstruktor mal so geschrieben dass die Namen der Parameter, die der Konstruktor
// als Eingangswerte benötigt nachvollziehbar waren.
// Insbesondere solltest du dadurch verstehen können welche Parameter in welches Arrayfeld der 
// HeavisideGenerator Klasse gehört.
// Für den Aufruf kannst du natürlich den Konstruktor so abändern, dass die von dir bisher erstellten Parameterarrays
// übergeben werden und hier im Konstruktor nur richtig zugewiesen.
// Ich war mir aber nicht 100% sicher welcher Parameter genau in welchem deiner Arrayfelder steht

// Hinweis:
// Ich habe leider weder nen Arduino noch eine IDE zur Hand, sodass ich den Code leider nicht
// kompilieren oder testen konnte....
// VS-Code zeigt mir zumindest aber keine Syntaxfehler an.
// Ich hoffe das passt alles so was ich geschrieben habe.

//Zuweisung der Parameter ins Array    

// -3 on all intParameters because commented out first 3 input fields on parametrization field on automatic mode. (quickfix)
DiscretePressureLevels[0]=AutomaticState::intParameters[1]; //pressure_temp
DiscretePressureLevels[1]=AutomaticState::intParameters[3]; //pressure_precond
DiscretePressureLevels[2]=AutomaticState::intParameters[5]; //pressure_test
DiscretePressureLevels[3]=AutomaticState::intParameters[7]; //pressure_postcond
DiscretePressureLevels[4]=AutomaticState::intParameters[9]; //pressure_expand
//Zuweisung der Rührstati ins Array
StirrerAcitivity[0] = AutomaticState::boolParameters[0];    //stirr_temp;
StirrerAcitivity[1] = AutomaticState::boolParameters[1];    //stirr_precon;
StirrerAcitivity[2] = AutomaticState::boolParameters[2];    //stirr_test;
StirrerAcitivity[3] = AutomaticState::boolParameters[3];    //stirr_postcon;
StirrerAcitivity[4] = AutomaticState::boolParameters[4];    //stirr_expan;

// Berechnung der Switch-Zeitpunkte
//Umrechnung von min in Sekunden beachten
timepoints[0]= AutomaticState::intParameters[0]*60L + MachineState::timeOffset; //t_temp*60
timepoints[1]=timepoints[0]+AutomaticState::intParameters[2]*60L; // t_precond*60 
timepoints[2]=timepoints[1]+AutomaticState::intParameters[4]*60L; // t_rest*60
timepoints[3]=timepoints[2]+AutomaticState::intParameters[6]*60L; //t_postcond*60
timepoints[4]=timepoints[3]+AutomaticState::intParameters[8]*60L; //t_expand*60
//Zuweisung der Restlichen Parameter
pauseStirrerWhileCompressing= StirrerAcitivity[0]; //pauseStirrDurComp; //Ich glaube hier stimmt die PArameterzuweisung nicht
pauseStirrerWhileExpanding= StirrerAcitivity[4]; //pauseStirrDurExp; //Ich glaube hier stimmt die PArameterzuweisung nicht
CompressionStirringPause=AutomaticState::intParameters[12]; //StirrPauseComp
ExpansionStirringPause=AutomaticState::intParameters[13]; //StirrPauseExp
StirrerSpeed=AutomaticState::intParameters[11];

}




int HeavisideGenerator::getTargetSize(unsigned long currentTime){
    // for(int i = 0; i < 5; i++ ){
    //     Serial.print("DRUCK ");
    //     Serial.print(i);
    //     Serial.print(":");
    //     Serial.println(DiscretePressureLevels[i]);
    // }    

   currentTime= currentTime/1000; //Change unit from milliseconds to seconds 
//    int i=1;
//    Serial.print("CURRENT TIME: ");
//    Serial.print(currentTime);
//    Serial.print(", ");
//    Serial.println(MachineState::timeOffset);
//    Serial.print("TIME POINTS: ");
//    Serial.print(timepoints[0]);
//    Serial.print(", ");
//    Serial.print(timepoints[1]);
//    Serial.print(", ");
//    Serial.print(timepoints[2]);
//    Serial.print(", ");
//    Serial.println(timepoints[3]);
    // Serial.print("TIMEPOINT 0,1,2,3: ");
    // Serial.print(timepoints[0]);
    // Serial.print(", ");
    // Serial.print(timepoints[1]);
    // Serial.print(", ");
    // Serial.print(timepoints[2]);
    // Serial.print(", ");
    // Serial.println(timepoints[3]);
    // Serial.print("currentTime: ");
    // Serial.println(currentTime);
   
   return (currentTime <= timepoints[0]) ? DiscretePressureLevels[0]: 
            (currentTime <= timepoints[1]) ? DiscretePressureLevels[1] : 
            (currentTime <= timepoints[2]) ? DiscretePressureLevels[2] :
            (currentTime <= timepoints[3]) ? DiscretePressureLevels[3] : DiscretePressureLevels[4];

    // while((currentTime >=timepoints[i]) && (i<4) ){ //Loop switching timepoints until the currentTime is smaller than the switching Point i
    //     Serial.print("STATUS measurement: ");
    //     Serial.println(i);
    //     i=i+1;
    // }
}

double HeavisideGenerator::getControlGain(unsigned long currentTime){

    currentTime /= 1000; //Change unit from milliseconds to seconds 
    
   return (currentTime <= timepoints[0]) ? AutomaticState::doubleParameters[0] * gainFactors[0] :
            (currentTime <= timepoints[1]) ? AutomaticState::doubleParameters[0] * gainFactors[1] : 
            (currentTime <= timepoints[2]) ? AutomaticState::doubleParameters[0] * gainFactors[2] :
            (currentTime <= timepoints[2] + 20) ? AutomaticState::doubleParameters[0] * gainFactors[3]:
            (currentTime <= timepoints[3]) ? AutomaticState::doubleParameters[0] * gainFactors[4]:
             AutomaticState::doubleParameters[0] * gainFactors[5];
}


int HeavisideGenerator::getStirrerSpeed(unsigned long currentTime){
    currentTime=currentTime/1000; //Change unit from milliseconds to seconds 

    for(int i = 0; i < 5; i++){
        if(currentTime < timepoints[i]){
            if(StirrerAcitivity[i] == true){
                if(i == 2 && currentTime <  timepoints[1] + CompressionStirringPause || 
                    i == 3 && currentTime < timepoints[2] + ExpansionStirringPause){
                        return 0;
                    }
                    return StirrerSpeed;
            }
            return 0;
        }
    }
    return 0; 
}