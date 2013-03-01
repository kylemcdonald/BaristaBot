#ifndef _MOTOR_THREAD
#define _MOTOR_THREAD

#include "ofMain.h"

class motorThread : public ofThread{

 public:

    ofArduino *ard;
    string name;
    int i;
    int STEP_PIN;
    int STEPS;
    int DELAY;


    //--------------------------
    motorThread(){
        i = 0;
    }

    //--------------------------
    void setArduino(ofArduino &parentArd, int pin, string nom){
        ard = &parentArd;
        STEP_PIN = pin;
        name = nom;
    }
    
    void aim(int stps, int dly) {
        STEPS = stps;
        DELAY = dly;
    }

    void start(){
        startThread(true, false);   // blocking, verbose
    }

    void stop(){
        stopThread();
        i = 0;
        STEPS = 0;
        DELAY = 1000;
    }

    //--------------------------
    void threadedFunction(){
        while(isThreadRunning() != 0){
            if (lock()){
                moveStepper();
                unlock();
            }
        }
    }


    //--------------------------------------------------------------
    void moveStepper(){      
        ard->sendDigital(STEP_PIN, ARD_HIGH);
        usleep(DELAY);
        ard->sendDigital(STEP_PIN, ARD_LOW);
        usleep(DELAY);
        
        if (++i == STEPS) {
            stop();
        }
    }
    
    //--------------------------------------------------------------
    void draw(){
        
        string str = name + " on pin " + ofToString(STEP_PIN) + ": i = " + ofToString(i);
        
        ofDrawBitmapString(str, 50, 720+STEP_PIN*7);
    }
};

#endif
