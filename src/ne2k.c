#include <sys/types.h>
#include <sys/param.h>
#include <sys/cmn_err.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/ddi.h>
#include <sys/socket.h>
#include <sys/dlpi.h>
#include <sys/errno.h>
#include <net/if.h>
#include <net/strioc.h>
#include "sys/ne2k.h"

extern struct ne2k_device devs[];
extern struct ne2k_handle handles[];
extern int num_devs;

extern unsigned char inb(unsigned int addr);
extern void outb(unsigned int addr, unsigned char val);
extern void bcopy(void*, void*, int);

int ne2kdevflag = D_NEW;

#ifdef NE2KDEBUG
#define DBGPRINT(x) printf x
#else
#define DBGPRINT(x)
#endif

/* Read I/O register addresses */
#define COMMAND 0
#define CLDA0 	1
#define CLDA1 	2
#define BNRY  	3
#define TSR   	4
#define NCR   	5
#define ISR		7
#define CRDA0	8
#define CRDA1	9
#define RSR		12
#define DCR		14
#define IMR		15
#define RESET	0x1f

/* Write I/O register addresses */
#define STARTPG	1
#define ENDPG	2
#define BNDRY	3
#define TPSR	4
#define TCNTLO	5
#define	TCNTHI	6
#define RSARLO	8
#define RSARHI	9
#define RCNTLO	10
#define RCNTHI	11
#define RXCR	12
#define TXCR	13

#define PAGE1_PHYS   1
#define PAGE1_CURPAG 7
#define PAGE1_MULT   8

/* Control register commands */
#define CMD_STOP		1
#define CMD_START 		2
#define CMD_TRANSMIT		4
#define CMD_RREAD		8
#define CMD_RWRITE		16
#define CMD_NODMA		32
#define CMD_PAGE0		0 
#define CMD_PAGE1		64
#define CMD_PAGE2		128

/* Interrupt status code */
#define INTR_RX			1
#define INTR_TX			2
#define INTR_RX_ERR		4
#define	INTR_TX_ERR		8
#define INTR_OVER		16
#define	INTR_COUNTERS	32
#define INTR_RDC		64
#define INTR_RESET		128

#define RBUF_STARTPG	70
#define RBUF_ENDPG		96

#define PKTSZ	(3*256)
#define HIWAT		(32*PKTSZ)
#define LOWAT		(8*PKTSZ)
#define MAXPKT	1500

static char* hexchrs[] = {
	"0","1","2","3","4","5","6","7","8",
	"9","a","b","c","d","e","f"
};


/* STREAMS queue definition */
static struct module_info minfo[DRVR_INFO_SZ] = {
     ENETM_ID, "ne2k", 0, MAXPKT, HIWAT, LOWAT,
     ENETM_ID, "ne2k", 0, MAXPKT, HIWAT, LOWAT,
     ENETM_ID, "ne2k", 0, MAXPKT, HIWAT, LOWAT,
  };

int queueopen(), queueclose(), queuewput(), queuewsrv(), queuersrv();

static struct qinit read_queue = {
	NULL, queuersrv, queueopen, queueclose, NULL, &minfo[IQP_RQ], NULL,
};

static struct qinit write_queue = {
	queuewput, queuewsrv, NULL, NULL, NULL, &minfo[IQP_WQ], NULL,
};

struct streamtab ne2kinfo = { &read_queue, &write_queue, NULL, NULL };

static struct ne2k_device *find_device(irq)
int irq;
{
	int i;
	for(i = 0; i < num_devs; i++) {
		if(devs[i].irq == irq) {
			return devs + i;
		}
	}
	return (void*)0;
}

static print_macaddr(s)
char s[];
{
	int i;
	for(i = 0; i < 6; i++) {
		printf("%s%s%s", hexchrs[s[2*i] >> 4 & 0xf], hexchrs[s[2*i] & 0xf],
			 (i != 5) ? ":" : "");
	}
	printf("\n");
}

ne2kinit() {
	int i, j;
	int iobase;
	unsigned char prom[32];

	for(i = 0; i < num_devs; i++) {
		DBGPRINT(("ne2k: Device configured at irq %d io %x, major %d\n", 
			devs[i].irq, devs[i].io_base, devs[i].devmajor));
		iobase = devs[i].io_base;
	
		/* Perform a card reset */
		outb(iobase+RESET, inb(iobase+RESET));
		while(inb(iobase+ISR) & 0x80 == 0);

		/* Mask interrupts */
		outb(iobase+ISR, 0xff);

		/* Stop the card and configure read address buffer and size */
		outb(iobase, CMD_PAGE0 | CMD_STOP | CMD_NODMA);
		outb(iobase+DCR, 0x58);
		outb(iobase+RSARHI, 0);
		outb(iobase+RSARLO, 0);
		outb(iobase+RCNTHI, 0);
		outb(iobase+RCNTLO, 32);

		/* Start again the card and read the EEPROM data */
		outb(iobase, CMD_PAGE0 | CMD_START | CMD_RREAD | CMD_NODMA);
		for(j = 0; j < 32; j++) {
			prom[j] = inb(iobase+0x10);
		}
		if(prom[28] != 0x57 || prom[30] != 0x57) {
			cmn_err(CE_WARN, "ne2k: Invalid NE2000 I/O address");
			devs[i].is_valid = 0;
			continue;
		}
		devs[i].is_valid = 1;
		devs[i].fhandle_idx = i * devs[i].n_minors;
		printf("ne2k: card MAC address is ");
		print_macaddr(prom); 
	
		/* Set ring buffer page offsets */
		devs[i].curpage = RBUF_STARTPG + 1;
		outb(iobase+STARTPG, RBUF_STARTPG);
		outb(iobase+BNDRY, RBUF_STARTPG);
		outb(iobase+ENDPG, RBUF_ENDPG);

		/* Configure TX and RX parameters */
		outb(iobase+RXCR, 0x6);
		outb(iobase+TXCR, 4);

		/* Configure interrupt mask */
		outb(iobase+IMR, 0x1f);
		/* Switch to page 1 and write MAC address */
		outb(iobase, CMD_PAGE1 | CMD_STOP | CMD_NODMA);
		for(j = 0; j < 6; j++) {
			outb(iobase + PAGE1_PHYS + j, prom[2*j]);
		}
		for(j = 0; j < 6; j++) {
			outb(iobase + PAGE1_MULT + j, 0xff);
		}
	
		outb(iobase + PAGE1_CURPAG, devs[i].curpage);

		/* Switch to page 0 and configure parameters */
		outb(iobase, CMD_START | CMD_PAGE0 | CMD_NODMA);

	}
}

void read_packet(struct ne2k_device *dev, int len) {
	mblk_t *mb;
	int iobase, i, count;
	struct ne2k_handle *h;
	char buf[2048];

	DBGPRINT(("Get packet of %d bytes\n", len));
	if(len > 2048) {
		cmn_err(CE_WARN, "Packet too big (%d bytes) received !", len);
		return;
	}
	iobase = dev->io_base;
	outb(iobase, CMD_PAGE0 | CMD_START | CMD_NODMA);
	outb(iobase + RSARHI, dev->curpage);
	outb(iobase + RSARLO, 4);
	outb(iobase + RCNTHI, len >> 8);
	outb(iobase + RCNTLO, len & 0xff);
	outb(iobase, CMD_PAGE0 | CMD_START | CMD_NODMA | CMD_RREAD);
	for(i = 0; i < len; i++) {
		buf[i] = inb(iobase + 0x10);
	}
	count = 0;
	for(i = 0; i < dev->n_minors; i++) {
		h = &handles[dev->fhandle_idx + i];
		if(h->queue == NULL) continue;
		DBGPRINT(("ne2k: Found queue at index %d\n", i));
		
		mb = allocb(len, BPRI_MED);
		if(mb == NULL) {
			cmn_err(CE_WARN, "ne2k: Not enough memory to allocate transmit buffer");
			return;
		}
		bcopy(buf, mb->b_wptr, len);
		mb->b_wptr += len;
		/* Enque the message to be processed by the service queue */	
		putq(h->queue, mb);
		count++;
	}
}

void handle_rx_pkt(struct ne2k_device *dev) {
	unsigned int iobase, len, boundary;
	unsigned char curpage, rsr, next, flag;

	iobase = dev->io_base;
	/* Read current page from buffer ring */
	outb(iobase, CMD_PAGE1 | CMD_START | CMD_NODMA);
	curpage = inb(iobase + PAGE1_CURPAG);

	while(dev->curpage != curpage) {
		/* Return to page 0 and read data from buffer ring */
		outb(iobase, CMD_PAGE0 | CMD_START | CMD_NODMA);
		outb(iobase + RSARHI, dev->curpage);
		outb(iobase + RSARLO, 0);
		outb(iobase + RCNTHI, 0);
		outb(iobase + RCNTLO, 4);
		outb(iobase, CMD_PAGE0 | CMD_START | CMD_NODMA | CMD_RREAD);
		rsr = inb(iobase + 0x10);
		next = inb(iobase + 0x10);
		len = inb(iobase + 0x10) + (inb(iobase + 0x10) << 8);
		DBGPRINT(("ne2k: rsr = %x cur = %d next = %d len = %d, "
			"boundary = %d, thiscur = %d\n",
			rsr, curpage, next, len-4, inb(iobase + BNDRY),
			dev->curpage));
		if(next == 0xff) {
			cmn_err(CE_WARN, "ne2k: Invalid next ! Marking card invalid\n");
			dev->is_valid = 0;
			break;
		}
		read_packet(dev, len - 4);
		dev->curpage = next;
	
		/* Read current page from buffer ring */
		outb(iobase, CMD_PAGE1 | CMD_START | CMD_NODMA);
		curpage = inb(iobase + PAGE1_CURPAG);
	}
	/* Compute new boundary and cur page */
	boundary = dev->curpage;
	outb(iobase, CMD_PAGE0 | CMD_START | CMD_NODMA);
	outb(iobase + BNDRY, boundary);
}

ne2kintr(irq)
int irq;
{
	struct ne2k_device *dev;
	unsigned char c;

	/* Find the device given the IRQ number */
	dev = find_device(irq);
	if(!dev || !dev->is_valid) {
		cmn_err(CE_WARN, "ne2k: Invalid device for IRQ %d", irq);
		return;
	}
	/* Switch to page 0 */
	outb(dev->io_base, CMD_PAGE0 | CMD_RREAD);

	/* Read the interrupt status register */
	c = inb(dev->io_base + ISR);
	outb(dev->io_base + ISR, 0xff);
	switch(c & 0xf) {
	case INTR_RX:
		handle_rx_pkt(dev);
		break;
	default:
		DBGPRINT(("ne2k: interrupted, status reg %x\n", c));
	}
}

/* Called by STREAMS to open a new device for the board */
int
queueopen(q, dev, flag, sflag, credp)
queue_t *q;
dev_t *dev;
int flag, sflag;
struct cred *credp;
{
	major_t devmajor;
	minor_t devminor;
	int i, oldlevel;
	struct ne2k_device *d_dev;

	/* Get major number for opened device and check it belongs to our
	   driver */
	DBGPRINT(("queueopen(%x %x %x %x %x) called\n", q, dev, flag, sflag, credp));
	devmajor = getemajor(*dev);
	d_dev = NULL;
	for(i = 0; i < num_devs; i++) {
		if(devs[i].devmajor == devmajor) {
			d_dev = devs + i;
			break;
		}	
	}

	if(d_dev == NULL) {
		cmn_err(CE_WARN, "ne2k: trying to open major that doesn't exist");
		return ECHRNG;
	}

	/* Disable interrupts to open the device */
	oldlevel = splstr();

	if(sflag & CLONEOPEN) {
		for(devminor = 0; devminor < d_dev->n_minors; devminor++) {
			if(handles[d_dev->fhandle_idx + devminor].queue == NULL) {
				break;	
			}
		}
	}
	else {
		devminor = geteminor(*dev);
	}

	/* If the queue is already opened, return the device */
	if(q->q_ptr) {
		DBGPRINT(("Queue already opened\n"));
		*dev = makedevice(devmajor, devminor);
		splx(oldlevel);
		return 0;
	}

	/* Save current handle information into queue information */
	q->q_ptr = &handles[d_dev->fhandle_idx + devminor];
	WR(q)->q_ptr = q->q_ptr;

	handles[d_dev->fhandle_idx + devminor].dev = d_dev;
	handles[d_dev->fhandle_idx + devminor].queue = q;

	splx(oldlevel);
	*dev = makedevice(devmajor, devminor);
	return 0;
}

/* Closes a handle */
queueclose(q)
queue_t *q;
{
	struct ne2k_handle *handle;
	int i;

	DBGPRINT(("ne2k: closing driver\n"));
	handle = (struct ne2k_handle*)q->q_ptr;
	handle->queue = NULL;
	q->q_ptr = NULL;
}

queuersrv(q)
queue_t *q;
{
	mblk_t *blk, *ctl_blk;
	register union DL_primitives *dlp;

	DBGPRINT(("Read service queue called\n"));

	/* Process enqued messages we got from the network card from the interrupt
	 * We need to add a LLC control message in order for the packet to be
	 * processed by the upper IP layer */
	while((blk = getq(q)) != NULL)
		if(canput(q->q_next)) {
		ctl_blk = allocb(DL_UNITDATA_IND_SIZE + 2*6, BPRI_MED);
		if(!ctl_blk) {
			cmn_err(CE_WARN, "Couldn't allocate buffer for control packet");
			continue;
		}
		ctl_blk->b_datap->db_type = M_PROTO;
		dlp = (union DL_primitives*)ctl_blk->b_rptr;
		
		/*  Create a DL_UNITDATA_IND message, with control part containing the
		 *  source and destination MAC addresses. IP stack does not seem to
		 *  make use of it in SVR4 sources but control message seems to be
		 *  required anyways 
		 */	
		dlp->unitdata_ind.dl_primitive = DL_UNITDATA_IND;
		dlp->unitdata_ind.dl_dest_addr_length = 6;
		dlp->unitdata_ind.dl_src_addr_length = 6;
		dlp->unitdata_ind.dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;
		dlp->unitdata_ind.dl_src_addr_offset = DL_UNITDATA_IND_SIZE + 6;
		bcopy(blk->b_rptr, ctl_blk->b_rptr + DL_UNITDATA_IND_SIZE, 6);
		bcopy(blk->b_rptr + 6, ctl_blk->b_rptr + DL_UNITDATA_IND_SIZE + 6, 6);
		linkb(ctl_blk, blk);	
		putnext(q, ctl_blk);	
	}
}

queuewsrv(q)
queue_t *q;
{
	printf("Write service queue called\n");
}

queuewput(q, mp)
queue_t *q;
mblk_t *mp;
{
	printf("Queuewput called\n");
}
