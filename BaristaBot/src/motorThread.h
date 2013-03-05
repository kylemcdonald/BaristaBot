
#ifndef _MOTOR_THREAD
#define _MOTOR_THREAD

#include "ofMain.h"

class motorThread : public ofThread{

 public:

    ofArduino *ard;
    string name;

    int STEP_PIN, DIR_PIN, SLEEP_PIN;
    int s, STEPS, DELAY, INC;
    bool DIR, GO, be_still;


    //--------------------------------------------------------------
    motorThread() {}

    void setArduino(ofArduino &parentArd, int step_p, int dir_p, int sleep_p, string nom) {
        ard = &parentArd;
        STEP_PIN = step_p;
        DIR_PIN = dir_p;
        SLEEP_PIN = sleep_p;
        name = nom;
        be_still = false;
    }
    
    void ready(int stps, int dly) {
        STEPS = abs(s = stps);
        INC = 0;
        DELAY = dly;
        DIR = (stps > 0) ? ARD_LOW : ARD_HIGH;
        lock();
            ard->sendDigital(SLEEP_PIN, ARD_HIGH);
            ard->sendDigital(DIR_PIN, DIR);
        unlock();
//        usleep(1000);
    }
    
    void clear() {
        DIR = GO = ARD_LOW;
        s = STEPS = INC = 0;
        DELAY = 1000;
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

    void freeze() {
        stop();
        be_still = true;
    }
    
    //--------------------------------------------------------------
    void threadedFunction(){
        while(isThreadRunning() != 0 && !be_still){
            if (++INC > STEPS) stop();

            lock();
            
                ard->sendDigital(STEP_PIN, GO = !GO);

            unlock();
            usleep(DELAY);
        }
    }
    
    //--------------------------------------------------------------
    void draw(){
        string str = name + ":  Step " + ofToString(INC);
        ofDrawBitmapString(str, 50, 1000-STEP_PIN*7);
        str = " / " + ofToString(STEPS);
        ofDrawBitmapString(str, 220, 1000-STEP_PIN*7);
    }
};




#endif




