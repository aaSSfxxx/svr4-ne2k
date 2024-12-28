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
#define ISR   7
#define CRDA0 8
#define CRDA1 9
#define RSR   12
#define DCR   15
#define RESET 0x1f

/* Write I/O register addresses */
#define RSARLO 8
#define RSARHI 9
#define RCNTLO 10
#define RCNTHI 11

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

static char* hexchrs[] = {
	"0","1","2","3","4","5","6","7","8",
	"9","a","b","c","d","e","f"
};


print_macaddr(s)
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
		outb(iobase+DCR, 0x49);
		outb(iobase+RSARHI, 0);
		outb(iobase+RSARLO, 0);
		outb(iobase+RCNTHI, 0);
		outb(iobase+RCNTLO, 32);	
	
		/* Start again the card and read the EEPROM data */	
		outb(iobase, CMD_PAGE0 | CMD_START | CMD_RREAD);
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
	
	}
}

ne2kintr(irq)
int irq;
{
/*	printf("Interrupted! %d\n", irq); */
}
