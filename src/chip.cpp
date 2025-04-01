#include "chip.h"

#include <iostream>

#include "posi.h"

chipInterface::chipInterface(int sampleRate) {
	this->sampleRate = sampleRate;
	fmchip = fmsynth_new(sampleRate, 128);
}

chipInterface::~chipInterface() {
	fmsynth_free(fmchip);
}

bool chipInterface::loadFile(std::vector<uint8_t> file) {
	return false;
}


void chipInterface::generate(int16_t* buf, int numSamples) {
	leftBuffer.resize(numSamples);
	rightBuffer.resize(numSamples);
	for(auto i = 0; i<numSamples; i++) {
		leftBuffer[i] = int16ToFloat(buf[i*2]);
		rightBuffer[i] = int16ToFloat(buf[i*2+1]);
	}
	fmsynth_render(fmchip, leftBuffer.data(),rightBuffer.data(), numSamples);
	for(auto i = 0; i < numSamples; i++) {
		buf[i*2] = floatToInt16(leftBuffer[i]);
		buf[i*2+1] = floatToInt16(rightBuffer[i]);
	}
}