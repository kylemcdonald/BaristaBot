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
    font.loadFont("franklinGothic.otf", 20);
	
	camWidth = 640, camHeight = 480;
	cam.initGrabber(camWidth, camHeight);
	
	img.init(camWidth, camHeight);
	imitate(gray, cam, CV_8UC1);
	imitate(cld, gray);
	imitate(thresholded, gray);
	imitate(thinned, gray);
	
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
	gui.addSlider("minPathLength", 10, 0, 50, true);
	gui.loadSettings("settings.xml");
    
    // ARDUINO
    
    // replace the string below with the serial port for your Arduino board
    // you can get this from the Arduino application or via command line
    // for OSX, in your terminal type "ls /dev/tty.*" to get a list of serial devices
	ard.connect("/dev/tty.usbmodem1421", 57600);
	
	// listen for EInitialized notification. this indicates that
	// the arduino is ready to receive commands and it is safe to
	// call setupArduino()
	ofAddListener(ard.EInitialized, this, &ofApp::setupArduino);
	bSetupArduino	= false;	// flag so we setup arduino when its ready, you don't need to touch this :)
    
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
		
		convertColor(cam, gray, CV_RGB2GRAY);
		
		int j = 0;
		unsigned char* grayPixels = gray.getPixels();
		for(int y = 0; y < camHeight; y++) {
			for(int x = 0; x < camWidth; x++) {
				img[x][y] = grayPixels[j++] - black;
			}
		}
		etf.init(camWidth, camHeight);
		etf.set(img);
		etf.Smooth(halfw, smoothPasses);
		GetFDoG(img, etf, sigma1, sigma2, tau);
		j = 0;
		unsigned char* cldPixels = cld.getPixels();
		for(int y = 0; y < camHeight; y++) {
			for(int x = 0; x < camWidth; x++) {
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
        curState = PRINT;
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
    cout << "arduino connected\n" << endl; 
    
    // print firmware name and version to the console
    cout << ard.getFirmwareName() << endl;
    cout << "firmata v" << ard.getMajorFirmwareVersion() << "." << ard.getMinorFirmwareVersion() << endl;
    
    // Note: pins A0 - A5 can be used as digital input and output.
    // Refer to them as pins 14 - 19 if using StandardFirmata from Arduino 1.0.
    // If using Arduino 0022 or older, then use 16 - 21.
    // Firmata pin numbering changed in version 2.3 (which is included in Arduino 1.0)
    
    // set digital outputs
    ard.sendDigitalPinMode(X_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(X_STEP_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Y_DIR_PIN, ARD_OUTPUT);
    ard.sendDigitalPinMode(Y_STEP_PIN, ARD_OUTPUT);
    
    startX = startY = 0;
    speedX = speedY = 1;
}


//--------------------------------------------------------------
void ofApp::updateArduino(){
	// update the arduino, get any data or messages.
    // the call to ard.update() is required
	ard.update();
}


//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(0);
	
	ofSetColor(255);
	gray.draw(0, 0);
	cld.draw(0, 480);
	thresholded.draw(640, 0);
	thinned.draw(640, 480);
	
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
    
    // ARDUINO
    
	if (!bSetupArduino){
		cout << "arduino not ready..." << endl;
	}
    
    cout << "\n\n curState = " << curState << endl;
    
    // Draw the polylines on the coffee
    
    if (curState == PRINT) {
        for (paths_iter = paths.begin(); paths_iter < paths.end(); ++paths_iter) {
            cout << "\n\n\nPath " << paths_iter-paths.begin() << " / " << paths.size() << endl;
            vector<ofPoint> points = paths_iter->getVertices();
            for (points_iter = points.begin(); points_iter < points.end(); ++points_iter) { 
                if (points_iter == points.begin()) {
                    pushInk();
                } else if (points_iter == points.end()) {
                    stopInk();
                } else {
                    moveTo (points_iter->x, points_iter->y);
                }
            }
            if (paths_iter == paths.end()-1) {
                curState = COFFEE_PHOTO;
                cout << "\n\n\n\n\n"
                    "\n***************************************************************"
                    "\n************************ COFFEE_PHOTO *************************"
                    "\n***************************************************************"
                     << "\n\n\n\n\n" << endl;
            }
        }
    }
}


//--------------------------------------------------------------
void ofApp::moveTo (float exx, float wyy) {
    endX = exx;
    endY = wyy;
    stepsX = endX - startX;
    stepsY = endY - startY;
    
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
    moveStepper(0, stepsX*10, speedX);
    moveStepper(1, stepsY*10, speedY);
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
    float delay = (1/speed) / 2;
    
    for(int i=0; i < steps; i++){
        ard.sendDigital(STEP_PIN, ARD_HIGH);
        ofSleepMillis(delay);
        ard.sendDigital(STEP_PIN, ARD_LOW);
        ofSleepMillis(delay);
    }
}



//--------------------------------------------------------------
void ofApp::pushInk() {

}

//--------------------------------------------------------------
void ofApp::stopInk() {
    
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

    switch (key) {
        case ' ':
            needToUpdate = true;
            break;
        case '1':
            moveStepper(0, 100, 1);
            curState = ONE_KEY;
            break;
        case '2':
            moveStepper(0, -200, 1);
            curState = TWO_KEY;
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
