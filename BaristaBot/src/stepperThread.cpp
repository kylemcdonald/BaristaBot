#include "ofMain.h"
#include "stepperThread.h"


//--------------------------------------------------------------
void stepperThread::start(){
    startThread(true, false);   // blocking, verbose
}


//--------------------------------------------------------------
void stepperThread::stop(){
    stopThread();
}


//--------------------------------------------------------------
void stepperThread::setup(){
    cout << "ST setup" << endl;
    connectToArduino();
    lastX = lastY = 0;
}


//--------------------------------------------------------------
void stepperThread::connectToArduino () {
    // ARDUINO
    
    // replace the string below with the serial port for your Arduino board
    // you can get this from the Arduino application or via command line
    // for OSX, in your terminal type "ls /dev/tty.*" to get a list of serial devices
	ard.connect("/dev/tty.usbmodem1411", 57600);
	
	// listen for EInitialized notification. this indicates that
	// the arduino is ready to receive commands and it is safe to
	// call setupArduino()
    cout << "ofAddListener" << endl;
	ofAddListener(ard.EInitialized, this, &stepperThread::setupArduino);
	bSetupArduino = false;	// flag so we setup arduino when its ready, you don't need to touch this :)
    
    curState = IDLE;
}



//--------------------------------------------------------------
void stepperThread::setupArduino(const int & version) {
	
    cout << "in setupArduino" << endl;
	// remove listener because we don't need it anymore
	ofRemoveListener(ard.EInitialized, this, &stepperThread::setupArduino);
    
    // it is now safe to send commands to the Arduino
    bSetupArduino = true;
    
    // Note: pins A0 - A5 can be used as digital input and output.
    // Refer to them as pins 14 - 19 if using StandardFirmata from Arduino 1.0.
    // If using Arduino 0022 or older, then use 16 - 21.
    // Firmata pin numbering changed in version 2.3 (which is included in Arduino 1.0)
    
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
}


//--------------------------------------------------------------
void stepperThread::updateArduino(){
    ard.update();
}

//--------------------------------------------------------------
void stepperThread::setTarget() {
    if (curPath < paths.size()) {
        // first point in drawing
        if (curPath == 0 && curPoint == 0) {
            pushInk = false;
            points = paths.at(curPath).getVertices();
            target = points.at(curPoint++);
            // a new point to draw
        } else if (curPoint < points.size()) {
            pushInk = true;
            target = points.at(curPoint++);
            // switch to a new path
        } else {
            pushInk = false;
            curPath++;
            curPoint = 0;
            points = paths.at(curPath).getVertices();
            target = points.at(curPoint++);
        }
        // all paths are drawn
    } else {
        curState = COFFEE_PHOTO;
        curPath = 0;
        curPoint = 0;
    }
    
    stepsX = abs((target.x - lastX) * 10);
    stepsY = abs((target.y - lastY) * 10);
    stepsInk = sqrt(stepsX*stepsX + stepsY*stepsY);
    limit = stepsX*stepsY;
    
    // set the directions for each servo
    int dir = (target.x > lastX) ? ARD_HIGH : ARD_LOW;
    ard.sendDigital(X_DIR_PIN, dir);
    dir = (target.y > lastY) ? ARD_HIGH : ARD_LOW;
    ard.sendDigital(Y_DIR_PIN, dir);
    
    lastX = target.x;
    lastY = target.y;
    updateTarget = false;
}


//--------------------------------------------------------------
void stepperThread::updateSteppers () {
    X_SIGNAL = Y_SIGNAL = INK_SIGNAL = ARD_HIGH;
    
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
    ard.sendDigital(Y_STEP_PIN, Y_SIGNAL);
    ard.sendDigital(INK_STEP_PIN, INK_SIGNAL);
    ofSleepMillis(MIN_PULSE);
    ard.sendDigital(X_STEP_PIN, ARD_LOW);
    ard.sendDigital(Y_STEP_PIN, ARD_LOW);
    ard.sendDigital(INK_STEP_PIN, ARD_LOW);
    ofSleepMillis(MIN_PULSE);
}


//--------------------------------------------------------------
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


//--------------------------------------------------------------
void stepperThread::update(){
    updateArduino();
    
    if (curState == PRINT) {
        stepsX = stepsY = 1000;
        counter++;
        updateSteppers();
        
        if (updateTarget) {
            setTarget();
            counter = 0;
        }
        if (counter < limit) {
            updateSteppers();
            counter++;
        } else {
            updateTarget = true;
        }
    }
}


//--------------------------------------------------------------
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




//--------------------------------------------------------------
void stepperThread::moveStepper(int num, int steps, float speed){
    
    /* NOTE: earliest code that moved the servo at speed '1' had a delay of 7/10 miliseconds
     * trying a delay of 1 milisecond now as max speed
     * speed should be from 0 – 1 with 1 being maximum speed
     */
    
    
    int DIR_PIN = (num == 0) ? X_DIR_PIN : Y_DIR_PIN;
    int STEP_PIN = (num == 0) ? X_STEP_PIN : Y_STEP_PIN;
    
    //rotate a specific number of microsteps (8 microsteps per step) - (negitive for reverse movement)
    //speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
    int dir = (steps > 0) ? ARD_HIGH : ARD_LOW;
    steps = abs(steps);
    
    ard.sendDigital(DIR_PIN, dir);
    
    // delay is inversely related to speed
    float delay = (1/speed);
    
    for(int i=0; i < steps; i++){
        ard.sendDigital(STEP_PIN, ARD_HIGH);
        ofSleepMillis(delay);
        ard.sendDigital(STEP_PIN, ARD_LOW);
        ofSleepMillis(delay);
    }
}


//--------------------------------------------------------------
void stepperThread::digitalPinChanged(const int & pinNum) {
    // do something with the digital input. here we're simply going to print the pin number and
    // value to the screen each time it changes
    buttonState = "digital pin: " + ofToString(pinNum) + " = " + ofToString(ard.getDigital(pinNum));
}

//--------------------------------------------------------------
void stepperThread::keyPressed(int key) {
	
    switch (key) {
        case ' ':
            counter = curPath = curPoint = 0;
            updateTarget = true;
            break;
        case '1':
            curState = KEY_PRESS;
            moveStepper(0, 100, 1);
            break;
        case '2':
            curState = KEY_PRESS;
            moveStepper(0, -200, 1);
            break;
        case '5':
            curState = KEY_PRESS;
            moveStepper(0, 500, 1);
            break;
        case 'q':
            curState = KEY_PRESS;
            moveStepper(1, 100, 1);
            break;
        case 'w':
            curState = KEY_PRESS;
            moveStepper(1, 200, 1);
            break;
        case 't':
            curState = KEY_PRESS;
            moveStepper(1, 500, 1);
            break;
        default:
            break;
    }
}
