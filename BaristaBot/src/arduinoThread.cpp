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
    X.stop();
    Z.stop();
    Y.stop();
    INK.stop();
}

//--------------------------------------------------------------
void arduinoThread::setup(){
    // do not change this sequence
    initializeVariables();
    initializeMotors();
    initializeArduino();
}

//--------------------------------------------------------------
void arduinoThread::initializeVariables(){
    X_LIMIT  = Z_LIMIT  = Y_LIMIT  = INK_LIMIT  = false;
}


//--------------------------------------------------------------
void arduinoThread::initializeMotors(){
    // pass arduino reference and pins to motor threads
    X.setArduino    (ard, X_STEP_PIN,   X_DIR_PIN,   X_SLEEP_PIN,   "Motor X");
    Z.setArduino    (ard, Z_STEP_PIN,   Z_DIR_PIN,   Z_SLEEP_PIN,   "Motor Z");
    Y.setArduino    (ard, Y_STEP_PIN,   Y_DIR_PIN,   Y_SLEEP_PIN,   "Motor Y");
    INK.setArduino  (ard, INK_STEP_PIN, INK_DIR_PIN, INK_SLEEP_PIN, "Motor I");
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

void arduinoThread::test(){
//    ard.sendDigital(INK_SLEEP_PIN, ARD_HIGH);
    INK.aim();
}

//--------------------------------------------------------------
void arduinoThread::home(){

    curState = HOMING;
    
//    X.ready(100000, 500);
//    X.start();
    
//    Z.ready(10000, 500);
//    Z.start();
//    
//    Y.ready(10000, 500);
//    Y.start();
//    
    INK.ready(10000, 500);
    INK.start();
}


//void arduinoThread::shootFace(){
//    ard.sendDigital(Z_DIR_PIN, ARD_HIGH);
//    
//    Z.ready(10000, 200);
//    Z.start();
//    while (Z.i < 10000) {}
//    // take photo here
//}
//
//void arduinoThread::shootCoffee(){
//    ard.sendDigital(Z_DIR_PIN, ARD_HIGH);
//    
//    // aim and move X and Y here depending on observations after print
//    
//    Z.ready(2000, 200);
//    Z.start();
//    while (Z.i < 2000) {}
//
//    //--------------------- take photo here -------------------------//
//    
//    // home all to get out of way of cup and be ready for next print
//    Z.ready(-2000, 200);
//    Z.start();
//    home();
//}

//--------------------------------------------------------------
void arduinoThread::update(){
    ard.update();
    
//    if (curState == PRINT) {
//        // assume that we are starting from home, robot will home after coffee photo
//        
//    }
//
//    
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
//    
//
//    // Draw the polylines on the coffee
//
//    if (curState == PRINT) {
//        for (int i = 0; i < paths.size(); i++) {
//            cout << "\n\n\nPath " << i+1 << " / " << paths.size() << endl;
//            vector<ofPoint> points = paths.at(i).getVertices();
//            cout << "\n points.size() = " << points.size() << endl;
//            for (int j = 0; j < points.size(); j++) {
//                if (j == 0) {
//                    pushInk();
//                } else if (j == points.size()) {
//                    stopInk();
//                }
//                moveTo (points.at(j).x, points.at(j).y);
//            }
//            if (i-1 == paths.size()) {
//                curState = COFFEE_PHOTO;
//                cout << "\n\n\n\n\n"
//                    "\n***************************************************************"
//                    "\n************************ COFFEE_PHOTO *************************"
//                    "\n***************************************************************"
//                     << "\n\n\n\n\n" << endl;
//            }
//        }
//    }
//    
//    endX = exx;
//    endY = wyy;
//    stepsX = (endX - startX) * 10;
//    stepsY = (endY - startY) * 10;
//    
//    //    stepsY = stepsY * 3;
//    
//    // scale the speed
//    if (abs(stepsX) > abs(stepsY)) {
//        speedX = 1;
//        speedY = abs(stepsY) / abs(stepsX);
//    } else {
//        speedY = 1;
//        speedX = abs(stepsX) / abs(stepsY);
//    }

    

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
   
    string str = "curState = " + ofToString(stateName[curState]) + ". ";
    if (!bSetupArduino){
		str += "arduino not ready...";
	} else {
        str += "arduino connected, firmware: " + ofToString(ard.getFirmwareName());
    }

    ofDrawBitmapString(str, 50, 700);
    
    X.draw();
    Y.draw();
    Z.draw();
    INK.draw();
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
