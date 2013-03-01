#ifndef _STEPPER_THREAD
#define _STEPPER_THREAD

#include "ofMain.h"

// this is not a very exciting example yet
// but ofThread provides the basis for ofNetwork and other
// operations that require threading.
//
// please be careful - threading problems are notoriously hard
// to debug and working with threads can be quite difficult


class stepperThread : public ofThread{

public:
    void start();
    void stop();
    void setup();
    void connectToArduino();
    void setupArduino(const int & version);
    void updateArduino();
    void setTarget();
    void updateSteppers();
    void threadedFunction();
    void update();
    void draw();
    void moveStepper(int num, int steps, float speed);
    void digitalPinChanged(const int & pinNum);
    void keyPressed(int key);
    
    
    ofArduino ard;
    ofPoint target;
    vector<ofPolyline> paths;
    vector<ofPoint> points;

    enum state {
        IDLE,
        FACE_PHOTO,
        PRINT,
        COFFEE_PHOTO,
        KEY_PRESS,
    };
    const char* stateName[20] = {"IDLE", "FACE_PHOTO", "PRINT", "COFFEE_PHOTO", "KEY_PRESS"};
    state curState;

    // PINS
    int X_DIR_PIN = 2;
    int X_STEP_PIN = 3;
    int Z_DIR_PIN = 4;
    int Z_STEP_PIN = 5;
    int Y_DIR_PIN = 6;
    int Y_STEP_PIN = 7;
    int INK_DIR_PIN = 8;
    int INK_STEP_PIN = 9;
    
    const int X_LIMIT_PIN = 10;
    const int Z_LIMIT_PIN = 11;
    const int Y_LIMIT_PIN = 12;
    const int INK_LIMIT_PIN = 13;
    
    bool X_SIGNAL, Z_SIGNAL, Y_SIGNAL, INK_SIGNAL;
    bool X_LIMIT, Z_LIMIT, Y_LIMIT, INK_LIMIT;
    bool bSetupArduino;			// flag variable for setting up arduino once
    int lastX, lastY;
    float MIN_PULSE = 0.2; // in milliseconds
    
    int startX, startY, endX, endY, speedX, speedY;
    int stepsX, stepsY, stepsInk;
    int counter, limit;
    int curPath, curPoint;
    bool pushInk, updateTarget;
    
    string buttonState;
    string potValue;
      
    int count;

};

#endif
