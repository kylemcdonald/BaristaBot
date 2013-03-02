#ifndef _MOTOR_THREAD
#define _MOTOR_THREAD

#include "ofMain.h"

class motorThread : public ofThread{

 public:

    ofArduino *ard;
    string name;

    int STEP_PIN, DIR_PIN, SLEEP_PIN;
    int STEPS, DELAY, INC;
    bool DIR, GO;


    //--------------------------------------------------------------
    motorThread() {}

    void setArduino(ofArduino &parentArd, int step_p, int dir_p, int sleep_p, string nom) {
        ard = &parentArd;
        STEP_PIN = step_p;
        DIR_PIN = dir_p;
        SLEEP_PIN = sleep_p;
        name = nom;
    }
    
    void clear() {
        DIR = GO = ARD_LOW;
        STEPS = INC = 0;
        DELAY = 1000;
    }
    
    void ready(int stps, int dly) {
        STEPS = abs(stps);
        DELAY = dly;
        DIR = (stps > 0) ? ARD_HIGH : ARD_LOW;
    }
    
    void aim(){
        lock();
            ard->sendDigital(SLEEP_PIN, ARD_HIGH);
            ard->sendDigital(DIR_PIN, DIR);
        unlock();
    }
    
    
    //--------------------------------------------------------------
    void start(){
        startThread(true, false);   // blocking, non-verbose
    }

    void stop(){
        stopThread();
        clear();
        
        lock();
            ard->sendDigital(SLEEP_PIN, ARD_LOW);
        unlock();
    }

    
    //--------------------------------------------------------------
    void threadedFunction(){
        while(isThreadRunning() != 0){
            lock();
            
                ard->sendDigital(STEP_PIN, GO = !GO);
                INC++;

            unlock();
            usleep(DELAY);

            if (INC == STEPS) stop();
        }
    }
    
    //--------------------------------------------------------------
    void draw(){
        string str = name + " on pin " + ofToString(STEP_PIN) + ": i = " + ofToString(INC);
        ofDrawBitmapString(str, 50, 800-STEP_PIN*7);
    }
};

#endif
