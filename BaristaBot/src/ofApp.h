#pragma once

#include "ofMain.h"
#include "ofxAutoControlPanel.h"
#include "ofxCv.h"

#include "imatrix.h"
#include "ETF.h"
#include "fdog.h"
#include "myvec.h"

#include "stepperThread.h"

class ofApp : public ofBaseApp{
public:
    void setup();
	void update();
	void draw();
	void drawPaths();
    void setTarget ();
	void keyPressed(int key);
    void exit();
    
    imatrix img;
	ETF etf;
    ofxAutoControlPanel gui;
	ofVideoGrabber cam;
	ofImage gray, cld, thresholded, thinned;
    ofImage graySmall, cropped;
    cv::CascadeClassifier classifier;
    stepperThread ST;
    
    vector<cv::Rect> objects;
	vector<ofPolyline> paths;

    
	float faceTrackingScaleFactor;
    int croppedSize;
    int camWidth, camHeight;
	bool needToUpdate;

};
