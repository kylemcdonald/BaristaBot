#ifndef _ARDUINO_THREAD
#define _ARDUINO_THREAD

#include "ofMain.h"
#include "motorThread.h"


class arduinoThread : public ofThread{

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
        SHOOT_FACE,
        FACE_PHOTO,
        PRINTING,
        SHOOT_COFFEE,
        COFFEE_PHOTO,
    };
    const char* stateName[20] = {"START", "IDLE", "HOMING", "SHOOT_FACE", "FACE_PHOTO", "PRINTING", "SHOOT_COFFEE", "COFFEE_PHOTO"};
    state curState;
    
    ofArduino ard;
    motorThread X, Y, Z, INK;
    ofPoint home, current, target;
    vector<ofPolyline> paths;
    vector<ofPoint> points;
    string ex, wy;
    

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
    
    bool bSetupArduino;     // flag variable for setting up arduino once
    int DELAY_MIN = 500;    // in microseconds
    int TOL = 50;          // in steps, not for the syringe
    
    int cropped_size;
    int paths_i, points_i;
    int point_count;
    bool start_path, continuing_path, start_transition;
};

#endif
