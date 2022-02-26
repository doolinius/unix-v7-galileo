/*
 * from Skidoo
 * Tom Trebisky  9/21/2001
 * split out from console.c 12/21/2002
 * Hauled over to pc486 VxWorks 2/25/2005
 */

/* Now for the details of talking to the keyboard controller.
 * It uses just a pair of port addresses.
 * The clearest explanation I have found so far is in
 * the VIA Technologies datasheet for the VT82C586B chip,
 * which includes a keyboard controller along with other
 * peripherals.  Here it is paraphrased:
 *
 * Reading port 0x64 returns a status byte.
 * Writing port 0x64 sends command codes.
 * Input and output data goes thru port 0x60
 *
 * A control register is available via some indirect
 * trickery:
 *
 *	Write 0x20 to the command port (port 0x64).
 *	wait for output buffer full status.
 *	Read the control byte from port 0x60.
 *
 *	Write 0x60 to the command port (port 0x64).
 *	Write the control byte to port 0x60.
 *	
 * Can only write to port 0x60 when the IBF flag
 * is clear in port 0x64. (bit 1)
 * 
 * Can only read from port 0x60 when the OBF flag
 * is set in port 0x64.	(bit 0)
 * 
 */

#include "intel.h"

#define KB_DATA		0x60
#define KB_STATUS	0x64
#define KB_CMD		0x64

#define KB_ST_IBF	0x02
#define KB_ST_OBF	0x01

#define KBC_SCAN	0x40	/* do PC scan codes */
#define KBC_DIS		0x10	/* disable keyboard */
#define KBC_ME		0x02	/* enable mouse interrupts (IRQ12) */
#define KBC_IE		0x01	/* enable keyboard interrupts (IRQ1) */

/* Good grief, this actually works.
 * We can use the keyboard controller to
 * reset the CPU.
 */
#ifndef BROKEN_COMPILER
void
x86_reset ( void )
{
	while ( inb ( KB_STATUS ) & KB_ST_IBF )
	    ;
	outb ( 0xfe, KB_CMD );

	for ( ;; )
	    ;
}
#else
void
x86_reset ( void )
{
	while ( sysInByte ( KB_STATUS ) & KB_ST_IBF )
	    ;
	sysOutByte ( KB_CMD, 0xfe );

	for ( ;; )
	    ;
}
#endif

/* We actually want this to replace the faulty reboot
 * in pc486 VxWorks.  We ignore the argument.
 */
void
reboot ( int type )
{
    x86_reset ();
}

/* THE END */
