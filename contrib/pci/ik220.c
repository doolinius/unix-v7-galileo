/*
 * ik220.c
 *
 * Tom Trebisky  2-24-2010 began
 *
 * Driver for the Heidenhain ik220 PC counter card.
 */

#include <vxWorks.h>
#include "pci.h"

#include "ik220_firmware.h"

/* Each card controls two axes, so this driver will support up to 4 cards.
 */
#define MAX_AXES	8

/* prototypes */

static int axis_count = 0;

#define PCI_VENDOR	0x10B5
#define PCI_DEVICE	0x9050

/* This structure would seem to be the G28 chip
 * (one per axis, two per ik220 card)
 */
struct ik_axis_hw {
	volatile unsigned short r0;
	volatile unsigned short r1;
	volatile unsigned short r2;
	volatile unsigned short r3;
	volatile unsigned short r4;
	volatile unsigned short r5;
	volatile unsigned short r6;
	volatile unsigned short r7;
	volatile unsigned short r8;
	volatile unsigned short r9;
	volatile unsigned short r10;
	volatile unsigned short flag_ctl;
	volatile unsigned short csr;
	volatile unsigned short flag0;
	volatile unsigned short flag1;
	volatile unsigned short code;
};

/* Note on flag_ctl:
 *    write to set   flag 0,
 *    read  to clear flag 1
 */

#define	FLAG_SEM0	0x0001
#define	FLAG_SEM10	0x0400

/* Commands */
#define	START		0x102
#define	READ_COUNT	0x301
#define	GET_VERSION	0x407
#define	LED_OFF		0x1010
#define	LED_ON		0x1011
#define	LED_BLINK	0x1012

#define RUN_MODE	0x0000
#define BOOT_MODE	0x0001
#define	WRITE_RAM	0x0009

#define	PIPE_EMPTY	0x0002

static struct ik_axis_hw *ik_base[MAX_AXES];

void ik220_init_axis ( int );
static void ik220_firmware_download ( struct ik_axis_hw *, int );
static void ik220_run ( struct ik_axis_hw * );

/*
 * This thing sets 4 PCI BAR registers:
 *
 * PCI 0:13, vendor= 10b5, device= 9050 class= 06:80 bridge device
 *     Base 0 = de800000 MEM 128 bytes
 *     Base 1 = 0000a001 IO  128 bytes
 *     Base 2 = de000000 MEM 32 bytes
 *     Base 3 = dd800000 MEM 32 bytes
 *     IRQ 10 on pin INTA
 *
 * Base 0 is a set of config registers (never used)
 * Base 1 is unknown
 * Base 2 are the registers for axis 0
 * Base 3 are the registers for axis 1
 */

static void
ik220_probe ( struct pci_dev *pci_info )
{
    if ( pci_info->vendor != PCI_VENDOR )
	return;

    if ( pci_info->device != PCI_DEVICE )
	return;

    if ( axis_count + 2 > MAX_AXES ) {
	printf ("Whoa!! too many IK220 cards!\n");
	return;
    }

    printf ("Attached a IK220 axis at %08x\n", pci_info->base[2] );
    ik_base[axis_count++] = (struct ik_axis_hw *) pci_info->base[2];
    ik220_init_axis ( axis_count-1 );

    printf ("Attached a IK220 axis at %08x\n", pci_info->base[3] );
    ik_base[axis_count++] = (struct ik_axis_hw *) pci_info->base[3];
    ik220_init_axis ( axis_count-1 );
}

/* On our ASUS P4B266 board we found the PCI slots on bus 2,
 * bus 0 was on board stuff and bus 1 was AGP
 * On the other hand on the 600 Mhz gigabyte board,
 * everything is on bus 0
 */
static void
locate_ik220 ( void )
{
    int bus;

    for ( bus=0; bus<MAX_PCI_BUS; bus++ )
	pci_find_all ( bus, ik220_probe );

    if ( axis_count ) {
	printf ("Found %d IK220 axes\n", axis_count );
    }
}

/* The first time you use this on a new motherboard, you will get
 * a page fault doing the IO accesses below.
 * Too bad there is no way to poke holes in the MMU at runtime,
 * so what you have to do is:
 * 	cd /opt/vxworks/VW531/target/config/pc486
 * 	vi sysLib.c
 * 	vxmake
 * 	vxmake install
 */
int
ik220_init ( void )
{
    axis_count = 0;

    locate_ik220 ();

    return axis_count;
}

void
ik220_init_axis ( int axis )
{
    struct ik_axis_hw *hwp;

    if ( axis < 0 || axis >= axis_count )
	return;

    hwp = ik_base[axis];

    ik220_firmware_download ( hwp, axis );
    ik220_run ( hwp );
}

/* This downloads the axis software to the G28 chip on the IK220
 * (the axis doesn't do jack until you give it a program).
 * Apparently there are two G28 chips on each ik220 card, one per axis.
 * We always download to RAM offset 0, though we could do other things.
 */
static void
ik220_firmware_download ( struct ik_axis_hw *hwp, int axis )
{
    unsigned short val;
    int count = sizeof(firmware) / sizeof(unsigned short);
    unsigned short addr = 0;
    int i;
    int wait;

    /* Always read 0x0192 */
    /*
    val = hwp->csr;
    printf ( "download: initial CSR register: %04x\n", val );
    */
    printf ( "%d words of firmware to download for axis %d\n", count, axis );

    for ( i=0; i<count; i++ ) {

	hwp->csr = BOOT_MODE;

	/* This wait never seems needed */
	wait = 0;
	while ( ! hwp->csr & PIPE_EMPTY )
	    wait++;

	/*
	if ( wait > 0 )
	    printf ("Waited for %d counts\n", wait );
	else
	    printf ( "Pipe empty!\n" );
	*/

	hwp->r0 = addr + i;
	hwp->r1 = firmware[i];
	hwp->csr = WRITE_RAM;

	/* This wait always takes 1 count on our fast machine */
        for ( wait=0; ; wait++ ) {
	    if ( hwp->csr & PIPE_EMPTY )
		break;
	}

	/*
	if ( wait > 0 )
	    printf ("Waited for %d counts\n", wait );
	else
	    printf ( "Pipe empty!\n" );
	*/

    }
}

/* Call this after firmware is loaded to run it.
 */
static void
ik220_run ( struct ik_axis_hw *hwp )
{
	int junk;

	/* dummy read to clear semaphores */
	junk = hwp->flag_ctl;

	hwp->csr = RUN_MODE;

	while ( ! hwp->flag1 & FLAG_SEM10 )
	    ;

	/* dummy read to clear semaphores */
	junk = hwp->flag_ctl;
}

ik220_command ( struct ik_axis_hw *hwp, int cmd )
{
    int wait;
    int val;

    /* G28 command port */
    hwp->r0 = cmd;

    val = hwp->flag1;
    printf ("command, flag = %04x\n", val );

    wait = 0;
    while ( ! hwp->flag1 & FLAG_SEM0 )
	wait++;

    if ( wait > 0 )
	printf ("Waited for %d counts\n", wait );
    else
	printf ( "No wait!\n" );

    /* readback to verify */
    val = hwp->r0;
    if ( val != cmd )
	printf ("Command write failed, found: %04x, expected %04x\n", val, cmd );
}

void
ik220_version ( int axis )
{
    struct ik_axis_hw *hwp;
    unsigned short	val;
    union {
	short u_s[7];
	char u_c[14];
    } v_buf;

    if ( axis < 0 || axis >= axis_count )
	return;

    hwp = ik_base[axis];

    /* Gets 0x07ff */
    val = hwp->flag1;
    printf ( "Flag register: %04x\n", val );

    printf ( "Write command to %08x\n", &hwp->r0 );

    hwp->r0 = GET_VERSION;

    val = hwp->flag1;
    printf ( "Flag register: %04x\n", val );

    /* Still holds the command */
    printf ( "r0: %04x\n", hwp->r0 );

    printf ( "r1: %04x\n", hwp->r1 );
    printf ( "r2: %04x\n", hwp->r2 );
    printf ( "r3: %04x\n", hwp->r3 );
    printf ( "r4: %04x\n", hwp->r4 );
    printf ( "r5: %04x\n", hwp->r5 );
    printf ( "r6: %04x\n", hwp->r6 );

    v_buf.u_s[0] = hwp->r1;
    v_buf.u_s[1] = hwp->r2;
    v_buf.u_s[2] = hwp->r3;
    v_buf.u_s[3] = hwp->r4;
    v_buf.u_s[4] = hwp->r5;
    v_buf.u_s[5] = hwp->r6;
    v_buf.u_s[6] = 0;

    /* spits out 42_652.740 */
    printf ("Version: %s\n", v_buf.u_c );
}

void
ik220_ledon ( int axis )
{
    struct ik_axis_hw *hwp;

    if ( axis < 0 || axis >= axis_count )
	return;

    ik220_command ( ik_base[axis], LED_ON );
}

/* THE END */
