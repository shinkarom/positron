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

void chipInterface::reset() {
	fmsynth_reset(fmchip);
}


void chipInterface::generate(std::array<float, audioFramesPerTick> &lbuf, std::array<float, audioFramesPerTick> &rbuf) {
	fmsynth_render(fmchip, lbuf.data(),rbuf.data(), audioFramesPerTick);
}

void chipInterface::setOperatorParameter(uint8_t operatorNumber, uint8_t parameter,float value) {
	fmsynth_set_parameter(fmchip,parameter, operatorNumber, value);
}

float chipInterface::getOperatorParameter(uint8_t operatorNumber, uint8_t parameter) {
	return fmsynth_get_parameter(fmchip, parameter, operatorNumber);
}

void chipInterface::setGlobalParameter(uint8_t parameter, float value) {
	fmsynth_set_global_parameter(fmchip, parameter, value);
}

float chipInterface::getGlobalParameter(uint8_t parameter) {
	return fmsynth_get_global_parameter(fmchip, parameter);
}

void chipInterface::noteOn(uint8_t note, uint8_t velocity) {
	fmsynth_note_on(fmchip, note, velocity);
}

void chipInterface::noteOff(uint8_t note) {
	fmsynth_note_off(fmchip, note);
}

void chipInterface::setSustain(bool enable) {
	fmsynth_set_sustain(fmchip, enable);
}

void chipInterface::setModWheel(uint8_t wheel) {
	fmsynth_set_mod_wheel(fmchip, wheel);
}

void chipInterface::setPitchBend(uint16_t value) {
	fmsynth_set_pitch_bend(fmchip,value);
}

void chipInterface::releaseAll() {
	fmsynth_release_all(fmchip);
}