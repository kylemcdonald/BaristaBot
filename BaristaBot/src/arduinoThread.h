#ifndef _ARDUINO_THREAD
#define _ARDUINO_THREAD

#include "ofMain.h"


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

#define X_LIMIT_PIN 10
#define Y_LIMIT_PIN 12
#define Z_LIMIT_PIN 11
#define INK_LIMIT_PIN 13

    
public:
    
    // GUI adjustable
    int INK_SLEEP_PIN = 17;  // is 17

    int SCALE_X = 62;    // first estimate 236.2 steps per mm in X || 125 GOOD but big
    int SCALE_Y = 50;    // first estimate 118.1 steps per mm in Y || 100 GOOD but big
    int home_x = -100;
    int home_y = -100;
    int z_height = 8000;
    int y_height = 17000;
    
    int TOL = 20;        // in steps, not for the syringe
    
    int DELAY_FAST = 900;
    int DELAY_MIN = 2000;
    int HIGH_DELAY = 50;
    
    int INK_DELAY = 7000;
    int INK_START_DELAY = 800;
    int INK_START_STEPS = 500;
    int INK_STOP_DELAY = 800;
    int INK_STOP_STEPS = 300;
    int INK_WAIT = 5000000;
    
    // Variables
    int INK_TRAVEL = 0;
    
    unsigned long long x_timer, y_timer, z_timer, i_timer;
    int x_delay, y_delay, z_delay, i_delay;
    int x_steps, y_steps, z_steps, i_steps, x_inc, y_inc, z_inc, i_inc;
    bool x_go, y_go, z_go, i_go;
    
    int cropped_size;
    int paths_i, points_i;
    int point_count;
    bool start_path, end_path, start_transition;
    bool bSetupArduino;     // flag variable for setting up arduino once
    
    ofArduino ard;
    ofPoint current, target;
    vector<ofPolyline> paths;
    vector<ofPoint> points;
    string ex, wy, hex, hwy;
    
    enum state {
        START,
        IDLE,
        JOG,
        HOMING,
        HOME,
        SHOOT_FACE,
        NEED_PHOTO,
        FACE_PHOTO,
        PREPRINT,
        PRINTING,
        SHOOT_COFFEE,
        COFFEE_PHOTO,
        DONE,
        ERROR,
        RESET
    };
    const char* stateName[25] = {"START", "IDLE", "JOG", "HOMING", "HOME", "SHOOT_FACE", "NEED_PHOTO", "FACE_PHOTO", "PREPRINT", "PRINTING", "SHOOT_COFFEE", "COFFEE_PHOTO", "DONE", "ERROR", "RESET"};
    state curState;

    
    // Methods
    arduinoThread(){
        ard.sendReset();
    }
    
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
    bool allDone();
    
    void startInk();
    void stopInk();
    
    void stopX();
    void stopY();
    void stopZ();
    void homeX();
    void homeY();
    void homeZ();
    
    void shootCoffee();
    void shootFace();
    void raiseY();
    void goHome();
    void reset();
    void sleepDoneMotors();
    
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
};

#endif
