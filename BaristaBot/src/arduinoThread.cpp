
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
    home.set(0,0);
    start_path = false;
    start_transition = true;
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
    
    X.ready(1000, 857);
    Y.ready(-1000, 756);
    
    X.start();
    Y.start();
}


void arduinoThread::planJourney(){
    
    // starting a new path
    if (points_i == 0) {
        start_path = true;
        current = *points.begin();
        target = points.at(++points_i);
    }
    // continuing a path except for the last stage
    else if (points_i+1 < points.size()-1) {
        continuing_path = true;
        target = points.at(++points_i);
    }
    // ending a path
    else if (points_i+1 == points.size()-1) {
        continuing_path = false;
        target = points.at(++points_i);
    }
    // starting a transition
    else if (paths_i < paths.size()-1) {
        start_transition = true;
        current = *points.end();
        points = paths.at(++paths_i).getVertices();
        target = points.at(points_i = 0);
    }
    // finishing the print
    else {
        shootCoffee();
        return;
    }
}

void arduinoThread::fireEngines(){
    int sdelta_x = 0;
    int sdelta_y = 0;
    int delay_x = DELAY_MIN;
    int delay_y = DELAY_MIN;
    
    // get steps
    int sx = abs(sdelta_x = getSteps(current.x, target.x, true));
    int sy = abs(sdelta_y = getSteps(current.y, target.y, false));
    
    // get delays
    if (!start_transition) {
        if (sy > sx && sx != 0) {
            delay_x = DELAY_MIN * sy / sx;
        } else if (sx > sy && sy != 0) {
            delay_y = DELAY_MIN * sx / sy;
        }
    }
    
    // debugging
    ex = " current.x: "  + ofToString(current.x) + " target.x: " + ofToString(target.x)
       + " sx: "         + ofToString(sx)        + " delay_x: "  + ofToString(delay_x);
    wy = " current.y: "  + ofToString(current.y) + " target.y: " + ofToString(target.y)
       + " sy: "         + ofToString(sy)        + " delay_y: "  + ofToString(delay_y)
       + " point_count " + ofToString(point_count);
    
    // send variables to motors and start them
    X.ready(sdelta_x, delay_x);
    Y.ready(sdelta_y, delay_y);
    X.start();
    Y.start();
}

void arduinoThread::journeyOn(bool new_coffee){
    point_count = 1;
    
    if (new_coffee) {
        start_transition = true;
        current = home;
        target = *points.begin();
        paths_i = points_i = 0;
    } else {
        planJourney();
        
        // starting a new path
        if (start_path){
            INK.ready(10000, 1000);
            INK.start();
            start_path = false;
            continuing_path = true;
        }
        // ending a path
        else if (!continuing_path) {
            fireEngines();
            return;
        }
        // starting a transition
        else if (start_transition){
            INK.stop();
            fireEngines();
            return;
        }
        
        // so if new path or continuing see if we should change target
        while (continuing_path) {
            int sx = abs(getSteps(current.x, target.x, true));
            int sy = abs(getSteps(current.y, target.y, false));
            
            // if we're alread above tolerance, break
            if (sx > TOL && sy > TOL) break;
            // if one dimension gets too big, break
            if (sx > TOL*2 || sy > TOL*2) break;
            
            planJourney();
            point_count++;
        }
    }
    fireEngines();
}

int arduinoThread::getSteps(float here, float there, bool is_x) {
    // first normalize
    // FYI 0,0 is the upper left corner, 1,1 is lower right
    float ndelta = (there - here) / cropped_size;
    
    // then convert to mm, an 80 mm square
    float mmdelta = ndelta * 80;
    
    // then convert to steps
    // estimate 236.2 steps per mm in X
    // estimate 118.1 steps per mm in Y
    if (is_x) {
        int sdelta = int(mmdelta * 236);
        return sdelta;
    } else {
        int sdelta = int(mmdelta * 118);
        return sdelta;
    }
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
                journeyOn(true);
                curState = PRINTING;
            }
            break;
        case PRINTING:
            if (journeysDone()) {
                journeyOn(false);
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
    Z.start();
    
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
    if (X.isThreadRunning()) {
        X.INC = 0;
        return;
    }
    X.ready(1000, DELAY_MIN);
    X.start();
}
void arduinoThread::jogLeft() {
    if (X.isThreadRunning()) {
        X.INC = 0;
        return;
    }
    X.ready(-1000, DELAY_MIN);
    X.start();
}
void arduinoThread::jogForward() {
    if (Y.isThreadRunning()) {
        Y.INC = 0;
        return;
    }
    Y.ready(1000, DELAY_MIN);
    Y.start();
}
void arduinoThread::jogBack() {
    if (Y.isThreadRunning()) {
        Y.INC = 0;
        return;
    }
    Y.ready(-1000, DELAY_MIN);
    Y.start();
}
void arduinoThread::jogUp() {
    if (Z.isThreadRunning()) {
        Z.INC = 0;
        return;
    }
    Z.ready(1000, DELAY_MIN);
    Z.start();
}
void arduinoThread::jogDown() {
    if (Z.isThreadRunning()) {
        Z.INC = 0;
        return;
    }
    Z.ready(-1000, DELAY_MIN);
    Z.start();
}
void arduinoThread::plungerUp() {
    if (INK.isThreadRunning()) {
        INK.INC = 0;
        return;
    }
    INK.ready(200, DELAY_MIN);
    INK.start();
}
void arduinoThread::plungerDown() {
    if (INK.isThreadRunning()) {
        INK.INC = 0;
        return;
    }
    INK.ready(-200, DELAY_MIN);
    INK.start();
}


//----------------------------------------------------------------------------------------------
void arduinoThread::threadedFunction(){
    while(isThreadRunning() != 0){

    }
}

void arduinoThread::draw(){
   
    string str = "curState = " + ofToString(stateName[curState]) + ". ";
    if (!bSetupArduino){
		str += "arduino not ready...";
	} else {
        str += "Point " + ofToString(points_i) + " / " + ofToString(points.size());
        str += ". Path " + ofToString(paths_i) + " / " + ofToString(paths.size());
        str += "\n" + ex;
        str += "\n" + wy;
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






