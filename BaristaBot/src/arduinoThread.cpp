#include "ofMain.h"
#include "arduinoThread.h"
#include "motorThread.h"


//--------------------------------------------------------------
void arduinoThread::start(){
    startThread(true, false);   // blocking, verbose
    curState = START;
}


//--------------------------------------------------------------
void arduinoThread::stop(){
    while(!lock()){
        ard.disconnect();
        unlock();
        stopThread();
    }
}

//--------------------------------------------------------------
void arduinoThread::setup(){
    initializeVariables();
    initializeArduino();
//    initializeMotors();

}

//--------------------------------------------------------------
void arduinoThread::initializeArduino() {
	ard.connect("/dev/tty.usbmodem1411", 57600);
	ofAddListener(ard.EInitialized, this, &arduinoThread::setupArduino);
	bSetupArduino = false;
}


//--------------------------------------------------------------
void arduinoThread::setupArduino(const int & version) {
    // remove listener because we don't need it anymore
    ofRemoveListener(ard.EInitialized, this, &arduinoThread::setupArduino);
    
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
    ofAddListener(ard.EDigitalPinChanged, this, &arduinoThread::digitalPinChanged);

    // it is now safe to send commands to the Arduino
    bSetupArduino = true;
}


//--------------------------------------------------------------
void arduinoThread::initializeMotors(){
    // pass arduino reference and pins to motor threads
//    X.setArduino(ard, X_STEP_PIN);
//    Z.setArduino(ard, Z_STEP_PIN);
//    Y.setArduino(ard, Y_STEP_PIN);
//    INK.setArduino(ard, INK_STEP_PIN);
    
    // set default pin directions towards limit switches
    ard.sendDigital(X_DIR_PIN, ARD_HIGH);
    ard.sendDigital(Z_DIR_PIN, ARD_HIGH);
    ard.sendDigital(Y_DIR_PIN, ARD_LOW);
    ard.sendDigital(INK_DIR_PIN, ARD_HIGH);
    
//    home();

}

//--------------------------------------------------------------
void arduinoThread::initializeVariables(){
    // set limit and signal defualts
    X_LIMIT  = Z_LIMIT  = Y_LIMIT  = INK_LIMIT  = false;
//    X_SIGNAL = Z_SIGNAL = Y_SIGNAL = INK_SIGNAL = ARD_LOW; // do not initialize ARD_HIGH

    // initialize positions
//    lastX = lastY = 0;
}


//--------------------------------------------------------------
void arduinoThread::home(){
    curState = HOMING;

    // set default pin directions towards limit switches
    ard.sendDigital(X_DIR_PIN, ARD_HIGH);
    ard.sendDigital(Z_DIR_PIN, ARD_HIGH);
    ard.sendDigital(Y_DIR_PIN, ARD_LOW);
    ard.sendDigital(INK_DIR_PIN, ARD_HIGH);
    
//    X.aim(10000, 500);
//    X.start();
    
//    Y.aim(10000, 500);
//    Y.start();
}

//--------------------------------------------------------------
void arduinoThread::update(){
    ard.update();
    
    
    
    
    

    

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
//void arduinoThread::setTarget() {
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
void arduinoThread::threadedFunction(){
    while(isThreadRunning() != 0){
        if (lock()){
            update();
            unlock();
        }
//        usleep(1000); // Mac only!!!
    }
}

//--------------------------
void arduinoThread::draw(){
   
    string str = "curState = " + ofToString(stateName[curState]) + ". test = " + ofToString(test);
    if (!bSetupArduino){
		str += "arduino not ready...";
	} else {
        str += "arduino connected, firmware: " + ofToString(ard.getFirmwareName());
    }

    ofDrawBitmapString(str, 50, 700);
}

//--------------------------------------------------------------
void arduinoThread::digitalPinChanged(const int & pinNum) {
//    cout << "SWITCH" << endl;
//    stopThread();
    if (pinNum == X_LIMIT_PIN) {
        X.stop();
    } else if (pinNum == Y_LIMIT_PIN) {
        Y.stop();
    }

}
