# BaristaBot

This repository contains code for an upcoming installation at SXSW. It's built with [openFrameworks](http://www.openframeworks.cc/) and makes use of the following addons:

* [ofxCv](https://github.com/kylemcdonald/ofxCv)
* [ofxControlPanel](https://github.com/kylemcdonald/ofxControlPanel)

## CoherentLinePlanning

This app demonstrates how to extract a collection of paths (`vector<ofPolyline>`) from a webcam image in real time (every time you hit the space bar) using coherent line drawing with thresholding and morphological thinning, followed by a slow technique for extracting connected components in a way that focuses on longer paths.

Play a bit with the parameters for controlling coherent line drawing: black, sigma1, sigma2, tau, halfw, smoothPasses. Once you get these set up for a given lighting environment they should be fairly fixed and not require any tweaking for different people.

The thresh setting is post-CLD but is an important number to tweak before the results are thinned. After thinning, a collection of paths is generated using the minGapLength and minPathLength. minGapLength determines how big a jump is between two pixels before it's considered a "gap" (i.e., two disconnected lines). minPathLength determines how many pixels a path must contain in order to be included in the final collection. This helps remove very short paths (noise).

## StepperTest

This app is based on firmataExample. It's modified to control a stepper motor via arduino and the EasyShield stepper driver. 
