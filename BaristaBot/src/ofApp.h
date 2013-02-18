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
	
	void keyPressed(int key);

    ofTrueTypeFont font;

	int camWidth, camHeight;
	
	vector<ofPolyline> paths;
	
	imatrix img;
	ETF etf;
	
	ofVideoGrabber cam;
	ofImage gray, cld, thresholded, thinned;
	
	bool needToUpdate;	
	
	ofxAutoControlPanel gui;
    
    // ARDUINO
    void moveStepper (int steps, float speed);

    ofArduino	ard;
	bool		bSetupArduino;			// flag variable for setting up arduino once
    int DIR_PIN = 3;
    int STEP_PIN = 4;
    
private:
    
    void setupArduino(const int & version);
    void digitalPinChanged(const int & pinNum);
    void analogPinChanged(const int & pinNum);
	void updateArduino();
    
    string buttonState;
    string potValue;
};
