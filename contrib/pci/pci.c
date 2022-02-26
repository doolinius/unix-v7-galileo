/* pci.c
 * $Id: pci.c,v 1.2 2010/03/03 01:30:59 tom Exp $
 * PCI bus support
 * Tom Trebisky  10/7/2001
 *
 * Most of the background here comes from
 * "PCI System Architecture", Tom Shanley & Don Anderson.
 * (but see linux/arch/i386/kernel/pci-pc.c)
 *
 * We don't need to break our necks optimizing this,
 * since it only gets used to probe and setup devices.
 */

#include "intel.h"
#include "pci.h"

typedef void (*pcifptr) ( struct pci_dev * );

#ifdef notdef
extern inline unsigned char  inb  (unsigned short port) {
    unsigned char  _v;
    __asm__ __volatile__ ("in" "b" " %"  "w"  "1,%"   ""   "0"  : "=a" (_v) : "Nd" (port)   );
    return _v;
}

extern inline unsigned short  inw  (unsigned short port) {
    unsigned short  _v;
    __asm__ __volatile__ ("in" "w" " %"  "w"  "1,%"   ""   "0"  : "=a" (_v) : "Nd" (port)   );
    return _v;
}

extern inline unsigned int  inl  (unsigned short port) {
    unsigned int  _v;
    __asm__ __volatile__ ("in" "l" " %"  "w"  "1,%"   ""   "0"  : "=a" (_v) : "Nd" (port)   );
    return _v;
}

extern inline void outb  ( unsigned   char   value, unsigned short port) {
    __asm__ __volatile__ ("out" "b" " %"   "b"   "0,%"  "w"  "1"  : : "a" (value), "Nd" (port));
}

extern inline void outw  (unsigned   short   value, unsigned short port) {
    __asm__ __volatile__ ("out" "w" " %"   "w"   "0,%"  "w"  "1"  : : "a" (value), "Nd" (port));
}

extern inline void outl  (unsigned   int   value, unsigned short port) {
    __asm__ __volatile__ ("out" "l" " %"      "0,%"  "w"  "1"  : : "a" (value), "Nd" (port));
}
#endif

#define PCI_VENDOR	0
#define PCI_DEVICE	2
#define PCI_COMMAND	4
#define PCI_STATUS	6

#define PCI_REV		8
#define PCI_PROG	9
#define PCI_SUBCLASS	0x0a
#define PCI_CLASS	0x0b

#define PCI_BASE_0	0x10
#define PCI_BASE_1	0x14
#define PCI_BASE_2	0x18
#define PCI_BASE_3	0x1c
#define PCI_BASE_4	0x20
#define PCI_BASE_5	0x24
#define PCI_BASE_ROM	0x30

#define PCI_INT_LINE	0x3c
#define PCI_INT_PIN	0x3d

/* Just for fun, here are the Vendor, Device scan values
 * from my home system: (4 devices on PCI bus 0).
 *
 * Vendor = 8086 = Intel
 *		Device = 1250 = i82439 (HX chipset)
 *		Device = 7000 = 82371SB_0
 * Vendor = 1002 = ATI
 *		Device = 4354 = 215CT222
 * Vendor = 1011 = Dec
 *		Device = 0014 = Tulip Plus (10base network)
 */

void pci_scan_bus ( int );
void pci_scan ( void );
int pci_read_char ( int, int, int, int );
int pci_read_short ( int, int, int, int );
long pci_read_long ( int, int, int, int );
void pci_write_char ( int, int, int, int, int );
void pci_write_short ( int, int, int, int, int );
void pci_write_long ( int, int, int, int, int );

static int inline pack ( int bus, int dev, int func )
{
	int rv;
	
	rv =  (bus  & 0xff) << 8;
	rv |= (dev  & 0x1f) << 3;
	rv |= (func & 0x07);
	return rv;
}

#ifdef notdef
/* This now conflicts with the linux PCI entry point.
 */
void
pci_init ( void )
{
	pci_scan ();
}
#endif

static int
pci_region_size ( int bus, int dev, int func, int off, int mask )
{
	unsigned long base, xbase;

	base = pci_read_long ( bus, dev, func, off );

	/* XXX - atomic lock */
	pci_write_long ( bus, dev, func, off, 0xffffffff );
	xbase = pci_read_long ( bus, dev, func, off );
	pci_write_long ( bus, dev, func, off, base );
	/* unlock */

	return (~xbase | mask) + 1;
}


/* The PCI spec sez that base registers must be
 * allocated in order, and that an undefined register
 * reads back zero, so we can stop the scan as soon
 * as we see a zero register.
 */
void
pci_scan_bases ( int bus, int dev, int func )
{
	unsigned long base;
	int i, size, off;

	for ( i=0; i<6; i++ ) {
	    off = PCI_BASE_0 + i * 4;
	    base = pci_read_long ( bus, dev, func, off );
	    if ( ! base )
	    	break;

	    size = pci_region_size ( bus, dev, func, off, 0xf );

	    printf ( "    Base %d = %08x", i, base );
	    if ( base & 0x1 )
	    	printf ( " IO " );
	    else
	    	printf ( " MEM" );

	    if ( size > 1024 )
		printf ( " %d Kb", size/1024 );
	    else
		printf ( " %d bytes", size );

	    if ( base & 0xe )
		printf ( " flags = %x", base & 0xf );
	    printf ( "\n" );
	}

	base = pci_read_long ( bus, dev, func, PCI_BASE_ROM );
	if ( ! base )
	    return;

	/* This is likely to be wrong for the ROM area size,
	 * (the bottom 11 bits are reserved)
	 * (well, actually the lowest bit enables the decoder,
	 *  but the other bits are reserved, hence the mask
	 */
	size = pci_region_size ( bus, dev, func, PCI_BASE_ROM, 0x7ff );

	printf ( "    Base (ROM) = %08x", i, base );
	if ( size > 1024 )
	    printf ( " %d Kb", size/1024 );
	else
	    printf ( " %d bytes", size );
	printf ( "\n" );
}

void
pci_scan_irq ( int bus, int dev, int func )
{
	int line, pin;

	pin = pci_read_char ( bus, dev, func, PCI_INT_PIN );
	if ( pin ) {
	    line = pci_read_char ( bus, dev, func, PCI_INT_LINE );
	    if ( line < 20 ) {
		printf ( "    IRQ %d on pin INT%c\n",
		    line, 'A' + pin - 1 );
	    }
	}
}

/* We can get carried away as far as we want here.
 * for more, see the mindshare book, pp. 338-342
 */
#define PCI_CLASS_DISPLAY	0x03
#define PCI_CLASS_NETWORK	0x02
#define PCI_CLASS_BRIDGE	0x06
#define PCI_CLASS_DISK		0x01

void
pci_scan_class ( int bus, int dev, int func, int whine )
{
	int class, subclass;

	class = pci_read_char ( bus, dev, func, PCI_CLASS );
	subclass = pci_read_char ( bus, dev, func, PCI_SUBCLASS );

	printf ( " class= %02x:%02x", class, subclass );
	if ( class == PCI_CLASS_DISPLAY )
	    printf ( " display controller" );
	else if ( class == PCI_CLASS_NETWORK )
	    printf ( " network interface" );
	else if ( class == PCI_CLASS_BRIDGE )
	    printf ( " bridge device" );
	else if ( class == PCI_CLASS_DISK )
	    printf ( " disk controller" );
	else {
	    /* don't print this if we know exactly what it is via the device */
	    if ( whine )
		printf ( " unknown gizmo" );
	}

	printf ( "\n" );
}

static char *
lookup_vendor ( int vid )
{
    	if ( vid == 0x8086 )
	    return "Intel";
    	if ( vid == 0x1307 )
	    return "Measurement Computing";
    	if ( vid == 0x124B )
	    return "SBS Greensprings";
    	if ( vid == 0x1106 )
	    return "VIA";
	return (char *) 0;
}

static char *
lookup_device ( int vid, int did )
{
    	if ( vid == 0x1307 && did == 0x71 )
	    return "DAC6703";
    	if ( vid == 0x8086 && did == 0x1229 )
	    return "eepro100 network";
    	if ( vid == 0x124B && did == 0x40 )
	    return "PCI-60/40 IP Carrier";
    	if ( vid == 0x197B && did == 0x2363 )
	    return "SATA Controller";
    	if ( vid == 0x1106 && did == 0x3044 )
	    return "IEEE 1394 OHCI Controller";
	return (char *) 0;
}

/* Notice that we don't scan beyond function 0,
 * so this will fail for multifunction boards.
 */
void
pci_scan_bus ( int bus )
{
	int dev;
	int vid, did;
	char *name;

	for ( dev=0; dev<=pci_maxdev(); dev++ ) {
	    vid = pci_read_short ( bus, dev, 0, PCI_VENDOR );
	    did = pci_read_short ( bus, dev, 0, PCI_DEVICE );
	    if ( vid == 0xffff && did == 0xffff ) {
		/*
		printf ( "PCI %d:%d Empty (%04x,%04x)\n", bus, dev, vid, did );
		*/
	    	continue;
	    }

	    printf ( "PCI %d:%d, vendor= %04x", bus, dev, vid );
	    name = lookup_vendor ( vid );
	    if ( name )
		printf ( " (%s)", name );

	    printf ( " device= %04x", did );
	    name = lookup_device ( vid, did );
	    if ( name )
		printf ( " (%s)", name );

	    pci_scan_class ( bus, dev, 0, ! name );

	    pci_scan_bases ( bus, dev, 0 );
	    pci_scan_irq ( bus, dev, 0 );
	}
}

void
pci_scan ( void )
{
    	int bus;

	for ( bus=0; bus < MAX_PCI_BUS; bus++ )
	    	pci_scan_bus ( bus );
}

static struct pci_dev *
pci_load_info ( int bus, int dev )
{
	static struct pci_dev pci_info_buf;
	struct pci_dev *pci_info;
	int i, off;

	pci_info = &pci_info_buf;

	pci_info->bus = bus;
	pci_info->dev = dev;
	pci_info->device = pci_read_short ( bus, dev, 0, PCI_DEVICE );
	pci_info->vendor = pci_read_short ( bus, dev, 0, PCI_VENDOR );
	pci_info->irqpin = pci_read_char ( bus, dev, 0, PCI_INT_PIN );
	pci_info->irq = pci_read_char ( bus, dev, 0, PCI_INT_LINE );
	for ( i=0; i<MAX_BAR; i++ ) {
	    off = PCI_BASE_0 + i * 4;
	    pci_info->base[i] = (void *) pci_read_long ( bus, dev, 0, off );
	}
	return pci_info;
}


void
pci_find_all ( int bus, pcifptr func )
{
	int dev;

	for ( dev=0; dev<=pci_maxdev(); dev++ ) {
	    if ( pci_read_short ( bus, dev, 0, PCI_VENDOR ) == 0xffff )
		continue;
	    if ( pci_read_short ( bus, dev, 0, PCI_DEVICE ) == 0xffff )
		continue;
	    (* func)( pci_load_info ( bus, dev ) );
	}
}

void
pci_find ( int bus, int vendor, int device, pcifptr func )
{
	int dev;

	for ( dev=0; dev<=pci_maxdev(); dev++ ) {
	    if ( pci_read_short ( bus, dev, 0, PCI_VENDOR ) != vendor )
		continue;
	    if ( pci_read_short ( bus, dev, 0, PCI_DEVICE ) != device )
		continue;
	    (* func)( pci_load_info ( bus, dev ) );
	}
}

/* ----------------------------------------------------------
 *
 * Support for PCI configuration transaction access follows.
 * There are two "mechanisms":
 * Mechanism 1 is the new way and
 *  must be implemented in new PCI hardware.
 * Mechanism 2 is the old way,
 *  the only place I have found it used is ancient
 *  P75 and P90 era intel NX chipset motherboards.
 * Running mechanism 2 on a modern chipset wrongly shows
 * an empty PCI bus.
 *
 * It is possible to figure out which to use at run time,
 * but I am too lazy to implement this right now.
 * Also note that the old mechanism supports 16, not 32
 * devices on a bus.
 */

/*
#define NEW_WAY
#define OLD_WAY
*/

#define NEW_WAY

#ifdef NEW_WAY

#define		X_PORT	0xcf8
#define		Y_PORT	0xcfc

/* Modern chipsets access the configuration space like this
 */
int
pci_maxdev ( void )
{
	return 0x1f;
}

int
pci_read_char ( int bus, int dev, int func, int off )
{
	outl ( 0x80000000 | pack(bus,dev,func) << 8 | (off&(~0x3)), X_PORT );
	return inb ( Y_PORT + (off&0x3) );
}

int
pci_read_short ( int bus, int dev, int func, int off )
{
	outl ( 0x80000000 | pack(bus,dev,func) << 8 | (off&(~0x3)), X_PORT );
	return inw ( Y_PORT + (off&0x2) );
}

long
pci_read_long ( int bus, int dev, int func, int off )
{
	outl ( 0x80000000 | pack(bus,dev,func) << 8 | (off&(~0x3)), X_PORT );
	return inl ( Y_PORT );
}

void
pci_write_char ( int bus, int dev, int func, int off, int val )
{
	outl ( 0x80000000 | pack(bus,dev,func) << 8 | (off&(~0x3)), X_PORT );
	outb ( val, Y_PORT + (off&0x3) );
}

void
pci_write_short ( int bus, int dev, int func, int off, int val )
{
	outl ( 0x80000000 | pack(bus,dev,func) << 8 | (off&(~0x3)), X_PORT );
	outb ( val, Y_PORT + (off&0x2) );
}

void
pci_write_long ( int bus, int dev, int func, int off, int val )
{
	outl ( 0x80000000 | pack(bus,dev,func) << 8 | (off&(~0x3)), X_PORT );
	outl ( val, Y_PORT );
}
#endif /* NEW WAY */

#ifdef OLD_WAY

#define		X_PORT	0xcf8
#define		Y_PORT	0xcfa
#define		Z_PORT	0xc000

/* Ancient chipsets access the configuration space like this:
 * (this code has not been tested.).
 */

int
pci_maxdev ( void )
{
	return 0x0f;
}

int
pci_read_char ( int bus, int dev, int func, int off )
{
	int rv;

	outb ( (dev&0xf)<<4 | (func&0x7)<< 1,  X_PORT );
	outb ( bus, Y_PORT );
	rv = inb ( Z_PORT | (dev&0xf)<<8 | off );
	outb ( 0, X_PORT );
	return rv;
}

int
pci_read_short ( int bus, int dev, int func, int off )
{
	int rv;

	outb ( (dev&0xf)<<4 | (func&0x7)<< 1,  X_PORT );
	outb ( bus, Y_PORT );
	rv = inw ( Z_PORT | (dev&0xf)<<8 | off );
	outb ( 0, X_PORT );
	return rv;
}

long
pci_read_long ( int bus, int dev, int func, int off )
{
	int rv;

	outb ( (dev&0xf)<<4 | (func&0x7)<< 1,  X_PORT );
	outb ( bus, Y_PORT );
	rv = inl ( Z_PORT | (dev&0xf)<<8 | off );
	outb ( 0, X_PORT );
	return rv;
}

void
pci_write_char ( int bus, int dev, int func, int off, int val )
{
	outb ( (dev&0xf)<<4 | (func&0x7)<< 1,  X_PORT );
	outb ( bus, Y_PORT );
	outb ( val, Z_PORT | (dev&0xf)<<8 | off );
	outb ( 0, X_PORT );
}

void
pci_write_short ( int bus, int dev, int func, int off, int val )
{
	outb ( (dev&0xf)<<4 | (func&0x7)<< 1,  X_PORT );
	outb ( bus, Y_PORT );
	outw ( val, Z_PORT | (dev&0xf)<<8 | off );
	outb ( 0, X_PORT );
}

void
pci_write_long ( int bus, int dev, int func, int off, int val )
{
	outb ( (dev&0xf)<<4 | (func&0x7)<< 1,  X_PORT );
	outb ( bus, Y_PORT );
	outl ( val, Z_PORT | (dev&0xf)<<8 | off );
	outb ( 0, X_PORT );
}
#endif /* OLD WAY */

/* THE END */
