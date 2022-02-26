/*
 * dac6703.c
 *
 * Tom Trebisky  11-7-2005 began
 * Tom Trebisky  1-23-2006 began in earnest
 * Tom Trebisky  1-26-2006 working driver
 *
 * Driver for the Measurement Computing DAC-6703
 * (also supports the 6702 and 6704, but 6704 support
 *  is untested)
 * The 6703 is a PCI card with 16 channels of 16-bit DAC,
 * (analog output) along with 8 bits of digital IO.
 *
 * "They" say this card is entirely compatible with the
 * National Instruments DAC 6703 card, but that has yet
 * to be proven with this driver.  This driver adheres to
 * the register map published by Measurement Computing,
 * which seems to apply to the registers mapped by BAR 2 and 3.
 * I suspect that the registers mapped by BAR 0 and 1 might
 * be the compatibility set, but who knows for sure.
 *
 * The connector on the back of this card is a high density
 * D connector with 100 pins,  A nice special cable splits
 * this into two plain old 50 pin ribbons.  The one labelled
 * 1-50 is the one you want:
 *
 * 	pin 1 = AGND
 * 	pin 2 = Dac output 0
 * 	pin 3 = AGND0
 * 	pin 4 = Dac output 1
 * 	pin 5 = AGND1
 * 	pin 6 = Dac output 2
 * 	pin 7 = AGND2
 * 	pin 8 = Dac output 3
 * 	pin 9 = AGND3
 */

#include <vxWorks.h>
#include "pci.h"

/* Some kind of hackery is required to write to this device,
 * see the detailed comments deeper in this driver source.
#define MW_METHOD
#define TOM_METHOD
 */
#define MW_METHOD

/* prototypes */
void pci_dac_clear ( void );
void pci_dac_out ( int, int );
int pci_dac_vscale ( double );

static int card_count = -1;

#define DAC_PCI_VENDOR		0x1307
#define DAC_PCI_DEVICE_6702	0x0070
#define DAC_PCI_DEVICE_6703	0x0071
#define DAC_PCI_DEVICE_6704	0x0072

/* The only readable register is "rb" */
struct dac_hw {
/* 00 */	volatile unsigned short nvram;
/* 02 */	volatile unsigned short up_config1;
/* 04 */	volatile unsigned short rb;
/* 06 */	volatile unsigned short update;
/* 08 */	volatile unsigned short channel;
/* 0A */	volatile unsigned short data;
};

/* For the DAC-6704 */
#define	up_config2	rb

struct dio_hw {
/* 00 */	volatile unsigned char direction;
/* 02 */	volatile unsigned char data;
};

/* Keeping these pointers like this limits us to
 * supporting only one card, but any English schoolboy
 * could upgrade the driver to support multiple cards in
 * an hour or so.
 */
static struct dac_hw *dac_base;
static struct dio_hw *dio_base;
static int dac_channels;

/*
 * This thing sets 4 PCI BAR registers:
 * PCI 0:11, vendor= 1307, device= 0071 class= ff:00 unknown gizmo
 *     Base 0 = e0044000 MEM 128 bytes
 *     Base 1 = 0000ac01 IO  128 bytes
 *     Base 2 = e0040000 MEM 32 bytes  <-- access to DAC registers
 *     Base 3 = e0041000 MEM 16 bytes  <-- access to DIGIO registers
 */

static void
pci_dac_probe ( struct pci_dev *pci_info )
{
    int channels;

    if ( pci_info->vendor != DAC_PCI_VENDOR )
	return;

    channels = 0;
    if ( pci_info->device == DAC_PCI_DEVICE_6702 )
	channels = 8;
    if ( pci_info->device == DAC_PCI_DEVICE_6703 )
	channels = 16;
    if ( pci_info->device == DAC_PCI_DEVICE_6704 )
	channels = 32;

    if ( channels == 0 )
	return;

    card_count++;

    if ( card_count > 1 ) {
	printf ("Whoa!! multiple DAC6703 cards!\n");
	return;
    }

    /* XXX - first card only */
    dac_base = (struct dac_hw *) pci_info->base[2];
    dio_base = (struct dio_hw *) pci_info->base[3];
    dac_channels = channels;
}

/* On our ASUS P4B266 board we found the PCI slots on bus 2,
 * bus 0 was on board stuff and bus 1 was AGP
 * On the other hand on the 600 Mhz gigabyte board,
 * everything is on bus 0
 */
static void
locate_pci_dac ( void )
{
    int bus;

    card_count = 0;

    for ( bus=0; bus<MAX_PCI_BUS; bus++ )
	pci_find_all ( bus, pci_dac_probe );

    if ( card_count ) {
	printf ("Found %d DAC670x cards\n", card_count );
	if ( dac_channels == 8 )
	    printf ("Attached a DAC6702 card at %08x\n", dac_base );
	if ( dac_channels == 16 )
	    printf ("Attached a DAC6703 card at %08x\n", dac_base );
	if ( dac_channels == 32 )
	    printf ("Attached a DAC6704 card at %08x\n", dac_base );
	printf ("Digital IO base at: %08x\n", dio_base );
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
pci_dac_init ( void )
{
    locate_pci_dac ();
    if ( card_count < 1 )
	return 0;

    /* Make the PIO bits outputs */
    dio_base->direction = 0xff;

    /* Make the DAC's update immediately */
    dac_base->up_config1 = 0x0000;
    if ( dac_channels == 32 )
	dac_base->up_config2 = 0x0000;

    pci_dac_clear ();

    return card_count;
}

/* Load all channels with
 * the same value.
 */
void
pci_dac_load ( int data )
{
    int chan;

    for ( chan=0; chan < dac_channels; chan++ )
	pci_dac_out ( chan, data );
}

/* Load all channels with zero */
void
pci_dac_clear ( void )
{
    int chan;

    for ( chan=0; chan < dac_channels; chan++ )
	pci_dac_out ( chan, 0x8000 );
}

/* Any dac's with a one in their mask will
 * not update until a write to the update
 * register occurs.
 * This allows all DAC's to be synchronized.  
 * We do not use this feature, and it is
 * untested.
 */
void
pci_dac_config ( int data )
{
    dac_base->up_config1 = data;
}

/* For the 6704 */
void
pci_dac_config2 ( int data )
{
    dac_base->up_config2 = data;
}

/* Note (7/1/2007) - a comment in the mathworks 2007a xpc
 * driver for this card says to NOT use this as very bad
 * output glitches result.
 * (their driver just writes a 0x1)
 */
void
pci_dac_update ( void )
{
    /* Write any data to this register to strobe
     * stuff into the DAC's
     */
    dac_base->update = 0xffff;
}

/* PIO access tested and works 1-24-2006 */
void
pci_dac_pio_out ( int data )
{
    dio_base->data = (unsigned char) data;
}

#ifdef TOM_METHOD
/* Strangely, there is some timing issue writing to
 * the DAC's.  If we don't wait a while after a
 * write, the data just doesn't get written, or at
 * least it doesn't get written right.  Luckily
 * the readback register tells the truth about
 * what is going on.
 *
/* I have never seen it require more than 3 writes
 * to the same channel to get the job done.
 * In fact now that I have the test and printf in
 * the loop, I never see it require more than 2.
 * XXX - we could just add some delay loop,
 * but this does the job.
 */
#define MAX_TRYS	5

void
pci_dac_out ( int chan, int data )
{
    int tries;
    int rb;

    for ( tries = MAX_TRYS; tries; tries-- ) {

	dac_base->channel = chan;
	dac_base->data = data;

	/* You really do have to reset the address */
	dac_base->channel = chan;
	rb = dac_base->rb;

	if ( rb == data )
	    return;

#ifdef DIAG
	if ( tries < MAX_TRYS )
	    printf ("dac_out ch %d rewriting after %d: wrote %04x, got %04x\n",
		chan, MAX_TRYS-tries+1, data, rb );
#endif
    }

    printf ("dac_out ch %d Failed: wrote %04x, got %04x\n",
	chan, data, rb );
#ifdef DIAG
    pci_dac_fail = 1;
#endif
    return;
}
#endif /* TOM METHOD */

#ifdef MW_METHOD
/* This is the way the mathworks rtw/xpc driver does it
 * circa r2007a - they say that the two dummy reads after
 * the write "keep the boards state machine from getting
 * confused -- and missing updating channels sometimes
 * (intermittent)".
 * This may all just be a timing issue for fast back to back
 * writes (see comment on next function).
 */
void
pci_dac_out ( int chan, int data )
{
    int rb;

    dac_base->channel = chan;
    dac_base->data = data;
    rb = dac_base->data;
    rb = dac_base->data;

    return;
}
#endif /* MW METHOD */

/* This works, but has weird
 * troubles with back to back writes.
 * Use the above, unless you KNOW your timing
 * is slow enough to avoid the problem.
 */
void
pci_dac_out_quick ( int chan, int data )
{
    dac_base->channel = chan;
    dac_base->data = data;
}

/* This is the published correct value */
#define DAC_SCALE_IDEAL	0.000308228

/* This makes our card give 10.0 volts
 * (we could dork with the internal gain
 * calibration, but we leave it alone and
 * do this (since the internal gain is a
 * bit hot, we are giving up a few last
 * bits of resolution this way, since our
 * unit will actually go to 10.25 volts).
 */
#define DAC_SCALE	0.000312543
/* 1 / (the above) */
#define DAC_SCALE_M	3199.55974058

void
pci_dac_volts ( int chan, double volts )
{
    int data;

    data = volts * DAC_SCALE_M;
    data += 0x8000;
    if ( data < 0 )
	data = 0;
    if ( data > 0xffff )
	data = 0xffff;

    pci_dac_out ( chan, data );
}

int
pci_dac_vscale ( double volts )
{
    int data;

    data = volts * DAC_SCALE_M;
    data += 0x8000;
    if ( data < 0 )
	data = 0;
    if ( data > 0xffff )
	data = 0xffff;

    return data;
}

unsigned int
pci_dac_rb ( int chan )
{
    dac_base->channel = chan;
    return dac_base->rb;
}

/* Not tested as yet */
void
pci_dac_save ( void )
{
    /* pulsing this bit should save all channel
     * DAC values as startup values, as well as
     * any changes to the offset and gain info.
     */
    dac_base->nvram = 0x80;
    dac_base->nvram = 0x00;
}

/* Two pseudo channels are used to fiddle with board
 * calibration values (offset and gain).
 * Once fiddled, they may be made permanent
 * (along with all DAC settings for startup values)
 * by yanking the SAVE bin in the nvram control register.
 *
 * As I received our 6703, it fires up with all
 * channels set to 0x8000 (0 volts)
 * r16 (offset) is 0x8001
 * 	(write this value and get 0.0 volts)
 * r17 (gain) is 0xFCF3
 * 	(write this value and get 10.0 volts)
 * With this offset and gain, I get the following:
 * 	write 0x0000 --> get -10.25 volts
 * 	write 0xffff --> get  10.24 volts
 */
#define OFFSET_6702	17
#define GAIN_6702	18

#define OFFSET_6703	16
#define GAIN_6703	17

#define OFFSET_6704	32	/* documented for the NI-6704 */
#define GAIN_6704	34	/* documented for the NI-6704 */
#define I_OFFSET_6704	33	/* current offset, documented for the NI-6704 */
#define I_GAIN_6704	35	/* current gain, documented for the NI-6704 */

void
pci_dac_offset ( int data )
{
    if ( dac_channels == 8 )
	dac_base->channel = OFFSET_6702;
    else if ( dac_channels == 16 )
	dac_base->channel = OFFSET_6703;
    else
	dac_base->channel = OFFSET_6704;
    dac_base->data = data;
}

void
pci_dac_gain ( int data )
{
    if ( dac_channels == 8 )
	dac_base->channel = GAIN_6702;
    else if ( dac_channels == 16 )
	dac_base->channel = GAIN_6703;
    else
	dac_base->channel = GAIN_6704;
    dac_base->data = data;
}

/*
#define DIAG
*/

#ifdef DIAG
#include <stdioLib.h>

static int pci_dac_fail;

void
pci_dac_test_one ( void )
{
    int i, d;

    pci_dac_fail = 0;

    for ( d=0; d<65536; d++ )
	for ( i=0; i<dac_channels; i++ )
	    pci_dac_out ( i, d );

    if ( pci_dac_fail )
	printf ("Oops\n");
    else
	printf ("OK\n");
}

void
pci_dac_test ( int times )
{
    int i;

    for ( i=0; i<times; i++ ) {
	printf ( "%d ", i+1 );
	pci_dac_test_one ();
	fflush ( stdout );
	taskDelay ( 1 );
    }
}

/* For diagnostic use only,
 * loads successive channels with 0.5, 1.0, 1.5 volts
 */
void
pci_dac_fill ( void )
{
    int chan;
    int d1, d2, dd, data;

    d1 = pci_dac_vscale ( 0.0 );
    d2 = pci_dac_vscale ( 8.0 );
    dd = (d2-d1) / 16;
    data = d1 + dd;

    /*
    printf ("d1 = %04x\n", d1 );
    printf ("d2 = %04x\n", d2 );
    printf ("dd = %04x\n", dd );
    */

    for ( chan=0; chan < dac_channels; chan++ ) {
	pci_dac_out ( chan, data );
	data += dd;
    }
}
#endif /* DIAG */

/* THE END */
