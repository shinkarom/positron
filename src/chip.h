#include <cstdint>
#include <vector>

#include "posi.h"
#include "thirdparty/opl3.h"

class chipInterface {
	public:
		chipInterface(int sampleRate = audioSampleRate);
		bool loadFile(std::vector<uint8_t> file);
		void generate(int16_t* buf, int numSamples = 1);
	private:
		opl3_chip chip;
		int sampleRate;
		std::vector<uint8_t> file;
		int dataOffset;
		int eofOffset;
		int filePos;
		int bufferPos;
		float delay;
		bool active;
		int one60;
		int one50;
		uint32_t read32FromOffset(int offset);
		void parseInstruction();
		void writeRegister(uint16_t reg, uint8_t value);
};