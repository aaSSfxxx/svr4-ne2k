#include "sys/ne2k.h"
#include "config.h"

struct ne2k_device devs[] = {
	{NE2K_0_VECT, NE2K_0_SIOA, NE2K_0_EIOA}
};

int num_devs = NE2K_UNITS;
