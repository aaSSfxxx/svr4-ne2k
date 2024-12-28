#ifndef TOTO_H
#define TOTO_H

#define NE2K_INVALID 0
#define NE2K_VALID 1

struct ne2k_device {
	int irq;
	int io_base;
	int io_high;
	char is_valid;
	unsigned char curpage;
};

#endif
