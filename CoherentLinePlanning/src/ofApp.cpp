#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

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

void ofApp::setup() {
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
	ofEnableSmoothing();
	
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
	gui.addSlider("minPathLength", 0, 0, 50, true);
	gui.loadSettings("settings.xml");
}

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
	}
}

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
}

void ofApp::keyPressed(int key) {
	if(key == ' ') {
		needToUpdate = true;
	}
}
