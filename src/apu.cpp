#include "posi.h"

#include <array>
#include <iostream>

#include "thirdparty/m4p.h"
#include "thirdparty/tsf.h"
#include "thirdparty/tml.h"

std::array<int16_t, audioFramesPerTick*2> soundBuffer;

bool playing = false;

int16_t* posiAudiofeed() {
	return soundBuffer.data();
}

void apuInit() {
	apuReset();
}

void apuProcess() {
	if(playing) {
		m4p_GenerateSamples(soundBuffer.data(), audioFramesPerTick);
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
	m4p_LoadFromData(x->data(), x->size(), audioSampleRate, audioFramesPerTick);
	m4p_PlaySong();
	playing = true;
}

void posiAPITrackStop() {
	m4p_Stop();
	playing = false;
	apuReset();
}