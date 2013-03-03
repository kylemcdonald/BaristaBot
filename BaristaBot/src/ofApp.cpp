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
	gui.addSlider("black", 65, -255, 255, true);        // kyle's 0
	gui.addSlider("sigma1", 0.85, 0.01, 2.0, false);    // kyle's 0.4
	gui.addSlider("sigma2", 4.45, 0.01, 10.0, false);   // kyle's 3.0
	gui.addSlider("tau", 0.97, 0.8, 1.0, false);        // kyle's 0.97
	gui.addSlider("halfw", 4, 1, 8, true);              // kyle's 4
	gui.addSlider("smoothPasses", 3, 1, 4, true);       // kyle's 2
	gui.addSlider("thresh", 121.8, 0, 255, false);      // kyle's 128
	gui.addSlider("minGapLength", 6.8, 2, 12, false);   // kyle's 2
	gui.addSlider("minPathLength", 20, 0, 50, true);    // kyle's 20
	gui.addSlider("facePadding", 1.77, 0, 2, false);    // kyle's 1.5
	gui.loadSettings("settings.xml");
    
    AT.setup();
    AT.cropped_size = croppedSize;
    AT.start();
}


//--------------------------------------------------------------
void ofApp::update(){
	cam.update();
	if(cam.isFrameNew() && needToUpdate) {
		int black = gui.getValueI("black");
		float sigma1 = gui.getValueF("sigma1");
		float sigma2 = gui.getValueF("sigma2");
		float tau = gui.getValueF("tau");
        float thresh = gui.getValueF("thresh");
		int halfw = gui.getValueI("halfw");
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
        
        // get arduino thread loaded with the paths and started
        AT.lock();
            AT.paths = paths;
            AT.points = paths.begin()->getVertices();
		AT.unlock();
    }
    AT.update();
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
    
    AT.draw();
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {	
    switch (key) {
        case 'n':
            // if coffee photo is good, press n
            // sends the arm up to start over, ready for a new person
            AT.shootFace();
            break;
        case ' ':
            // when the arm is up press space to take a face photo
            // press again until you like it
            needToUpdate = true;
            break;
        case 'p':
            // once you have a good face photo, press p
            // machine will lower and go home then print starts automatically
            // after print machine raises up and takes a coffee photo
            AT.curState = AT.FACE_PHOTO;
            AT.goHome();
            break;
        case 'c':
            // take a look at the coffee photo, if it's not good press c
            // retake photo of coffee (or make this happen in spacebar for consistency)
            break;
            
        // special controls
        case 'h':
            // for debugging
            AT.goHome();
            break;
        case 'b':
//            AT.drawBox();
            break;
        case OF_KEY_RIGHT:
            AT.jogRight();
            break;
        case OF_KEY_LEFT:
            AT.jogLeft();
            break;
        case OF_KEY_UP:
            AT.jogForward();
            break;
        case OF_KEY_DOWN:
            AT.jogBack();
            break;
        case OF_KEY_HOME:
            AT.jogUp();
            break;
        case OF_KEY_END:
            AT.jogDown();
            break;
        case OF_KEY_PAGE_UP:
            AT.plungerUp();
            break;
        case OF_KEY_PAGE_DOWN:
            AT.plungerDown();
            break;
        default:
            break;
    }
}

void ofApp::exit() {
    AT.stop();
    ofSleepMillis(1000);
}




