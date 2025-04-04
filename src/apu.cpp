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

void posiAPISetOperatorParameter(int channelNumber, uint8_t operatorNumber, uint8_t parameter,float value) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return;
	}
	chips[channelNumber].setOperatorParameter(operatorNumber, parameter, value);
}

float posiAPIGetOperatorParameter(int channelNumber, uint8_t operatorNumber, uint8_t parameter) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return 0.0;
	}
	return chips[channelNumber].getOperatorParameter(operatorNumber, parameter);
}

float posiAPIFromNormalizedOperatorParameter(int channelNumber,uint8_t parameter, float value) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return 0.0;
	}
	return chips[channelNumber].fromNormalizedOperatorParameter(parameter, value);
}

float posiAPIToNormalizedOperatorParameter(int channelNumber,uint8_t parameter, float value) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return 0.0;
	}
	return chips[channelNumber].toNormalizedOperatorParameter(parameter, value);
}

void posiAPISetGlobalParameter(int channelNumber,uint8_t parameter, float value) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return;
	}
	chips[channelNumber].setGlobalParameter(parameter, value);
}

float posiAPIGetGlobalParameter(int channelNumber,uint8_t parameter) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return 0.0;
	}
	return chips[channelNumber].getGlobalParameter(parameter);
}

float posiAPIFromNormalizedGlobalParameter(int channelNumber,uint8_t parameter, float value) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return 0.0;
	}
	return chips[channelNumber].fromNormalizedGlobalParameter(parameter, value);
}

float posiAPIToNormalizedGlobalParameter(int channelNumber,uint8_t parameter, float value) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return 0.0;
	}
	return chips[channelNumber].toNormalizedGlobalParameter(parameter, value);
}

void posiAPINoteOn(int channelNumber,uint8_t note, uint8_t velocity) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return;
	}
	chips[channelNumber].noteOn(note, velocity);
}

void posiAPINoteOff(int channelNumber,uint8_t note) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return;
	}
	chips[channelNumber].noteOff(note);
}

void posiAPISetSustain(int channelNumber,bool enable) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return;
	}
	chips[channelNumber].setSustain(enable);
}

void posiAPISetModWheel(int channelNumber,uint8_t wheel) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return;
	}
	chips[channelNumber].setModWheel(wheel);
}

void posiAPISetPitchBend(int channelNumber,uint16_t value) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return;
	}
	chips[channelNumber].setPitchBend(value);
}

void posiAPIReleaseAll(int channelNumber) {
	if(channelNumber < 0 || channelNumber >= numAudioChannels) {
		return;
	}
	chips[channelNumber].releaseAll();
}