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

    //--------------------------
    void setArduino(ofArduino &parentArd){
        *ard = parentArd;
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
            if(lock()){
                    moveStepper(STEP_PIN, STEPS, DELAY);
                unlock();
            }
        }
    }


    //--------------------------------------------------------------
    void moveStepper(int num, int steps, int delay){      
        while(i < steps){
            if (lock()) {
                if (ard->isArduinoReady()) {
                    ard->sendDigital(STEP_PIN, ARD_HIGH);
                    usleep(delay);
                    ard->sendDigital(STEP_PIN, ARD_LOW);
                    usleep(delay);
                    i++;
                }
                unlock();
            }
        }
        stop();
    }




};

#endif
