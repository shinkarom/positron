#include "chip.h"

#include <iostream>

chipInterface::chipInterface(int sampleRate) {
	this->sampleRate = sampleRate;
	OPL3_Reset(&chip,sampleRate);
	writeRegister(0x105,0x01);
	one60 = sampleRate / 60;
	one50 = sampleRate / 50;
}

bool chipInterface::loadFile(std::vector<uint8_t> file) {
	if(!(file[0] == 'V' && file[1] == 'g' && file[2] == 'm' && file[3] == ' ')) {
		return false;
	}
	this->file = file;
	auto offset = read32FromOffset(0x34);
	dataOffset = offset+0x34;
	filePos = dataOffset;
	auto eof = read32FromOffset(0x04);
	eofOffset = eof+0x04;

	active = true;
	
	return true;
}

uint32_t chipInterface::read32FromOffset(int offset) {
	uint8_t x1 = file[offset];
	uint8_t x2 = file[offset+1];
	uint8_t x3 = file[offset+2];
	uint8_t x4 = file[offset+3];
	return (x4<<24) | (x3<<16) | (x2 <<8) | x1;
}

void chipInterface::generate(int16_t* buf, int numSamples) {
	if(!active) return;
	bufferPos = 0;
	//std::cout<<"new frame delay "<<delay<<std::endl;
	while(bufferPos < numSamples) {
		if(delay) {
			int rem = numSamples - bufferPos;
			int r = delay > rem ? rem : delay;
			OPL3_GenerateStream(&chip,&buf[bufferPos], r);
			//std::cout<<"generating "<<r<<std::endl;
			delay -= r;
			bufferPos += r * 2;
		} else {
			parseInstruction();
		}
		
	}
}

void chipInterface::writeRegister(uint16_t reg, uint8_t value) {
	OPL3_WriteReg(&chip, reg,value);
	//std::cout<<std::hex<<"written "<<int(value)<<" to "<<reg<<std::dec<<std::endl;
}

void chipInterface::parseInstruction() {
	uint8_t instr = file[filePos];
	//std::cout<<std::hex<<filePos<<" "<<int(instr)<<std::dec<<std::endl;
	switch(instr) {
		case 0x5e: {
			auto reg = file[filePos+1];
			auto val = file[filePos+2];
			writeRegister(reg, val);
			filePos+=3;
			break;
		}
		case 0x5f: {
			auto reg = file[filePos+1];
			auto val = file[filePos+2];
			writeRegister(reg|0x100, val);
			filePos+=3;
			break;
		}
		case 0x61: {
			uint16_t d = (file[filePos+2]<<8)|file[filePos+1];
			d = static_cast<uint16_t>(d * sampleRate / 44100.0);
			delay += d;
			//std::cout<<"set delay to "<<delay<<" "<<d<<std::endl;
			filePos+=3;
			break;
		}
		case 0x62: {
			delay += one60;
			//std::cout<<"set delay to "<<delay<<std::endl;
			filePos++;
			break;
		}
		case 0x63: {
			delay += one50;
			//std::cout<<"set delay to "<<delay<<std::endl;
			filePos++;
			break;
		}
		case 0x66:
			active = false;
			break;
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7E:
		case 0x7F: {
			auto d = instr - 0x70 + 1;
			d = static_cast<uint16_t>(d * sampleRate / 44100.0);
			delay += d;
			std::cout<<"set delay to "<<delay<<" "<<d<<std::endl;
			filePos++;
			break;
		}
		default:
			std::cout<<"Unknown instruction at "<<std::hex<<filePos<<": "<<int(instr)<<std::dec<<std::endl;
			break;
	}
}