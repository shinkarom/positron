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
	auto filelen = read32FromOffset(0x04);
	std::cout<<"length "<<filelen<<std::endl;
	auto clock = read32FromOffset(0x5C);
	std::cout<<"clock "<<clock<<std::endl;
	auto clock = read32FromOffset(0x5C);
	std::cout<<"clock "<<clock<<std::endl;
	return true;
}

uint32_t chipInterface::read32FromOffset(int offset) {
	uint8_t x1 = file->at(offset);
	uint8_t x2 = file->at(offset+1);
	uint8_t x3 = file->at(offset+2);
	uint8_t x4 = file->at(offset+3);
	return (x4<<24) | (x3<<16) | (x2 <<8) | x1;
}