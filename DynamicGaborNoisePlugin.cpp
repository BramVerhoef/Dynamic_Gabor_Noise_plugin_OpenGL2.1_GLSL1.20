/*
 *  DynamicGaborNoisePlugin.cpp
 *  DynamicGaborNoise
 *
 *  Created by Bram-Ernst Verhoef on 9/01/14.
 *  Copyright 2014 University of Chicago. All rights reserved.
 *
 */



#include "DynamicGaborNoise.h"


class DynamicGaborNoisePlugin : public Plugin {
	void registerComponents(shared_ptr<ComponentRegistry> registry) MW_OVERRIDE {
        registry->registerFactory<StandardStimulusFactory, DynamicGaborNoise>();
    }	
};


MW_SYMBOL_PUBLIC
extern "C" Plugin* getPlugin() {
    return new DynamicGaborNoisePlugin();
}

