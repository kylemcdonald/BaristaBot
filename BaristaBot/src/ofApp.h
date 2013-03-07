#pragma once

#include "ofMain.h"
#include "ofxAutoControlPanel.h"
#include "ofxCv.h"

#include "imatrix.h"
#include "ETF.h"
#include "fdog.h"
#include "myvec.h"

#include "arduinoThread.h"

class ofApp : public ofBaseApp{
public:
    void setup();
	void update();
	void draw();
	void drawPaths();
    void setTarget ();
	void keyPressed(int key);
    void exit();
    
    int getPoints(int steps);
    
    imatrix img;
	ETF etf;
    ofxAutoControlPanel gui;
	ofVideoGrabber cam;
	ofImage gray, cld, thresholded, thinned, io, jk, nm;
    ofImage graySmall, cropped;
    cv::CascadeClassifier classifier;
    arduinoThread AT;
    
    vector<cv::Rect> objects;
	vector<ofPolyline> paths;
    ofPolyline ln;

    
	float faceTrackingScaleFactor;
    int croppedSize;
    int camWidth, camHeight;
	bool needToUpdate;
    
    float minGapLength;
    int minPathLength;

};
