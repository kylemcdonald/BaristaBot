#ifndef _MOTOR_THREAD
#define _MOTOR_THREAD

#include "ofMain.h"

class motorThread : public ofThread{

 public:

    ofArduino *ard;
    string name;
    int i;
    int STEP_PIN;
    int DIR_PIN;
    int SLEEP_PIN;
    int STEPS;
    int DELAY;


    //--------------------------
    motorThread(){
        i = 0;
    }

    //--------------------------
    void setArduino(ofArduino &parentArd, int step_p, int dir_p, int sleep_p, string nom) {
        ard = &parentArd;
        STEP_PIN = step_p;
        DIR_PIN = dir_p;
        SLEEP_PIN = sleep_p;
        name = nom;
    }
    
    void aim(int stps, int dly) {
        STEPS = abs(stps);
        DELAY = dly;
//        bool dir = (STEPS > 0) ? ARD_HIGH : ARD_LOW;
//        while(!lock()){
//            ard->sendDigitalPinMode(SLEEP_PIN, ARD_HIGH);
//            ard->sendDigitalPinMode(DIR_PIN, ARD_HIGH);
//            unlock();
//        }
    }

    void start(){
        startThread(true, false);   // blocking, verbose
    }

    void stop(){
        stopThread();
        i = 0;
        STEPS = 0;
        DELAY = 1000;
        
        while(!lock()){
            ard->sendDigitalPinMode(SLEEP_PIN, ARD_LOW);
        }

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
//        i++;
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
