/* pci.h
 * header for the skidoo pci subsystem
 * Tom Trebisky  3/9/2005
 */
#define MAX_PCI_BUS	256

#define MAX_BAR		6

struct pci_dev {
	int bus;
	int dev;
	int device;
	int vendor;
	int irq;
	int irqpin;
	void * base[MAX_BAR];
};

/* THE END */
