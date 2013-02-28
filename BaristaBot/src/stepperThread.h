#ifndef _STEPPER_THREAD
#define _STEPPER_THREAD

#include "ofMain.h"

// this is not a very exciting example yet
// but ofThread provides the basis for ofNetwork and other
// operations that require threading.
//
// please be careful - threading problems are notoriously hard
// to debug and working with threads can be quite difficult


class stepperThread : public ofThread{

	public:


	    int count;  // threaded fucntions that share data need to use lock (mutex)
	                // and unlock in order to write to that data
	                // otherwise it's possible to get crashes.
	                //
	                // also no opengl specific stuff will work in a thread...
	                // threads can't create textures, or draw stuff on the screen
	                // since opengl is single thread safe


        void start();
        void stop();
        void threadedFunction();
        void draw();
};

#endif
