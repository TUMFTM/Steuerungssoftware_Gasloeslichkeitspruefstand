#pragma once

class TargetSizeGenerator {
    
    public:
        virtual int getTargetSize(unsigned long currentTime) = 0;
        virtual int getStirrerSpeed(unsigned long currentTime) = 0;
        virtual double getControlGain(unsigned long) = 0;
        virtual ~TargetSizeGenerator(){};
    private:
};

class HeavisideGenerator : public TargetSizeGenerator {
        //attributes for parametrization of the desired function
        public:
        unsigned long timepoints[5]; //in seconds
        int DiscretePressureLevels[5]; //in 0.1 bar. d.h. "500" equals 50 bar
        bool StirrerAcitivity[5];

        int StirrerSpeed; //unit not important in this part of the code
        bool pauseStirrerWhileCompressing;
        bool pauseStirrerWhileExpanding;
        int CompressionStirringPause; //in seconds
        int ExpansionStirringPause; // in seconds
        double gainFactors[6] = {4,10,80,80,10,4};
        // Methoden
        HeavisideGenerator(
            // int t_temp,int t_precon, int t_test,int t_postcon, int t_expan,
            //                int p_temp,int p_precon, int p_test,int p_postcon, int p_expan,
            //                bool stirr_temp, bool stirr_precon, bool stirr_test, bool stirr_postcon, bool stirr_expan,
            //                bool pauseStirrDurComp, bool pauseStirrDurExp, int StirrPauseComp, int StirrPauseExp, int StirringSpeed
                            );
        int getTargetSize(unsigned long currentTime); //CurrentTime in milliseconds since start of measurement
        int getStirrerSpeed(unsigned long currentTime); //CurrentTime in milliseconds since start of measurement
        double getControlGain(unsigned long);
        ~HeavisideGenerator(){};
};