#pragma once
#include <rack.hpp>

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelDatawave;
extern Model* modelChronos;
extern Model* modelSyn;

extern bool _SYNC;
extern int _BPM;
extern bool _RUN;