#include "posi.h"

#include <array>

#include "thirdparty/opl3.h"

std::array<int16_t, audioFramesPerTick*2> soundBuffer;

opl3_chip chip;

int16_t* posiAudiofeed() {
	return soundBuffer.data();
}

void apuInit() {
	OPL3_Reset(&chip, audioSampleRate);
}

void apuProcess() {
	OPL3_GenerateStream(&chip, soundBuffer.data(), audioFramesPerTick);
}

void apuReset() {
	for(int i = 0; i < audioFramesPerTick*2; i++) {
		soundBuffer[i] = 0;
	}
}