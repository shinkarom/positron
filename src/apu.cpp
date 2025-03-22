#include "posi.h"

#include <array>
#include <iostream>

std::array<int16_t, audioFramesPerTick*2> soundBuffer;


int16_t* posiAudiofeed() {
	return soundBuffer.data();
}

void apuInit() {
	apuReset();
}

void apuProcess() {
	
}

void apuReset() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
}