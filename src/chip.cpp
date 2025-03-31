#include "chip.h"

#include <iostream>

chipInterface::chipInterface(int sampleRate) : chip(*this) {
	this->sampleRate = sampleRate;
}

bool chipInterface::loadFile(std::vector<uint8_t>& file) {
	if(!(file[0] == 'V' && file[1] == 'g' && file[2] == 'm' && file[3] == ' ')) {
		return false;
	}
	this->file = &file;
	auto offset = read32FromOffset(0x34);
	std::cout<<"offset "<<offset<<std::endl;
	dataOffset = offset+0x34;
	auto eof = read32FromOffset(0x04);
	std::cout<<"eof "<<eof<<std::endl;
	eofOffset = eof+0x04;
	auto clock = read32FromOffset(0x5C);
	std::cout<<"clock "<<clock<<std::endl;
	auto rate = chip.sample_rate(clock);
	chipStep = rate / (sampleRate*1.0);
	std::cout<<"step "<<chipStep<<std::endl;
	return true;
}

uint32_t chipInterface::read32FromOffset(int offset) {
	uint8_t x1 = file->at(offset);
	uint8_t x2 = file->at(offset+1);
	uint8_t x3 = file->at(offset+2);
	uint8_t x4 = file->at(offset+3);
	return (x4<<24) | (x3<<16) | (x2 <<8) | x1;
}

void chipInterface::generate(int16_t* buf, int numSamples, bool overdub) {
	generateNFrames(buf, numSamples, overdub);
}

int16_t* chipInterface::generateNFrames(int16_t* buf, int numSamples, bool overdub) {
	int16_t tmpBuf[2];
	auto bufPos = buf;
	for(auto i = 0; i < numSamples; i++) {
		generateOneFrame(tmpBuf);
		if(!overdub) {
			*bufPos = tmpBuf[0];
			*(bufPos+1) = tmpBuf[1];
		} else {
			auto b1 = *bufPos;
			auto b2 = *(bufPos+1);
			b1 += tmpBuf[0];
			b2 += tmpBuf[1];
			if(b1 < INT16_MIN) {
				b1 = 0;
			}
			if(b1 > INT16_MAX) {
				b1 = INT16_MAX;
			}
			if(b2 < INT16_MIN) {
				b2 = 0;
			}
			if(b2 > INT16_MAX) {
				b2 = INT16_MAX;
			}
			*bufPos = b1;
			*(bufPos+1) = b2;
		}
		
		bufPos += 2;
	}
	return bufPos;
}

void chipInterface::generateOneFrame(int16_t* buf) {
	ymfm::ymf262::output_data tmpBuf;
	chip.generate(&tmpBuf);
	*buf = tmpBuf.data[0];
	*(buf+1) = tmpBuf.data[1];
}