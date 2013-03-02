#ifndef _MOTOR_THREAD
#define _MOTOR_THREAD

#include "ofMain.h"

class motorThread : public ofThread{

 public:

    ofArduino *ard;
    string name;

    int STEP_PIN;
    int DIR_PIN;
    int SLEEP_PIN;
    
    int i = 0;
    bool DIR = ARD_LOW;
    int STEPS = 0;
    int DELAY = 1000;
    
    bool flame = ARD_HIGH;


    //--------------------------------------------------------------
    motorThread() {}

    void setArduino(ofArduino &parentArd, int step_p, int dir_p, int sleep_p, string nom) {
        ard = &parentArd;
        STEP_PIN = step_p;
        DIR_PIN = dir_p;
        SLEEP_PIN = sleep_p;
        name = nom;
    }
    
    void ready(int stps, int dly) {
        STEPS = abs(stps);
        DELAY = dly;
        DIR = (stps > 0) ? ARD_HIGH : ARD_LOW;
    }
    

    //--------------------------------------------------------------
    void aim(){
        ard->sendDigital(SLEEP_PIN, ARD_HIGH);
        ard->sendDigital(DIR_PIN, DIR);
    }
    
    void fire(){
        ard->sendDigital(STEP_PIN, ARD_HIGH);
        usleep(DELAY);
        ard->sendDigital(STEP_PIN, ARD_LOW);
        usleep(DELAY);
//        i++;
    }
    
    
    //--------------------------------------------------------------
    void start(){
        startThread(true, false);   // blocking, non-verbose
    }

    bool stop(){
        stopThread();
        i = 0;
        DIR = ARD_LOW;
        STEPS = 0;
        DELAY = 1000;
        
        if (lock()) {
            ard->sendDigital(SLEEP_PIN, ARD_LOW);
            unlock();
            return true;
        } else {
            return false;
        }
    }

    
    //--------------------------------------------------------------
    void threadedFunction(){
        while(isThreadRunning() != 0){
            lock();
            //                ard->sendDigital(STEP_PIN, flame = !flame);
//                unlock();
//                usleep(DELAY);
                fire();
            unlock();
//            if (i == STEPS) while (!stop());
        }
    }
    
    //--------------------------------------------------------------
    void draw(){
        string str = name + " on pin " + ofToString(STEP_PIN) + ": i = " + ofToString(i);
        ofDrawBitmapString(str, 50, 800-STEP_PIN*7);
    }
};

#endif
