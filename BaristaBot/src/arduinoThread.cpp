
#include "ofMain.h"
#include "arduinoThread.h"


//----------------------------------------------------------------------------------------------
void arduinoThread::start(){
    startThread(true, false);   // blocking, not-verbose
    curState = START;
}

void arduinoThread::stop(){
    stopThread();
    ard.sendDigital(X_SLEEP_PIN, ARD_LOW);
    ard.sendDigital(Y_SLEEP_PIN, ARD_LOW);
    ard.sendDigital(Z_SLEEP_PIN, ARD_LOW);
    ard.sendDigital(INK_SLEEP_PIN, ARD_LOW);
    
    ard.sendReset();
    ofSleepMillis(10);
    ard.disconnect();
}

//----------------------------------------------------------------------------------------------
void arduinoThread::setup(){
    // do not change this sequence
    initializeVariables();
    initializeArduino();
}

void arduinoThread::initializeVariables(){
    start_path = false;
    start_transition = true;
    
    x_timer = y_timer = z_timer = i_timer = ofGetElapsedTimeMicros();
    x_steps = y_steps = z_steps = i_steps = x_inc = y_inc = z_inc = i_inc = 0;
    x_go = y_go = z_go = i_go = ARD_HIGH;
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
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
void arduinoThread::update(){
    ard.update();
    
    switch (curState) {
        case JOG:
            sleepDoneMotors();
            if (allDone()) curState = IDLE;
            break;
        // arm has raised and is ready to take a photo
        case SHOOT_FACE:
            sleepDoneMotors();
            if (allDone()) curState = IDLE;
            break;
        // photo taken, arm is going to the limit switches: home
        case FACE_PHOTO:
            goHome();
            break;
        case HOMING: 
            break;
        // X, Z, and Y have hit limits
        case HOME:
            // start X, Y, and I
            ard.sendDigital(X_SLEEP_PIN, ARD_HIGH);
            ard.sendDigital(Y_SLEEP_PIN, ARD_HIGH);
            ard.sendDigital(INK_SLEEP_PIN, ARD_HIGH);
            // sleep Z
            ard.sendDigital(Z_SLEEP_PIN, ARD_LOW);
            ofSleepMillis(10);
            journeyOn(true);
            curState = PREPRINT;
            point_count = 1;
            break;
        // this is when arm moves from home to first point
        case PREPRINT:
            if (journeysDone()) {
                curState = PRINTING;
            }
            break;
        // print has started from first point
        case PRINTING:
            if (journeysDone()) {
                journeyOn(false);
            }
            break;
        // print is finished and arm is raising up
        case DONE:
            break;
        case ERROR:
            x_steps = y_steps = z_steps = i_steps = x_inc = y_inc = z_inc = i_inc = 0;
            ard.sendDigital(X_SLEEP_PIN, ARD_LOW);
            ard.sendDigital(Y_SLEEP_PIN, ARD_LOW);
            ard.sendDigital(Z_SLEEP_PIN, ARD_LOW);
            ard.sendDigital(INK_SLEEP_PIN, ARD_LOW);
            break;
        case RESET:
            if (journeysDone()){
                curState = IDLE;
                ard.sendDigital(X_SLEEP_PIN, ARD_LOW);
                ard.sendDigital(Y_SLEEP_PIN, ARD_LOW);
                ard.sendDigital(Z_SLEEP_PIN, ARD_LOW);
                x_inc = y_inc = z_inc = x_steps = y_steps = z_steps = 0;
            }
        default:
            break;
    }
}


void arduinoThread::journeyOn(bool new_coffee){
    
    if (new_coffee) {
        start_transition = true;
        current = ofPoint(home_x, home_y);
        target = *points.begin();
        paths_i = points_i = 0;
        fireEngines();
    }
    else {
        planJourney();
        
        // starting a transition
        if (start_transition){
            start_transition = false;
            stopInk();
            fireEngines();
        }
        // starting a new poy, ink flows
        else if (start_path){
            start_path = false;
            startInk();
            fireEngines();
        }
        // drawing last segment in a polyline
        else if (end_path) {
            end_path = false;
            fireEngines();
        }
        // check to see if movements are long enough, if not add a point
        else {
            int sx = abs(getSteps(current.x, target.x, true));
            int sy = abs(getSteps(current.y, target.y, false));
            
            // we've hit the last point
            if (paths_i >= paths.size() && points_i >= points.size()) {
                fireEngines();
            }
            
            // if we're already above tolerance, fire
            if (sx > TOL && sy > TOL) {
                fireEngines();
            }
            // if one dimension gets too big, fire
            else if (sx > TOL*4 || sy > TOL*4) {
                fireEngines();
            }
            else {
                // if you didn't fire and return out then you're hitting planJourney() again
                point_count++;
                update();
            }
        }
    }
}

// increments through path and sets the target point, the next x and y values to move to
void arduinoThread::planJourney(){

    // if the current polyline has just one point, it's a transition
    if (points.size() == 1) {
        // if it's the last polyline just skip it
        if (paths_i == paths.size()-1) {
            stopInk();
            shootCoffee();
        }
        // if it's not the last line move to the point and get the next polyline
        else {
            start_transition = true;
            paths_i++;
            points = paths.at(paths_i).getVertices();
            points_i = 0;
            target = points.at(points_i);
        }
    }
    // starting a new path, points_i is where we are currently, where the machine is
    else if (points_i == 0) {
        start_path = true;
        start_transition = false;
        points_i++;
        target = points.at(points_i);
    }
    // continuing a path except for the last stage
    else if (points_i < points.size()-2) {
        points_i++;
        target = points.at(points_i);
    }
    // ending a path
    else if (points_i < points.size()-1) {
        end_path = true;
        points_i++;
        target = points.at(points_i);
    }
    // starting a transition
    else if (paths_i < paths.size()-1) {
        start_transition = true;
        paths_i++;
        points_i = 0;
        points = paths.at(paths_i).getVertices();
        target = points.at(points_i);
    }
    // finishing the print
    else {
        stopInk();
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
        return;
    }
    
    // get delays
    if (sy > sx && sx != 0) {
        delay_x = DELAY_MIN * sy / sx;
    } else if (sx > sy && sy != 0) {
        delay_y = DELAY_MIN * sx / sy;
    }
    
    // toggle direction pins and enable motors
    bool DIR = (sdelta_x > 0) ? ARD_LOW : ARD_HIGH;
    ard.sendDigital(X_DIR_PIN, DIR);
    DIR = (sdelta_y > 0) ? ARD_LOW : ARD_HIGH;
    ard.sendDigital(Y_DIR_PIN, DIR);
    
    // send variables to motors and start them
    x_steps = sx;
    x_inc = 0;
    x_delay = delay_x;
    y_steps = sy;
    y_inc = 0;
    y_delay = delay_y;

    // debugging
    hex = "\nsdelta_x:   " + ofToString(sdelta_x) + "\ndelay_x:    " + ofToString(delay_x);
    hwy = "\nsdelta_y:   " + ofToString(sdelta_y) + "\ndelay_y:    " + ofToString(delay_y)
        + "\npoint_count " + ofToString(point_count);

    // after the move, we are at the target, our new current position
    current = target;
    point_count = 1;
}


int arduinoThread::getSteps(float here, float there, bool is_x) {
    // normalize: 0,0 is the upper left corner and 1,1 is lower right
    float ndelta = there/cropped_size - here/cropped_size;
    
    // then convert to mm, an 80 mm square
    float mmdelta = ndelta * 80;
    
    // then convert to steps (NOTE reversing X)
    if (is_x) {
        ex = "\nhere.x:     " + ofToString(int(here/cropped_size*80*SCALE_X))
           + "\nthere.x:    " + ofToString(int(there/cropped_size*80*SCALE_X)) + hex;
        int sdelta = -int(mmdelta * SCALE_X);
//        cout << "sdelta: " << sdelta << endl;
        return sdelta;
    } else {
        wy = "\nhere.y:     " + ofToString(int(here/cropped_size*80*SCALE_Y))
           + "\nthere.y:    " + ofToString(int(there/cropped_size*80*SCALE_Y)) + hwy;
        int sdelta = int(mmdelta * SCALE_Y);
//        cout << "sdelta: " << sdelta << endl;
        return sdelta;
    }
}

bool arduinoThread::journeysDone(){
    if (x_inc < x_steps || y_inc < y_steps) {
        return false;
    } else {
        return true;
    }
}
bool arduinoThread::allDone(){
    if (x_inc>=x_steps && y_inc>=y_steps && z_inc>=z_steps && i_inc>=i_steps) {
        return true;
    } else {
        return false;
    }
}






//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
void arduinoThread::startInk(){
    plungerDown();
    usleep(INK_WAIT);
    i_inc = 0;
    i_steps = 999999;
    i_delay = INK_DELAY;
}
void arduinoThread::stopInk(){
    INK_TRAVEL += i_inc;
    i_steps = i_inc = 0;
    usleep(10000);    // wait for ink to stop
    plungerUp();            // pull up to fast stop flow
//    usleep(INK_WAIT*2);    // wait for ink to stop
}
void arduinoThread::plungerUp() {
    ard.sendDigital(INK_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(INK_DIR_PIN, ARD_HIGH);
    i_steps = INK_STOP_STEPS;
    i_inc = 0;
    i_delay = INK_STOP_DELAY;
    INK_TRAVEL -= INK_STOP_STEPS;
}
void arduinoThread::plungerDown() {
    ard.sendDigital(INK_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(INK_DIR_PIN, ARD_LOW);
    i_steps = INK_START_STEPS;
    i_inc = 0;
    i_delay = INK_START_DELAY;
    INK_TRAVEL += INK_START_STEPS;
}





//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
void arduinoThread::shootCoffee(){
    // let X and Y and Ink sleep
    ard.sendDigital(X_SLEEP_PIN, ARD_LOW);
    ard.sendDigital(Y_SLEEP_PIN, ARD_LOW);
    ard.sendDigital(Z_SLEEP_PIN, ARD_LOW);
    ard.sendDigital(INK_SLEEP_PIN, ARD_LOW);
    
    shootFace();
}

void arduinoThread::shootFace(){
    x_steps = x_inc = y_steps = y_inc = z_steps = z_inc = 0;

    if (curState == PRINTING) {
        // raise the syringe in prep for change out
        ard.sendDigital(INK_SLEEP_PIN, ARD_HIGH);
        ard.sendDigital(INK_DIR_PIN, ARD_HIGH);
        i_steps = INK_TRAVEL;
        i_inc = 0;
        i_delay = DELAY_FAST;
        ofSleepMillis(20);
    }
    
    curState = SHOOT_FACE;
    
    // Raise the Z stage
    ard.sendDigital(Z_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(Z_DIR_PIN, ARD_LOW);
    z_steps = z_height;
    z_inc = 0;
    z_delay = DELAY_FAST;
    
    ofSleepMillis(2000);
    
//    raiseY();
}

void arduinoThread::raiseY(){
    // retract the Y stage
    ard.sendDigital(Y_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(Y_DIR_PIN, ARD_HIGH);
    y_steps = y_height;
    y_inc = 0;
    y_delay = DELAY_FAST;
}

void arduinoThread::goHome(){
    curState = HOMING;
    
    homeX();
    // others go home after pin change events below
}

void arduinoThread::reset(){
    // moves X, Y, and Z away one jog from the switches
    // does not move Syringe to avoid air bubbles
    curState = RESET;
    jogLeft();
    ofSleepMillis(20);
    jogUp();
    ofSleepMillis(20);
    jogForward();
//    ofSleepMillis(20);
//    plungerUp();
}

void arduinoThread::sleepDoneMotors(){
    if (x_inc >= x_steps) {
        ard.sendDigital(X_SLEEP_PIN, ARD_LOW);
        x_inc = x_steps = 0;
    }
    if (y_inc >= y_steps) {
        ard.sendDigital(Y_SLEEP_PIN, ARD_LOW);
        y_inc = y_steps = 0;
    }
    if (z_inc >= z_steps) {
        ard.sendDigital(Z_SLEEP_PIN, ARD_LOW);
        z_inc = z_steps = 0;
    }
    if (i_inc >= i_steps) {
        ard.sendDigital(INK_SLEEP_PIN, ARD_LOW);
        i_inc = i_steps = 0;
    }
}




//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
void arduinoThread::jogRight() {
    ard.sendDigital(X_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(X_DIR_PIN, ARD_LOW);
    x_steps = 1000;
    x_inc = 0;
    x_delay = DELAY_FAST;
}
void arduinoThread::jogLeft() {
    ard.sendDigital(X_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(X_DIR_PIN, ARD_HIGH);
    x_steps = 1000;
    x_inc = 0;
    x_delay = DELAY_FAST;
}
void arduinoThread::jogForward() {
    ard.sendDigital(Y_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(Y_DIR_PIN, ARD_LOW);
    y_steps = 1000;
    y_inc = 0;
    y_delay = DELAY_FAST;
}
void arduinoThread::jogBack() {
    ard.sendDigital(Y_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(Y_DIR_PIN, ARD_HIGH);
    y_steps = 1000;
    y_inc = 0;
    y_delay = DELAY_FAST;
}
void arduinoThread::jogUp() {
    ard.sendDigital(Z_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(Z_DIR_PIN, ARD_LOW);
    z_steps = 1000;
    z_inc = 0;
    z_delay = DELAY_FAST+200;
}
void arduinoThread::jogDown() {
    ard.sendDigital(Z_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(Z_DIR_PIN, ARD_HIGH);
    z_steps = 1000;
    z_inc = 0;
    z_delay = DELAY_FAST;
}
void arduinoThread::stopX () {
    x_steps = x_inc = 0;
    ard.sendDigital(X_SLEEP_PIN, ARD_LOW);
    ofSleepMillis(20);
}
void arduinoThread::stopY() {
    y_steps = y_inc = 0;
    ard.sendDigital(Y_SLEEP_PIN, ARD_LOW);
    ofSleepMillis(20);
}
void arduinoThread::stopZ(){
    z_steps = z_inc = 0;
    ard.sendDigital(Z_SLEEP_PIN, ARD_LOW);
    ofSleepMillis(20);
}
void arduinoThread::homeX(){
    ard.sendDigital(X_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(X_DIR_PIN, ARD_LOW);
    x_steps = 100000;
    x_inc = 0;
    x_delay = DELAY_FAST;
}
void arduinoThread::homeY(){
    ard.sendDigital(Y_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(Y_DIR_PIN, ARD_HIGH);
    y_steps = 100000;
    y_inc = 0;
    y_delay = DELAY_FAST;
}
void arduinoThread::homeZ(){
    ard.sendDigital(Z_SLEEP_PIN, ARD_HIGH);
    ard.sendDigital(Z_DIR_PIN, ARD_HIGH);
    z_steps = 100000;
    z_inc = 0;
    z_delay = DELAY_FAST;
}



//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
void arduinoThread::threadedFunction(){
    unsigned long long now = ofGetElapsedTimeMicros();
    
    while(isThreadRunning() != 0){
        usleep(1);

        // X axis
        if (x_steps > 0 && x_inc < x_steps) {
            now = ofGetElapsedTimeMicros();
            if (now - x_timer > HIGH_DELAY) {
                if (x_go) {
                    x_go = false;
                    x_timer = now;
                    ard.sendDigital(X_STEP_PIN, ARD_HIGH);
                    x_inc++;
                }
                else if (now - x_timer > x_delay) {
                    x_go = true;
                    x_timer = now;
                    ard.sendDigital(X_STEP_PIN, ARD_LOW);
                }
            }
        }
        // Y axis
        if (y_steps > 0 && y_inc < y_steps) {
            now = ofGetElapsedTimeMicros();
            if (now - y_timer > HIGH_DELAY) {
                if (y_go) {
                    y_go = false;
                    y_timer = now;
                    ard.sendDigital(Y_STEP_PIN, ARD_HIGH);
                    y_inc++;
                }
                else if (now - y_timer > y_delay) {
                    y_go = true;
                    y_timer = now;
                    ard.sendDigital(Y_STEP_PIN, ARD_LOW);
                }
            }
        }
        // Z axis
        if (z_steps > 0 && z_inc < z_steps) {
            now = ofGetElapsedTimeMicros();
            if (now - z_timer > HIGH_DELAY) {
                if (z_go) {
                    z_go = false;
                    z_timer = now;
                    ard.sendDigital(Z_STEP_PIN, ARD_HIGH);
                    z_inc++;
                }
                else if (now - z_timer > z_delay) {
                    z_go = true;
                    z_timer = now;
                    ard.sendDigital(Z_STEP_PIN, ARD_LOW);
                }
            }
        }
        // Syringe
        if (i_steps > 0 && i_inc < i_steps) {
            now = ofGetElapsedTimeMicros();
            if (now - i_timer > HIGH_DELAY) {
                if (i_go) {
                    i_go = false;
                    i_timer = now;
                    ard.sendDigital(INK_STEP_PIN, ARD_HIGH);
                    i_inc++;
                }
                else if (now - i_timer > i_delay) {
                    i_go = true;
                    i_timer = now;
                    ard.sendDigital(INK_STEP_PIN, ARD_LOW);
                }
            }
        }
    }
}

void arduinoThread::draw(){
   
    string str = "curState = " + ofToString(stateName[curState]);
    if (!bSetupArduino){
		str += "\narduino not ready...";
	} else {
        str += "\nI need a coffee.";
    }
    ofDrawBitmapString(str, 50, 660);
    
    str = "Path:       " + ofToString(paths_i) + "\n\nPoint:      "
        + ofToString(points_i);
    ofDrawBitmapString(str, 50, 720);
    str = "/ " + ofToString(paths.size()) + "\n\n/ " + ofToString(points.size());
    ofDrawBitmapString(str, 220, 720);

    ofDrawBitmapString(ex, 50, 780);
    ofDrawBitmapString(wy, 220, 780);
    
    str = ":  Step " + ofToString(x_inc);
    ofDrawBitmapString(str, 50, 1000-X_STEP_PIN*7);
    str = " / " + ofToString(x_steps);
    ofDrawBitmapString(str, 220, 1000-X_STEP_PIN*7);
    
    str = ":  Step " + ofToString(y_inc);
    ofDrawBitmapString(str, 50, 1000-Y_STEP_PIN*7);
    str = " / " + ofToString(y_steps);
    ofDrawBitmapString(str, 220, 1000-Y_STEP_PIN*7);
    
    str = ":  Step " + ofToString(z_inc);
    ofDrawBitmapString(str, 50, 1000-Z_STEP_PIN*7);
    str = " / " + ofToString(z_steps);
    ofDrawBitmapString(str, 220, 1000-Z_STEP_PIN*7);
    
    str = ":  Step " + ofToString(i_inc);
    ofDrawBitmapString(str, 50, 1000-INK_STEP_PIN*7);
    str = " / " + ofToString(i_steps);
    ofDrawBitmapString(str, 220, 1000-INK_STEP_PIN*7);
}


void arduinoThread::digitalPinChanged(const int & pinNum) {
    // note: this will throw tons of false positives on a bare mega, needs resistors
    if (curState == HOMING) {
        if (pinNum == X_LIMIT_PIN && ard.getDigital(X_LIMIT_PIN)) {
            stopX();
            homeY();
        }
        if (pinNum == Y_LIMIT_PIN && ard.getDigital(Y_LIMIT_PIN)) {
            stopY();
            homeZ();
        }
        if (pinNum == Z_LIMIT_PIN && ard.getDigital(Z_LIMIT_PIN)) {
            stopZ();
            curState = HOME;
        }
    }
    // if not homing or reseting and the switch is down, it's an error
    else if (curState != PREPRINT && curState != RESET && curState != HOME && curState != SHOOT_FACE) {
        if (ard.getDigital(pinNum) == ARD_HIGH){
            curState = ERROR;
        }
    }
}






