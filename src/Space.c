#include <sys/stream.h>
#include "sys/ne2k.h"
#include "config.h"

#define NSTR 32

/* Must match value in Master file */
int max_devices = NSTR;
int num_devs = NE2K_CNTLS;

struct ne2k_device devs[] = {
	{NE2K_0_VECT, NE2K_0_SIOA, NE2K_0_EIOA}
};


struct ne2k_handle handles[NSTR * NE2K_CNTLS];

