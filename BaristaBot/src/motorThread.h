#ifndef _THREADED_OBJECT
#define _THREADED_OBJECT

#include "ofMain.h"

// this is not a very exciting example yet
// but ofThread provides the basis for ofNetwork and other
// operations that require threading.
//
// please be careful - threading problems are notoriously hard
// to debug and working with threads can be quite difficult


class threadedObject : public ofThread{

 public:

    ofArduino *ard;
    int i;
    int STEP_PIN;
    int STEPS;
    int DELAY;


    //--------------------------
    threadedObject(ofArduino &parentArd){
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
