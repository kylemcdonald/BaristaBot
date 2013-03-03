
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

ofPoint arduinoThread::getNextTarget() {
    ofPoint next;
    
    if (points_i == 0) start_path = true;
    
    // if there is another point in this path, well grab the fucker
    if (++points_i < points.size()) {
        next = points.at(points_i);
    }
    // if not, get the next path then, and stop drawing, dickbag
    else if (++paths_i < paths.size()) {
        start_transition = true;
        points = paths.at(paths_i).getVertices();
        next = points.at(points_i = 0);
    }
    // no more goddamned points or paths, I need a coffee
    else {
        print_done = true;
        points_i = paths_i = 0;
        shootCoffee();
        next = home;
    }
    
    return next;
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

void arduinoThread::journeyOn(bool new_coffee){
    int sdelta_x = 0;
    int sdelta_y = 0;
    int sx = 0;
    int sy = 0;
    int point_count = 0;
    int delay_x = DELAY_MIN;
    int delay_y = DELAY_MIN;
    ofPoint current;
    ofPoint target;
    
    if (start_transition) {
        if (new_coffee){
            paths_i = points_i = 0;
            current = home;
            target = *points.begin();
        }
        sx = abs(sdelta_x = getSteps(current.x, target.x, true));
        sy = abs(sdelta_y = getSteps(current.y, target.y, false));
        delay_x += 77; // so they aren't running at same speed
    }
    else {
        // if either movement is smaller than the tolerance of the robot
        // get more points and add them up until tolerance is passed
        while (sx < TOL || sy < TOL) {
            // keep going unless one dimension gets too big
            if (sx > TOL*2 || sy > TOL*2) {
                break;
            }
            current = target;
            target = getNextTarget();
            
            sx = abs(sdelta_x += getSteps(current.x, target.x, true));
            sy = abs(sdelta_y += getSteps(current.y, target.y, false));
            point_count++;
        }
        // now find the delays based on ratio of steps x and y
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
//                current = home;
//                target = *points.begin();
//                paths_i = points_i = 0;
                journeyOn(true);

                curState = PRINTING;
            }
            break;
        case PRINTING:
            if (journeysDone()) {
//                if (start_ink) {
//                    INK.ready(1000, 100000);
//                    INK.start();
//                    start_ink = false;
//                }
//                if (stop_ink) {
//                    INK.stop();
//                    stop_ink = false;
//                }
                
                journeyOn(false);
//                current = target;
//                target = getNextTarget();
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






