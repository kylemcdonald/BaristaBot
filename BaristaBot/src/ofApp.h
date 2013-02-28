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
	
	imatrix img;
	ETF etf;
	
	ofVideoGrabber cam;
	ofImage gray, cld, thresholded, thinned;
	
	bool needToUpdate;	
	
	ofxAutoControlPanel gui;
    
    // STEPPERS
    void moveStepper (int num, int steps, float speed);
    void moveTo (float exx, float wyy);
    void pushInk ();
    void stopInk ();
    float startX, startY, endX, endY, stepsX, stepsY, speedX, speedY;
    
	
    ofArduino ard;
	bool bSetupArduino;			// flag variable for setting up arduino once
	
    // TESTING
    
    ofRectangle rect;
    
    enum state {
        IDLE,
        DRAW,
        FACE_PHOTO,
        UPLOAD_FACE,
        PRINT,
        COFFEE_PHOTO,
        UPLOAD_COFFEE,
        ONE_KEY,
        TWO_KEY,
    };
    
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
    
    string buttonState;
    string potValue;
};
