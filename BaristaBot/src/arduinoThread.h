#ifndef _ARDUINO_THREAD
#define _ARDUINO_THREAD

#include "ofMain.h"
#include "motorThread.h"


class arduinoThread : public ofThread{

public:
    void start();
    void stop();
    void setup();
    
    void initializeArduino();
    void setupArduino(const int & version);
    void initializeMotors();
    void initializeVariables();
    
    void update();
    void updateArduino();
    
    void goHome();
    void journey(ofPoint orig, ofPoint dest);
    bool journeysDone();
    void shootFace();
    void shootCoffee();
    ofPoint getNextTarget();
    
    void jogLeft();
    void jogRight();
    void jogForward();
    void jogBack();
    void jogUp();
    void jogDown();

    
    void threadedFunction();
    void draw();
    void digitalPinChanged(const int & pinNum);
    

    arduinoThread(){
        ard.sendReset();
    }
    
    ofArduino ard;
    motorThread X, Y, Z, INK;
    ofPoint home, current, target;
    vector<ofPolyline> paths;
    vector<ofPoint> points;
    

    
    enum state {
        START,
        IDLE,
        HOMING,
        SHOOT_FACE,
        FACE_PHOTO,
        PRINTING,
        SHOOT_COFFEE,
        COFFEE_PHOTO,
    };
    const char* stateName[20] = {"START", "IDLE", "HOMING", "SHOOT_FACE", "FACE_PHOTO", "PRINTING", "SHOOT_COFFEE", "COFFEE_PHOTO"};
    state curState;

    // PINS
    int X_DIR_PIN = 9;
    int Y_DIR_PIN = 7;
    int Z_DIR_PIN = 5;
    int INK_DIR_PIN = 3;
    
    int X_STEP_PIN = 8;
    int Y_STEP_PIN = 6;
    int Z_STEP_PIN = 4;
    int INK_STEP_PIN = 2;

    int X_SLEEP_PIN = 14;
    int Y_SLEEP_PIN = 15;
    int Z_SLEEP_PIN = 16;
    int INK_SLEEP_PIN = 17;
    
    const int X_LIMIT_PIN = 10;
    const int Y_LIMIT_PIN = 12;
    const int Z_LIMIT_PIN = 11;
    const int INK_LIMIT_PIN = 13;
    
    bool X_LIMIT, Y_LIMIT, Z_LIMIT, INK_LIMIT;
    bool bSetupArduino;     // flag variable for setting up arduino once
    int DELAY_MIN = 1000;    // in microseconds
    
    int home_x, home_y, steps_x, steps_y, delay_x, delay_y;
    int paths_i, points_i;
    bool should_ink;
    
    int startX, startY, endX, endY, speedX, speedY;
    int stepsX, stepsY, stepsInk;
    int counter, limit;
    int curPath, curPoint;
    bool pushInk, updateTarget;
        
    string buttonState;
};

#endif
