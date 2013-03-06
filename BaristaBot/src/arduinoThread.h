#ifndef _ARDUINO_THREAD
#define _ARDUINO_THREAD

#include "ofMain.h"
#include "motorThread.h"


class arduinoThread : public ofThread{
    
// PINS
#define X_DIR_PIN 9
#define Y_DIR_PIN 7
#define Z_DIR_PIN 5
#define INK_DIR_PIN 3

#define X_STEP_PIN 8
#define Y_STEP_PIN 6
#define Z_STEP_PIN 4
#define INK_STEP_PIN 2

#define X_SLEEP_PIN 14
#define Y_SLEEP_PIN 15
#define Z_SLEEP_PIN 16
#define INK_SLEEP_PIN 17 // is 17

#define X_LIMIT_PIN 10
#define Y_LIMIT_PIN 12
#define Z_LIMIT_PIN 11
#define INK_LIMIT_PIN 13
    
// CONSTANTS
//#define DELAY_MIN 500        // in microseconds (20000 is good for debugging w/o robot)
#define TOL 1              // in steps, not for the syringe
#define INK_TIMEOUT 1500000  // in microseconds
#define INK_DELAY 2000        // in microseconds

#define HOME_X -128
#define HOME_Y 128
#define SCALE_X 200     // estimate 236.2 steps per mm in X -- was 200
#define SCALE_Y 125     // estimate 118.1 steps per mm in Y


public:
    void start();
    void stop();
    
    void setup();
    void initializeVariables();
    void initializeMotors();
    void initializeArduino();
    void setupArduino(const int & version);
    void updateArduino();
    
    void update();
    void journeyOn(bool new_coffee);
    void planJourney();
    void fireEngines();
    int  getSteps(float here, float there, bool is_x);
    bool journeysDone();
    
    void startInk();
    void stopInk();
    
    void shootFace();
    void shootCoffee();
    void goHome();
    
    void jogLeft();
    void jogRight();
    void jogForward();
    void jogBack();
    void jogUp();
    void jogDown();
    void plungerUp();
    void plungerDown();

    void threadedFunction();
    void draw();
    void digitalPinChanged(const int & pinNum);
    

    arduinoThread(){
        ard.sendReset();
    }
    
    enum state {
        START,
        IDLE,
        HOMING,
        HOME,
        SHOOT_FACE,
        FACE_PHOTO,
        PRINTING,
        SHOOT_COFFEE,
        COFFEE_PHOTO,
        DONE
    };
    const char* stateName[20] = {"START", "IDLE", "HOMING", "HOME", "SHOOT_FACE", "FACE_PHOTO", "PRINTING", "SHOOT_COFFEE", "COFFEE_PHOTO", "DONE"};
    state curState;
    
    ofArduino ard;
    motorThread X, Y, Z, INK;
    ofPoint home, current, target;
    vector<ofPolyline> paths;
    vector<ofPoint> points;
    string ex, wy, hex, hwy;
    
    int DELAY_MIN = 500;
    int cropped_size;
    int paths_i, points_i;
    int point_count;
    bool start_path, end_path, start_transition;
    bool bSetupArduino;     // flag variable for setting up arduino once
};

#endif
