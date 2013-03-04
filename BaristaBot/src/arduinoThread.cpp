
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
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
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


void arduinoThread::journeyOn(bool new_coffee){
    point_count = 1;
    
    if (new_coffee) {
        start_transition = true;
        current = ofPoint(cropped_size, cropped_size);
        target = *points.begin();
        paths_i = points_i = 0;
        fireEngines();
    }

    else {
        planJourney();
        
        // starting a transition
        if (start_transition){
            fireEngines();
            return;
        }
        // starting a new path
        else if (start_path){
            start_path = false;
            continuing_path = true;
        }
        // so if new path or continuing see if we should change target
        else if (continuing_path) {

        }
        // ending a path
        else {
            fireEngines();
            return;
        }

        int sx = abs(getSteps(current.x, target.x, true));
        int sy = abs(getSteps(current.y, target.y, false));
        
        // if we're already above tolerance, fire
        if (sx > TOL && sy > TOL) {
            fireEngines();
            return;
        }
        // if one dimension gets too big, fire
        else if (sx > TOL*2 || sy > TOL*2) {
            fireEngines();
            return;
        }
        
        // if you didn't fire and return out then you're hitting planJourney() again
        point_count++;
    }
}


void arduinoThread::planJourney(){
    // starting a new path
    if (points_i++ == 0) {
        // if this path has only one point it's a transition
        if (points.size() == 1) {
            // if it's the last path just skip it
            if (paths_i == paths.size()-1) {
                shootCoffee();
            } else {
                start_transition = true;
                points = paths.at(paths_i).getVertices();
                target = points.at(points_i = 0);
            }
        } else {
            start_path = true;
            start_transition = false;
            target = points.at(points_i);
        }
    }
    // continuing a path except for the last stage
    else if (points_i++ < points.size()-2) {
        continuing_path = true;
        target = points.at(points_i);
    }
    // ending a path
    else if (points_i++ < points.size()-1) {
        continuing_path = false;
        target = points.at(points_i);
    }
    // starting a transition
    else if (paths_i++ < paths.size()-1) {
        start_transition = true;
        points = paths.at(paths_i).getVertices();
        target = points.at(points_i = 0);
    }
    // finishing the print
    else {
        shootCoffee();
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
    
    // if nowhere to go, skip it
    if (sx == 0 && sy == 0) {
        journeyOn(false);
        return;
    }
    
    // get delays
    if (!start_transition) {
        if (sy > sx && sx != 0) {
            delay_x = DELAY_MIN * sy / sx;
        } else if (sx > sy && sy != 0) {
            delay_y = DELAY_MIN * sx / sy;
        }
    }
    
    // send variables to motors and start them
    // NOTE x is flipped here to test 
    X.ready(-sdelta_x, delay_x);
    Y.ready(sdelta_y, delay_y);
    X.start();
    Y.start();
    
    // debugging
    hex = "\nsdelta_x:   " + ofToString(sdelta_x)        + "\ndelay_x:    " + ofToString(delay_x);
       // +  "\ncurrent.x:  " + ofToString(current.x) + "\ntarget.x:   "   + ofToString(target.x);
    hwy = "\nsdelta_y:   " + ofToString(sdelta_y)        + "\ndelay_y:    " + ofToString(delay_y);
       // +  "\ncurrent.y:  " + ofToString(current.y) + "\ntarget.y:   "   + ofToString(target.y);

    // after the move, we are at the target, our new current position
    current = target;
}


int arduinoThread::getSteps(float here, float there, bool is_x) {
    // normalize: 0,0 is the upper left corner and 1,1 is lower right
    float ndelta = there/cropped_size - here/cropped_size;
    
    // then convert to mm, an 80 mm square
    float mmdelta = ndelta * 80;
    
    // then convert to steps
    // estimate 236.2 steps per mm in X
    // estimate 118.1 steps per mm in Y
    if (is_x) {
        ex = "\nhere.x:     " + ofToString(int(here/cropped_size*80*150))
           + "\nthere.x:    " + ofToString(int(there/cropped_size*80*150))+ hex;
        int sdelta = int(mmdelta * 200);
        return sdelta;
    } else {
        wy = "\nhere.y:     " + ofToString(int(here/cropped_size*80*118))
           + "\nthere.y:    " + ofToString(int(there/cropped_size*80*118)) + hwy;
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
//----------------------------------------------------------------------------------------------
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
//    Z.ready(2000, 500);
//    Z.start();
    
    
    // **** TAKE PHOTO ****
    // operator pushes button to accept it, 
    // that sends the machine up, ready for next face photo
}

void arduinoThread::goHome(){
    //    if (curState == FACE_PHOTO) {
    //        Z.ready(-10000, 397);
    //        Z.start();
    //    } else {
    //        curState = HOMING;
    //    }
    
    X.ready(-256, 500);
    Y.ready(-256, 500);
    
    X.start();
    Y.start();
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
    INK.ready(-1000, 50);
    INK.start();
}
void arduinoThread::plungerDown() {
    if (INK.isThreadRunning()) {
        INK.INC = 0;
        return;
    }
    INK.ready(2000, 350);
    INK.start();
}


//----------------------------------------------------------------------------------------------
void arduinoThread::threadedFunction(){
    while(isThreadRunning() != 0){

    }
}

void arduinoThread::draw(){
   
    string str = "curState = " + ofToString(stateName[curState]);
    if (!bSetupArduino){
		str += "\narduino not ready...";
	} else {
        str += "\nI need a fucking coffee.";
    }
    ofDrawBitmapString(str, 50, 660);
    
    str = "Path:       " + ofToString(paths_i) + "\n\nPoint:      " + ofToString(points_i) + "\npoint_count " + ofToString(point_count);
    ofDrawBitmapString(str, 50, 720);
    str = "/ " + ofToString(paths.size()) + "\n\n/ " + ofToString(points.size());
    ofDrawBitmapString(str, 220, 720);

    ofDrawBitmapString(ex, 50, 780);
    ofDrawBitmapString(wy, 220, 780);
    
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






