
#include "ofMain.h"
#include "arduinoThread.h"
#include "motorThread.h"


//----------------------------------------------------------------------------------------------
void arduinoThread::start(){
    startThread(true, false);   // blocking, not-verbose
    curState = START;
}

void arduinoThread::stop(){
    stopThread();
    
    if (X.isThreadRunning()) X.stop();
    if (Y.isThreadRunning()) Y.stop();
    if (Z.isThreadRunning()) Z.stop();
    if (INK.isThreadRunning()) INK.stop();
    
    ard.disconnect();
}





//----------------------------------------------------------------------------------------------
void arduinoThread::setup(){
    // do not change this sequence
    initializeVariables();
    initializeMotors();
    initializeArduino();
}

void arduinoThread::initializeVariables(){
    X_LIMIT  = Z_LIMIT  = Y_LIMIT  = INK_LIMIT  = false;
    home.set(0,0);
    should_ink = false;
}

void arduinoThread::initializeMotors(){
    // pass arduino reference and pins to motor threads
    X.setArduino    (ard, X_STEP_PIN,   X_DIR_PIN,   X_SLEEP_PIN,   "Motor X");
    Y.setArduino    (ard, Y_STEP_PIN,   Y_DIR_PIN,   Y_SLEEP_PIN,   "Motor Y");
    Z.setArduino    (ard, Z_STEP_PIN,   Z_DIR_PIN,   Z_SLEEP_PIN,   "Motor Z");
    INK.setArduino  (ard, INK_STEP_PIN, INK_DIR_PIN, INK_SLEEP_PIN, "Motor I");
}

void arduinoThread::initializeArduino() {
    // StandardFirmata for OF is at 57600 by default
	ard.connect("/dev/tty.usbmodem1411", 115200);
	ofAddListener(ard.EInitialized, this, &arduinoThread::setupArduino);
	bSetupArduino = false;
}

void arduinoThread::setupArduino(const int & version) {
    // remove listener because we don't need it anymore
    ofRemoveListener(ard.EInitialized, this, &arduinoThread::setupArduino);
    
    // set digital outputs
    ard.sendDigitalPinMode(X_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Z_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Y_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(INK_DIR_PIN, ARD_OUTPUT);
    
    ard.sendDigitalPinMode(X_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Z_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Y_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(INK_STEP_PIN, ARD_OUTPUT);
    
    ard.sendDigitalPinMode(X_SLEEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Z_SLEEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Y_SLEEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(INK_SLEEP_PIN, ARD_OUTPUT);
    
    // set digital inputs
    ard.sendDigitalPinMode(X_LIMIT_PIN, ARD_INPUT);
    ard.sendDigitalPinMode(Z_LIMIT_PIN, ARD_INPUT);
    ard.sendDigitalPinMode(Y_LIMIT_PIN, ARD_INPUT);
    ard.sendDigitalPinMode(INK_LIMIT_PIN, ARD_INPUT);
    ofAddListener(ard.EDigitalPinChanged, this, &arduinoThread::digitalPinChanged);

    // it is now safe to send commands to the Arduino
    bSetupArduino = true;
    curState = IDLE;
}





//----------------------------------------------------------------------------------------------
void arduinoThread::goHome(){
//    if (curState == FACE_PHOTO) {
//        Z.ready(-10000, 397);
//        Z.start();
//    } else {
//        curState = HOMING;
//    }
    
    X.ready(10000, 857);
    Y.ready(-10000, 756);
    
    X.start();
    Y.start();
}

ofPoint arduinoThread::getNextTarget() {
    ofPoint next;
    
    // if there is another point in this path, well grab the fucker
    if (++points_i < points.size()) {
        next = points.at(points_i);
        if (points_i == 1) should_ink = true;
    }
    // if not, get the next path then, and stop drawing, dickbag
    else if (++paths_i < paths.size()) {
        points = paths.at(paths_i).getVertices();
        next = points.at(points_i = 0);
        should_ink = false;
    }
    // no more goddamned points or paths, I need a coffee
    else {
        points_i = paths_i = 0;
        should_ink = false;
        shootCoffee();
        return home;
    }
    
    return next;
}

void arduinoThread::journey(ofPoint orig, ofPoint dest){
    int sx = abs(steps_x = (dest.x - orig.x) * 5);
    int sy = abs(steps_y = (dest.y - orig.x) * 5);
    
    if (sx > sy) {
        delay_x = DELAY_MIN;
        if (sy != 0) {
            delay_y = (sx/sy) * DELAY_MIN;
        } else {
            delay_y = DELAY_MIN;
        }
    } else {
        delay_y = DELAY_MIN;
        if (sx != 0) {
            delay_x = (sy/sx) * DELAY_MIN;
        } else {
            delay_x = DELAY_MIN;
        }
    }
    
    X.ready(steps_x, delay_x);
    Y.ready(steps_y, delay_y);
    
    ofSleepMillis(10);
    X.start();
    Y.start();
}

bool arduinoThread::journeysDone(){
    if (X.isThreadRunning() || Y.isThreadRunning())
        return false;
    return true;
}





//----------------------------------------------------------------------------------------------
void arduinoThread::update(){
    lock();
        ard.update();
    unlock();

    
    switch (curState) {
        case FACE_PHOTO:
            if (journeysDone() && !Z.isThreadRunning()) {
                // we are starting from home, robot moves home after face photo
                current = home;
                target = *points.begin();
                paths_i = points_i = 0;

                curState = PRINTING;
            }
            break;
        case PRINTING:
            if (journeysDone()) {
                journey(current, target);
                if (should_ink) {
                    INK.ready(10000, 10000);
                    ofSleepMillis(10);
                    INK.start();
                } else {
                    INK.stop();
                    ofSleepMillis(10);
                }
                current = target;
                target = getNextTarget();
            }
            break;
        default:
            break;
    }
}




//----------------------------------------------------------------------------------------------
void arduinoThread::shootFace(){
    curState = SHOOT_FACE;
    // change these value depending on observation
//    Z.ready(10000, 500);
//    Z.start();
    
}

void arduinoThread::shootCoffee(){
    curState = SHOOT_COFFEE;
    // change these value depending on observation
    Z.ready(2000, 500);
    Z.start();
    
    
    // **** TAKE PHOTO ****
    // operator pushes button to accept it, 
    // that sends the machine up, ready for next face photo
}


void arduinoThread::jogRight() {
    X.ready(1000, DELAY_MIN);
    X.start();
}
void arduinoThread::jogLeft() {
    X.ready(-1000, DELAY_MIN);
    X.start();
}
void arduinoThread::jogForward() {
    Y.ready(1000, DELAY_MIN);
    Y.start();
}
void arduinoThread::jogBack() {
    Y.ready(-1000, DELAY_MIN);
    Y.start();
}
void arduinoThread::jogUp() {
    Z.ready(1000, DELAY_MIN);
    Z.start();
}
void arduinoThread::jogDown() {
    Z.ready(-1000, DELAY_MIN);
    Z.start();
}



//----------------------------------------------------------------------------------------------
void arduinoThread::threadedFunction(){
    while(isThreadRunning() != 0){
//        usleep(10000);
//        update();
    }
}

void arduinoThread::draw(){
   
    string str = "curState = " + ofToString(stateName[curState]) + ". ";
    if (!bSetupArduino){
		str += "arduino not ready...";
	} else {
        str += "Point " + ofToString(points_i) + " / " + ofToString(points.size());
        str += ". Path " + ofToString(paths_i) + " / " + ofToString(paths.size());
    }

    ofDrawBitmapString(str, 50, 700);
    
    X.draw();
    Y.draw();
    Z.draw();
    INK.draw();
}

void arduinoThread::digitalPinChanged(const int & pinNum) {
    // note: this will throw tons of false positives on a bare mega, needs resistors
    if (ard.getDigital(X_LIMIT_PIN)) {
        X.stop();
    } else if (ard.getDigital(Z_LIMIT_PIN)) {
        Z.stop();
    } else if (ard.getDigital(Y_LIMIT_PIN)) {
        Y.stop();
    } else if (ard.getDigital(INK_LIMIT_PIN)) {
        INK.stop();
    }
}






