#include <cstdint>
#include <vector>

#include "posi.h"
#include "ymfm.h"
#include "ymfm_opl.h"

class chipInterface: public ymfm::ymfm_interface {
	public:
		chipInterface(int sampleRate = audioSampleRate);
		bool loadFile(std::vector<uint8_t>& file);
	private:
		ymfm::ymf262 chip;
		int sampleRate;
		std::vector<uint8_t>* file;
		uint32_t read32FromOffset(int offset);
};