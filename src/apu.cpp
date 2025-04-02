#include "posi.h"

#include <array>
#include <iostream>

#include "chip.h"

std::array<int16_t, audioFramesPerTick*2> soundBuffer;
chipInterface chips[numAudioChannels];

bool playing = false;

int16_t* posiAudiofeed() {
	return soundBuffer.data();
}

void apuInit() {
	apuReset();
}

void apuProcess() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
	for(int i = 0; i<numAudioChannels; i++) {
		chips[i].generate(soundBuffer.data(),audioFramesPerTick);
	}
}

void apuReset() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
	for(int i = 0; i<numAudioChannels; i++) {
		chips[i].reset();
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

void posiAPISetOperatorParameter(int channelNumber, uint8_t operatorNumber, uint8_t parameter,float value) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return;
	}
	chips[channelNumber].setOperatorParameter(operatorNumber, parameter, value);
}

float posiAPIgetOperatorParameter(int channelNumber, uint8_t operatorNumber, uint8_t parameter) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return 0.0;
	}
	return chips[channelNumber].getOperatorParameter(operatorNumber, parameter);
}