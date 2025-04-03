#include <cstdint>
#include <vector>

#include "posi.h"
#include "fmsynth.h"

class chipInterface {
	public:
		chipInterface(int sampleRate = audioSampleRate);
		~chipInterface();
		bool loadFile(std::vector<uint8_t> file);
		void generate(int16_t* buf, int numSamples = 1);
		void setOperatorParameter(uint8_t operatorNumber, uint8_t parameter,float value);
		float getOperatorParameter(uint8_t operatorNumber, uint8_t parameter);
		void setGlobalParameter(uint8_t parameter, float value);
		float getGlobalParameter(uint8_t parameter);
		void noteOn(uint8_t note, uint8_t velocity);
		void noteOff(uint8_t note);
		void setSustain(bool enable);
		void setModWheel(uint8_t wheel);
		void setPitchBend(uint16_t value);
		void releaseAll();
		void reset();
	private:
		fmsynth* fmchip;
		std::vector<float> leftBuffer, rightBuffer;
		int sampleRate;
};