#include <cstdint>

#include "ymfm.h"
#include "ymfm_opl.h"

class chipInterface: public ymfm::ymfm_interface {
	public:
		chipInterface();
	private:
		ymfm::ymf262 chip;
};