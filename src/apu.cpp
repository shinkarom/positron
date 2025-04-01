#include "posi.h"

#include <array>
#include <iostream>

#include "chip.h"

std::array<int16_t, audioFramesPerTick*2> soundBuffer;
chipInterface chips[2];

bool playing = false;

int16_t* posiAudiofeed() {
	return soundBuffer.data();
}

void apuInit() {
	apuReset();
}

void apuProcess() {
	if(playing) {
		chips[0].generate(soundBuffer.data(),audioFramesPerTick);
	}
}

void apuReset() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
}

void posiAPITrackPlay(std::string trackName) {
	auto x = dbLoadByName("track",trackName);
	if(!x) return;
	if(chips[0].loadFile((*x))){
		playing = true;
		std::cout<<"Loaded file "<<trackName<<std::endl;
	}
	
}

void posiAPITrackStop() {
	playing = false;
	apuReset();
}