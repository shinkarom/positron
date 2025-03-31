#include <cstdint>
#include <vector>

#include "posi.h"
#include "ymfm.h"
#include "ymfm_opl.h"

class chipInterface: public ymfm::ymfm_interface {
	public:
		chipInterface(int sampleRate = audioSampleRate);
		bool loadFile(std::vector<uint8_t>& file);
		void generate(int16_t* buf, int numSamples = 1, bool overdub = false);
	private:
		ymfm::ymf262 chip;
		int sampleRate;
		std::vector<uint8_t>* file;
		int dataOffset;
		int eofOffset;
		int filePos;
		float chipStep;
		float chipPos;
		uint32_t read32FromOffset(int offset);
		void generateOneFrame(int16_t* buf);
		int16_t* generateNFrames(int16_t* buf, int numSamples = 1, bool overdub = false);
};