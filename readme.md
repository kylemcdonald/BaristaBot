# BaristaBot

This repository contains code for an upcoming installation at SXSW. It's built with [openFrameworks](http://www.openframeworks.cc/) and makes use of the following addons:

* [ofxCv](https://github.com/kylemcdonald/ofxCv)
* [ofxControlPanel](https://github.com/kylemcdonald/ofxControlPanel)

## CoherentLinePlanning

This app demonstrates how to extract a collection of paths (`vector<ofPolyline>`) from a webcam image in real time using coherent line drawing with thresholding and morphological thinning, followed by a slow technique for extracting connected components in a way that focuses on longer paths.