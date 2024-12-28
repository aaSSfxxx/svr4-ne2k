#include <sys/cmn_err.h>
#include "sys/ne2k.h"

extern struct ne2k_device devs[];
extern int num_devs;
extern unsigned char inb(unsigned int addr);
extern void outb(unsigned int addr, unsigned char val);

#define COMMAND 0
#define CLDA0 1
#define CLDA1 2
#define BNRY  3
#define TSR   4
#define NCR   5

/* Read I/O register addresses */
#define ISR	7
#define CRDA0	8
#define CRDA1	9
#define RSR	12
#define DCR	14
#define IMR	15
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

/* Control register commands */
#define CMD_STOP 1
#define CMD_START 2
#define CMD_TRANSMIT 4
#define CMD_RREAD 8
#define CMD_RWRITE 16
#define CMD_NODMA 32
#define CMD_PAGE0 0 
#define CMD_PAGE1 64
#define CMD_PAGE2 128

#define PAGE1_PHYS   1
#define PAGE1_CURPAG 7
#define PAGE1_MULT   8

static char* hexchrs[] = {
	"0","1","2","3","4","5","6","7","8",
	"9","a","b","c","d","e","f"
};

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
		printf("%s%s%s", hexchrs[s[2*i] >> 4 & 0xf], hexchrs[s[2*i] & 0xf], (i != 5) ? ":" : "");
	}
	printf("\n");
}

ne2kinit() {
	int i, j;
	int iobase;
	unsigned char prom[32];

	for(i = 0; i < num_devs; i++) {
		printf("ne2k: Device configured at irq %d io %x\n", 
			devs[i].irq, devs[i].io_base);
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
		printf("ne2k: card MAC address is ");
		print_macaddr(prom); 
		
		/* Switch to page 1 and write MAC address */
		outb(iobase, CMD_PAGE1 | CMD_STOP | CMD_NODMA);
		for(j = 0; j < 6; j++) {
			outb(iobase + PAGE1_PHYS + j, prom[2*j]);
		}
		for(j = 0; j < 6; j++) {
			outb(iobase + PAGE1_MULT + j, 0xff);
		}
		/* Switch to page 0 and configure parameters */
		outb(iobase, CMD_START | CMD_PAGE0 | CMD_NODMA);

		/* Configure TX and RX parameters */
		outb(iobase+RXCR, 0xe);
		outb(iobase+TXCR, 4);
		/* Set ring buffer size */
		outb(iobase+STARTPG, 60);
		outb(iobase+BNDRY, 60);
		outb(iobase+ENDPG, 96);

		/* Configure interrupt mask */
		outb(iobase+IMR, 0x1f);
	}
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
	printf("ne2k: interrupted, status reg %x", c);	
}