#include "ofApp.h"

using namespace ofxCv;
using namespace cv;



//--------------------------------------------------------------
void removeIslands(ofPixels& img) {
	int w = img.getWidth(), h = img.getHeight();
	int ia1=-w-1,ia2=-w-0,ia3=-w+1,ib1=-0-1,ib3=-0+1,ic1=+w-1,ic2=+w-0,ic3=+w+1;
	unsigned char* p = img.getPixels();
	for(int y = 1; y + 1 < h; y++) {
		for(int x = 1; x + 1 < w; x++) {
			int i = y * w + x;
			if(p[i]) {
				if(!p[i+ia1]&&!p[i+ia2]&&!p[i+ia3]&&!p[i+ib1]&&!p[i+ib3]&&!p[i+ic1]&&!p[i+ic2]&&!p[i+ic3]) {
					p[i] = 0;
				}
			}
		}
	}
}


//--------------------------------------------------------------
typedef std::pair<int, int> intPair;
vector<ofPolyline> getPaths(ofPixels& img, float minGapLength = 2, int minPathLength = 0) {
	float minGapSquared = minGapLength * minGapLength;
	
	list<intPair> remaining;
	int w = img.getWidth(), h = img.getHeight();
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			if(img.getColor(x, y).getBrightness() > 128) {
				remaining.push_back(intPair(x, y));
			}
		}
	}
	
	vector<ofPolyline> paths;
	if(!remaining.empty()) {
		int x = remaining.back().first, y = remaining.back().second;
		while(!remaining.empty()) {
			int nearDistance = 0;
			list<intPair>::iterator nearIt, it;
			for(it = remaining.begin(); it != remaining.end(); it++) {
				intPair& cur = *it;
				int xd = x - cur.first, yd = y - cur.second;
				int distance = xd * xd + yd * yd;
				if(it == remaining.begin() || distance < nearDistance) {
					nearIt = it, nearDistance = distance;
					// break for immediate neighbors
					if(nearDistance < 4) {
						break;
					}
				}
			}
			intPair& next = *nearIt;
			x = next.first, y = next.second;
			if(paths.empty()) {
				paths.push_back(ofPolyline());
			} else if(nearDistance >= minGapSquared) {
				if(paths.back().size() < minPathLength) {
					paths.back().clear();
				} else {
					paths.push_back(ofPolyline());
				}
			}
			paths.back().addVertex(ofVec2f(x, y));
			remaining.erase(nearIt);
		}
	}
	
	return paths;
}


//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
	ofEnableSmoothing();
	
    cam.setDeviceID(0);
	camWidth = 640, camHeight = 480;
	cam.initGrabber(camWidth, camHeight);
	//    cam.listDevices();
	
	faceTrackingScaleFactor = .5;
	croppedSize = 256;
	classifier.load(ofToDataPath("haarcascade_frontalface_alt2.xml"));
	graySmall.allocate(camWidth * faceTrackingScaleFactor, camHeight * faceTrackingScaleFactor, OF_IMAGE_GRAYSCALE);
	cropped.allocate(croppedSize, croppedSize, OF_IMAGE_GRAYSCALE);
	
	img.init(croppedSize, croppedSize);
	imitate(gray, cropped);
	imitate(cld, cropped);
	imitate(thresholded, cropped);
	imitate(thinned, cropped);
	
	gui.setup();
	gui.addPanel("Settings");
	gui.addSlider("black", 0, -255, 255, true);
	gui.addSlider("sigma1", 0.4, 0.01, 2.0, false);
	gui.addSlider("sigma2", 3.0, 0.01, 10.0, false);
	gui.addSlider("tau", 0.97, 0.8, 1.0, false);
	gui.addSlider("halfw", 4, 1, 8, true);
	gui.addSlider("smoothPasses", 2, 1, 4, true);
	gui.addSlider("thresh", 128, 0, 255, false);
	gui.addSlider("minGapLength", 2, 2, 12, false);
	gui.addSlider("minPathLength", 20, 0, 50, true);
	gui.addSlider("facePadding", 1.5, 0, 2, false);
	gui.loadSettings("settings.xml");
    
    // ARDUINO
    
    // replace the string below with the serial port for your Arduino board
    // you can get this from the Arduino application or via command line
    // for OSX, in your terminal type "ls /dev/tty.*" to get a list of serial devices
	ard.connect("/dev/tty.usbmodem1411", 57600);
	
	// listen for EInitialized notification. this indicates that
	// the arduino is ready to receive commands and it is safe to
	// call setupArduino()
	ofAddListener(ard.EInitialized, this, &ofApp::setupArduino);
	bSetupArduino = false;	// flag so we setup arduino when its ready, you don't need to touch this :)
    
    curState = IDLE;
}


//--------------------------------------------------------------
void ofApp::update(){
	cam.update();
	if(cam.isFrameNew() && needToUpdate) {
        // tied to threshold somehow, maybe prethreshold
		int black = gui.getValueI("black");
        
        // level of detail, freq of image
        // if both low, super detailed and jittery data
        // if sigma2 or both high, all features will be swirly lower freq
        // both high yields thick lines, most dominant edges; also ghost lines
		float sigma1 = gui.getValueF("sigma1");
		float sigma2 = gui.getValueF("sigma2");
        
        // kind of related to high pass HDR look
        // value of 1 (high) will create edges everwhere it can imaging
		float tau = gui.getValueF("tau");
        
        // high thresh gives more lines (more black), low gives less
		float thresh = gui.getValueF("thresh");
        
        // some measure of local vs. global complexity
        // low gives lots of swirls, high gives longer lines
		int halfw = gui.getValueI("halfw");
        
        // tries to make contours smoother, low number is sharper
		int smoothPasses = gui.getValueI("smoothPasses");
        
		float minGapLength = gui.getValueF("minGapLength");
		int minPathLength = gui.getValueI("minPathLength");
		
		float facePadding = gui.getValueF("facePadding");
		
		convertColor(cam, gray, CV_RGB2GRAY);
		
		resize(gray, graySmall);
		Mat graySmallMat = toCv(graySmall);
		equalizeHist(graySmallMat, graySmallMat);		
		classifier.detectMultiScale(graySmallMat, objects, 1.05, 1,
									CASCADE_DO_CANNY_PRUNING |
									CASCADE_FIND_BIGGEST_OBJECT |
									//CASCADE_DO_ROUGH_SEARCH |
									0);
		
		ofRectangle faceRect;
		if(objects.empty()) {
			// if there are no faces found, use the whole canvas
			faceRect.set(0, 0, camWidth, camHeight);
		} else {
			faceRect = toOf(objects[0]);
			faceRect.getPositionRef() /= faceTrackingScaleFactor;
			faceRect.scale(1 / faceTrackingScaleFactor);
			faceRect.scaleFromCenter(facePadding);
		}
		
		ofRectangle camBoundingBox(0, 0, camWidth, camHeight);
		faceRect = faceRect.getIntersection(camBoundingBox);
		float whDiff = fabsf(faceRect.width - faceRect.height);
		if(faceRect.width < faceRect.height) {
			faceRect.height = faceRect.width;
			faceRect.y += whDiff / 2;
		} else {
			faceRect.width = faceRect.height;
			faceRect.x += whDiff / 2;
		}
		
		cv::Rect roi = toCv(faceRect);
		Mat grayMat = toCv(gray);
		Mat croppedGrayMat(grayMat, roi);
		resize(croppedGrayMat, cropped);
		cropped.update();
		
		int j = 0;
		unsigned char* grayPixels = cropped.getPixels();
		for(int y = 0; y < croppedSize; y++) {
			for(int x = 0; x < croppedSize; x++) {
				img[x][y] = grayPixels[j++] - black;
			}
		}
		etf.init(croppedSize, croppedSize);
		etf.set(img);
		etf.Smooth(halfw, smoothPasses);
		GetFDoG(img, etf, sigma1, sigma2, tau);
		j = 0;
		unsigned char* cldPixels = cld.getPixels();
		for(int y = 0; y < croppedSize; y++) {
			for(int x = 0; x < croppedSize; x++) {
				cldPixels[j++] = img[x][y];
			}
		}
		threshold(cld, thresholded, thresh, true);
		copy(thresholded, thinned);
		thin(thinned);
		removeIslands(thinned.getPixelsRef());
		
		gray.update();
		cld.update();
		thresholded.update();
		thinned.update();
		
		paths = getPaths(thinned, minGapLength, minPathLength);
		
		needToUpdate = false;
        curState = DRAW;
    }
    
    // ARDUINO
    
    updateArduino();
    
}


//--------------------------------------------------------------
void ofApp::setupArduino(const int & version) {
	
	// remove listener because we don't need it anymore
	ofRemoveListener(ard.EInitialized, this, &ofApp::setupArduino);
    
    // it is now safe to send commands to the Arduino
    bSetupArduino = true;
    gui.msg += "arduino connected";
    gui.msg += ard.getFirmwareName();
    
    // Note: pins A0 - A5 can be used as digital input and output.
    // Refer to them as pins 14 - 19 if using StandardFirmata from Arduino 1.0.
    // If using Arduino 0022 or older, then use 16 - 21.
    // Firmata pin numbering changed in version 2.3 (which is included in Arduino 1.0)
    
    // set digital outputs
    ard.sendDigitalPinMode(X_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(X_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Z_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Z_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Y_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Y_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(INK_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(INK_STEP_PIN, ARD_OUTPUT);

    // set digital inputs
    ard.sendDigitalPinMode(X_LIMIT_PIN, ARD_INPUT);
    ard.sendDigitalPinMode(Z_LIMIT_PIN, ARD_INPUT);
    ard.sendDigitalPinMode(Y_LIMIT_PIN, ARD_INPUT);
    ard.sendDigitalPinMode(INK_LIMIT_PIN, ARD_INPUT);
    ofAddListener(ard.EDigitalPinChanged, this, &ofApp::digitalPinChanged);
    
    startX = startY = 0;
    speedX = speedY = 1;
}


//--------------------------------------------------------------
void ofApp::updateArduino(){
	if(bSetupArduino) {
		// update the arduino, get any data or messages.
		// the call to ard.update() is required
		ard.update();
	}
}

//--------------------------------------------------------------
void ofApp::drawPaths() {
	for(int i = 0; i < paths.size(); i++) {
		ofSetColor(yellowPrint);
		paths[i].draw();
		if(i + 1 < paths.size()) {
			ofVec2f endPoint = paths[i].getVertices()[paths[i].size() - 1];
			ofVec2f startPoint = paths[i + 1].getVertices()[0];
			ofSetColor(magentaPrint, 128);
			ofLine(endPoint, startPoint);
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(0);
	
//    if (curState == DRAW) {
        ofSetColor(255);
        gray.draw(0, 0);
		int y = 0;
		cropped.draw(gray.getWidth(), 0);
        cld.draw(gray.getWidth(), (y+=cropped.getHeight()));
        thresholded.draw(gray.getWidth(), (y+=cld.getHeight()));
        thinned.draw(gray.getWidth(), (y+=thresholded.getHeight()));
	
		ofPushMatrix();
		ofTranslate(gray.getWidth(), 0);
		drawPaths();
		ofPopMatrix();
	
		ofPushMatrix();
		ofTranslate(gray.getWidth() + cropped.getWidth(), 0);
		ofScale(2, 2);
		ofPushStyle();
		ofSetLineWidth(3);
		drawPaths();
		ofPopStyle();
		ofPopMatrix();
//        curState = PRINT;
//    }
    
    // ARDUINO
    
	gui.msg = "curState = " + ofToString(stateName[curState]) + ". ";
	if (!bSetupArduino){
		gui.msg += "arduino not ready...";
	}
    
    // Draw the polylines on the coffee
    
    if (curState == PRINT) {
        for (int i = 0; i < paths.size(); i++) {
            gui.msg = "Path " + ofToString(i+1) + " / " + ofToString(paths.size());
            vector<ofPoint> points = paths.at(i).getVertices();
            for (int j = 0; j < points.size(); j++) {
                gui.msg += " | Point " + ofToString(j) + " / " + ofToString(points.size());
                if (j == points.size()) {
                    // dont draw
                    moveTo (points.at(j).x, points.at(j).y, false);
                } else {
                    // draw
                    moveTo (points.at(j).x, points.at(j).y, true);
                }
            }
            if (i-1 == paths.size()) {
                curState = COFFEE_PHOTO;
            }
        }
    }
}


//--------------------------------------------------------------
void ofApp::moveTo (float exx, float wyy, bool draw) {
    endX = exx;
    endY = wyy;
    stepsX = (endX - startX) * 10;
    stepsY = (endY - startY) * 10;
    
    float stepsInk = sqrt(stepsX*stepsX + stepsY*stepsY);
    
	//    stepsY = stepsY * 3;
    
    // scale the speed
    if (abs(stepsX) > abs(stepsY)) {
        speedX = 1;
        speedY = abs(stepsY) / abs(stepsX);
    } else {
        speedY = 1;
        speedX = abs(stepsX) / abs(stepsY);
    }
    
    cout << "\nMOVING STEPPERS" << endl;
    cout << "startX: " << startX << " endX: " << endX << " stepsX: " << stepsX << " speedX: " << speedX << endl;
    cout << "startY: " << startY << " endY: " << endY << " stepsY: " << stepsY << " speedY: " << speedY << endl;
    
    // this means that aside from 0, the shortest movement will be 1/64th of an inch
    // becuase min path is 10 and 400 steps per 1/16th of an inch
    // 10*10 = 100 --> 1/64th of an inch
    moveStepper(0, stepsX, speedX);
    moveStepper(1, stepsY, speedY);
    if (draw) {
        moveStepper(3, stepsInk, 0.01);
    }
    startX = endX;
    startY = endY;
}


//--------------------------------------------------------------
void ofApp::moveStepper(int num, int steps, float speed){
    
    /* NOTE: earliest code that moved the servo at speed '1' had a delay of 7/10 miliseconds
     * trying a delay of 1 milisecond now as max speed
     * speed should be from 0 – 1 with 1 being maximum speed
     */
    
    
    int DIR_PIN = (num == 0) ? X_DIR_PIN : Y_DIR_PIN;
    int STEP_PIN = (num == 0) ? X_STEP_PIN : Y_STEP_PIN;
    
    //rotate a specific number of microsteps (8 microsteps per step) - (negitive for reverse movement)
    //speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
    int dir = (steps > 0) ? ARD_HIGH : ARD_LOW;
    steps = abs(steps);
    
    ard.sendDigital(DIR_PIN, dir);
    
    // delay is inversely related to speed
    float delay = (1/speed);
    
    for(int i=0; i < steps; i++){
        ard.sendDigital(STEP_PIN, ARD_HIGH);
        ofSleepMillis(delay);
        ard.sendDigital(STEP_PIN, ARD_LOW);
        ofSleepMillis(delay);
    }
}


//--------------------------------------------------------------
void ofApp::digitalPinChanged(const int & pinNum) {
    // do something with the digital input. here we're simply going to print the pin number and
    // value to the screen each time it changes
    buttonState = "digital pin: " + ofToString(pinNum) + " = " + ofToString(ard.getDigital(pinNum));
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	
    switch (key) {
        case ' ':
            needToUpdate = true;
            break;
        case '1':
            moveStepper(0, 100, 1);
            curState = KEY_PRESS;
            break;
        case '2':
            moveStepper(0, -200, 1);
            curState = KEY_PRESS;
            break;
        case '5':
            moveStepper(0, 500, 1);
            break;
        case 'q':
            moveStepper(1, 100, 1);
            break;
        case 'w':
            moveStepper(1, 200, 1);
            break;
        case 't':
            moveStepper(1, 500, 1);
            break;
        default:
            break;
    }
}
