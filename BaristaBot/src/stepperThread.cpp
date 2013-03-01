#include "ofMain.h"
#include "stepperThread.h"


//--------------------------------------------------------------
void stepperThread::start(){
    startThread(false, false);   // non-blocking, verbose
}


//--------------------------------------------------------------
void stepperThread::stop(){
    stopThread();
    while (!ard.isArduinoReady()) {}
    ard.disconnect();
}


//--------------------------------------------------------------
void stepperThread::setup(){
    cout << "ST setup" << endl;
    while (!ard.isArduinoReady()) {}
    connectToArduino();
    
    // init positions
    lastX = lastY = 0;
    X_LIMIT = Z_LIMIT = Y_LIMIT = INK_LIMIT = false;
    Y_SIGNAL = ARD_LOW;
}


//--------------------------------------------------------------
void stepperThread::connectToArduino () {
	ard.connect("/dev/tty.usbmodem1411", 57600);
	ofAddListener(ard.EInitialized, this, &stepperThread::setupArduino);
	bSetupArduino = false;
}



//--------------------------------------------------------------
void stepperThread::setupArduino(const int & version) {
	
    cout << "in setupArduino" << endl;
	// remove listener because we don't need it anymore
	ofRemoveListener(ard.EInitialized, this, &stepperThread::setupArduino);
    
    // set digital outputs
    ard.sendDigitalPinMode(X_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(X_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Z_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Z_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Y_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Y_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(INK_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(INK_STEP_PIN, ARD_OUTPUT);
    
    // set digital inputs
    ard.sendDigitalPinMode(X_LIMIT_PIN, ARD_INPUT);
    ard.sendDigitalPinMode(Z_LIMIT_PIN, ARD_INPUT);
    ard.sendDigitalPinMode(Y_LIMIT_PIN, ARD_INPUT);
    ard.sendDigitalPinMode(INK_LIMIT_PIN, ARD_INPUT);
    ofAddListener(ard.EDigitalPinChanged, this, &stepperThread::digitalPinChanged);

    // it is now safe to send commands to the Arduino
    bSetupArduino = true;
    curState = IDLE;
    
    ard.sendDigital(Y_DIR_PIN, ARD_HIGH);
}


//--------------------------------------------------------------
void stepperThread::update(){
    if (ard.isArduinoReady()){
        ard.update();
        updateSteppers();
    }
    //    if (curState == PRINT) {
    ////        stepsX = stepsY = 1000;
    //        counter++;
    //        updateSteppers();
    //
    //        if (updateTarget) {
    //            setTarget();
    //            counter = 0;
    //        }
    //        if (counter < limit) {
    //            updateSteppers();
    //            counter++;
    //        } else {
    //            updateTarget = true;
    //        }
    //    }
}

//--------------------------------------------------------------
//void stepperThread::setTarget() {
//    if (curPath < paths.size()) {
//        // first point in drawing
//        if (curPath == 0 && curPoint == 0) {
//            pushInk = false;
//            points = paths.at(curPath).getVertices();
//            target = points.at(curPoint++);
//            // a new point to draw
//        } else if (curPoint < points.size()) {
//            pushInk = true;
//            target = points.at(curPoint++);
//            // switch to a new path
//        } else {
//            pushInk = false;
//            curPath++;
//            curPoint = 0;
//            points = paths.at(curPath).getVertices();
//            target = points.at(curPoint++);
//        }
//        // all paths are drawn
//    } else {
//        curState = COFFEE_PHOTO;
//        curPath = 0;
//        curPoint = 0;
//    }
//    
//    stepsX = abs((target.x - lastX) * 10);
//    stepsY = abs((target.y - lastY) * 10);
//    stepsInk = sqrt(stepsX*stepsX + stepsY*stepsY);
//    limit = stepsX*stepsY;
//    
//    // set the directions for each servo
//    int dir = (target.x > lastX) ? ARD_HIGH : ARD_LOW;
//    ard.sendDigital(X_DIR_PIN, dir);
//    dir = (target.y > lastY) ? ARD_HIGH : ARD_LOW;
//    ard.sendDigital(Y_DIR_PIN, dir);
//    ard.sendDigital(INK_DIR_PIN, ARD_LOW); // low pushes the syringe
//    
//    lastX = target.x;
//    lastY = target.y;
//    updateTarget = false;
//}


//--------------------------------------------------------------
void stepperThread::updateSteppers () {
    X_SIGNAL = Z_SIGNAL = INK_SIGNAL = ARD_LOW;
//    Y_SIGNAL = ARD_HIGH;
    
//    if (count % 100 == 0) {
//        INK_SIGNAL = ARD_HIGH;
//    }
//    
//    if (count % 10 == 0) {
//        X_SIGNAL = ARD_HIGH;
//    }
    
//    if (!Y_LIMIT) {
//        Y_SIGNAL = ARD_HIGH;
//    } 
    //    if (stepsX > stepsY) {
    //        X_SIGNAL = ARD_HIGH;
    //    } else {
    //        Y_SIGNAL = ARD_HIGH;
    //    }
    
    //
    //    if ((limit-counter) % stepsY == 0) {
    //        X_SIGNAL = ARD_HIGH;
    //    }
    //    if ((limit-counter) % stepsX == 0) {
    //        Y_SIGNAL = ARD_HIGH;
    //    }
    //    if (counter % stepsInk && pushInk) {
    //        INK_SIGNAL = ARD_HIGH;
    //    }
    
    ard.sendDigital(X_STEP_PIN, X_SIGNAL);
    ard.sendDigital(Z_STEP_PIN, Z_SIGNAL);
    ard.sendDigital(Y_STEP_PIN, Y_SIGNAL);
    ard.sendDigital(INK_STEP_PIN, INK_SIGNAL);
    usleep(MIN_PULSE);
    ard.sendDigital(X_STEP_PIN, ARD_LOW);
    ard.sendDigital(Z_STEP_PIN, ARD_LOW);
    ard.sendDigital(Y_STEP_PIN, ARD_LOW);
    ard.sendDigital(INK_STEP_PIN, ARD_LOW);
    usleep(MIN_PULSE);
}


//void stepperThread::sleepMicros (int microseconds) {
//    while (ofGetElapsedTimeMicros() % microseconds != 0) {}
//}


//--------------------------------------------------------------
void stepperThread::threadedFunction(){
    while(isThreadRunning() != 0){
        if(lock()){
            if(count++ > 50000) count = 0;
            update();
            unlock();
        }
    }
}


//--------------------------------------------------------------
void stepperThread::digitalPinChanged(const int & pinNum) {
    cout << "SWITCH" << endl;
//    stopThread();
    Y_SIGNAL = !Y_SIGNAL;

}
