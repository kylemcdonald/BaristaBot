#include "ofApp.h"
#include "ofAppGlutWindow.h"

int main() {
	ofAppGlutWindow window;
	window.setGlutDisplayString("rgba double samples>=8 depth");
	ofSetupOpenGL(&window, 800, 800, OF_FULLSCREEN);
	ofRunApp(new ofApp());
}
