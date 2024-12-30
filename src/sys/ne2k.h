#ifndef NE2K_H
#define NE2K_H

/* Structure to store internal state of the physical device, which
   can have several handles opened to it (thanks to STREAMS API)
 */
struct ne2k_device {
	int irq;
	int io_base;
	int io_high;
	int devmajor;	
	int n_minors;	
	char is_valid;
	int fhandle_idx;
	unsigned char curpage;
};


/* Structure to describe an handle opened for a device and store
   anything about it (STREAMS queue, device minor number, ...)
*/
struct ne2k_handle {
	struct ne2k_device *dev;
	queue_t *queue;
};

#endif
