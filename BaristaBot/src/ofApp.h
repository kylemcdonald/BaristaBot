#pragma once

#include "ofMain.h"
#include "ofxAutoControlPanel.h"
#include "ofxCv.h"

#include "imatrix.h"
#include "ETF.h"
#include "fdog.h"
#include "myvec.h"

class ofApp : public ofBaseApp{
public:
	void setup();
	void update();
	void draw();
	void drawPaths();
	
	void keyPressed(int key);
	
	int camWidth, camHeight;
	
	vector<ofPolyline> paths;
	vector<ofPoint> points;
    
	imatrix img;
	ETF etf;
	
	ofVideoGrabber cam;
	ofImage gray, cld, thresholded, thinned;
	
	bool needToUpdate;	
	
	ofxAutoControlPanel gui;
    
    // PINS
    int X_DIR_PIN = 2;
    int X_STEP_PIN = 3;
    int Z_DIR_PIN = 4;
    int Z_STEP_PIN = 5;
    int Y_DIR_PIN = 6;
    int Y_STEP_PIN = 7;
    int INK_DIR_PIN = 8;
    int INK_STEP_PIN = 9;
    
    int X_LIMIT_PIN = 10;
    int Z_LIMIT_PIN = 11;
    int Y_LIMIT_PIN = 12;
    int INK_LIMIT_PIN = 13;
    
    // STEPPERS
    void moveStepper (int num, int steps, float speed);
    void moveTo (float exx, float wyy);
    void setTarget ();
    float startX, startY, endX, endY, speedX, speedY;
    int stepsX, stepsY, stepsInk;
    bool X_SIGNAL, Y_SIGNAL, INK_SIGNAL;
    int counter, limit;
    float MIN_PULSE = 0.010; // in milliseconds
    bool pushInk, updateTarget;
    int curPath, curPoint;
    ofPoint target;
    
	
    ofArduino ard;
	bool bSetupArduino;			// flag variable for setting up arduino once
	
    // TESTING
    
    ofRectangle rect;
    
    enum state {
        IDLE,
        FACE_PHOTO,
        PRINT,
        COFFEE_PHOTO,
        KEY_PRESS,
    };
    const char* stateName[20] = {"IDLE", "FACE_PHOTO", "PRINT", "COFFEE_PHOTO", "KEY_PRESS"};
    state curState;
	
	int croppedSize;
	ofImage graySmall, cropped;
	
	cv::CascadeClassifier classifier;
	vector<cv::Rect> objects;
	float faceTrackingScaleFactor;
    
private:
    
    void setupArduino(const int & version);
    void digitalPinChanged(const int & pinNum);
    void analogPinChanged(const int & pinNum);
	void updateArduino();
    void updateSteppers();
    
    string buttonState;
    string potValue;
};
