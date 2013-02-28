#include "ofMain.h"
#include "stepperThread.h"


void stepperThread::start(){
    startThread(true, false);   // blocking, verbose
}

void stepperThread::stop(){
    stopThread();
}

void stepperThread::threadedFunction(){
    while( isThreadRunning() != 0 ){
        if( lock() ){
            count++;
            if(count > 50000) count = 0;
            unlock();
            //					ofSleepMillis(1 * 1000);
        }
    }
}

void stepperThread::draw(){

    string str = "I am a slowly increasing thread. \nmy current count is: ";

    if( lock() ){
        str += ofToString(count);
        unlock();
    }else{
        str = "can't lock!\neither an error\nor the thread has stopped";
    }
    ofDrawBitmapString(str, 50, 56);
}
