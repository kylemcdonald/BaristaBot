#ifndef _MOTOR_THREAD
#define _MOTOR_THREAD

#include "ofMain.h"

class motorThread : public ofThread{

 public:

    ofArduino *ard;
    int i;
    int STEP_PIN;
    int STEPS;
    int DELAY;


    //--------------------------
    motorThread(){
        i = 0;
    }
    void waitLock(){
        // wait until ard is locked and ready
        while (!lock() && !ard->isArduinoReady()) {}
    }

    //--------------------------
    void setArduino(ofArduino &parentArd, int pin){
        *ard = parentArd;
        STEP_PIN = pin;
    }
    
    void aim(int stps, int dly) {
        STEPS = stps;
        DELAY = dly;
    }

    void start(){
        startThread(true, false);   // blocking, verbose
        i = 0;
    }

    void stop(){
        stopThread();
    }

    //--------------------------
    void threadedFunction(){
        while(isThreadRunning() != 0){
            moveStepper();
        }
    }


    //--------------------------------------------------------------
    void moveStepper(){      
        waitLock();
            ard->sendDigital(STEP_PIN, ARD_HIGH);
            usleep(DELAY);
            ard->sendDigital(STEP_PIN, ARD_LOW);
            usleep(DELAY);
            i++;
        unlock();
        
        if (i == STEPS) {
            stop();
        }
    }
};

#endif
