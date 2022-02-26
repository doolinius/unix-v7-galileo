/* intel.h
 * $Id: intel.h,v 1.1 2005/05/19 22:41:00 tom Exp $
 * part of skidoo, T. Trebisky   11/2001
 *
 * machine dependent definitions for the x86 architecture.
 */

#define PAGE_SIZE	4096
#define PAGE_SHIFT	12
#define PAGE_MASK	(~(PAGE_SIZE-1UL))

#define MAX_DMA_ADDR	0x1000000	/* 16M ISA DMA */
#define L1_CACHE_LINE	256

/* --------------------------------------------------------- */
/* Here is a 48 bit "pseudo-descriptor"
 * XXX - note that accessing the "offset"
 *  via a long gets in trouble.
 *  The compiler inserts 2 bytes of padding
 *  to nicely (and wrongly) align "offset"
 *  on a quad byte boundary.
 */

struct desc48 {
	unsigned short limit;
	unsigned short off_lo;
	unsigned short off_hi;
};

/* --------------------------------------------------------- */
/* This is the structure of an i386 segment
 * descriptor.
 * XXX - now we must split "attr" due to compiler padding.
 *
 * XXX - only partly tested,
 *	at least the order of elements is correct.
 *	The attribute bits are OK, if the attributes
 *	are assembled into a 16-bit word as shown.
 */

struct desc64 {
	unsigned short limit;
	unsigned short base_lo;
	unsigned char base_mid;
	unsigned char attr_lo;
	unsigned char attr_hi;
	unsigned char base_hi;
};

/* in attr_hi
 */
#define DAT_GRAN	0x80	/* 4k if set, 1b otherwise */
#define DAT_386		0x40	/* always set for i386 */
#define DAT_AVL		0x10	/* available for software */
				/* bit here must be zero */
#define DAT_LIMIT	0x0f	/* upper 4 bits of limit */

/* in attr_lo
 */
#define DAT_VALID	0x80	/* set when valid */
#define DAT_LEVEL	0x60	/* 2 bits for privelege level */
#define DAT_MEM		0x10	/* else it is a gate or sysseg */
#define DAT_TYPE	0x0f	/* see DTY definitions */

#define DTY_DATA	0x02	/* read-write (data) */
#define DTY_CODE	0x0a	/* read-execute (code) */

/* 6 more DTY...
 * notice that the lowest bit in DAT_TYPE starts out zero,
 * but gets set to one by hardware when the segment is accessed.
 * (kind of like a dirty bit).
 */

/* --------------------------------------------------------- */
/* This is the structure of an i386 task/interrupt
 * gate descriptor.
 */

struct gate {
	unsigned short off_lo;
	unsigned short sel;
	unsigned short attr;
	unsigned short off_hi;
};

#define GATE_VALID	0x8000	/* set when valid */
#define GATE_LEVEL	0x6000	/* 2 bits */
#define GATE_MEM	0x1000	/* else a gate or sysseg */
#define GATE_TYPE	0x0f00	/* see below */
#define GATE_COUNT	0x00ff	/* only 5 bits */

#define GATE_LDT	2	/* LDT */
#define GATE_TASK	5	/* task gate */
#define GATE_TSS_FREE	9	/* available TSS */
#define GATE_TSS_BUSY	B	/* busy TSS */
#define GATE_CALL	C	/* call gate */
#define GATE_INTR	E	/* interrupt gate */
#define GATE_TRAP	F	/* trap gate */

/* The following are mostly
 * ripped off from linux 2.4
 *
 * /usr/src/linux/include/asm/ *.h
 */

extern inline void cli ( void )
{
    __asm__ __volatile__ ( "cli" );
}

extern inline void sti ( void )
{
    __asm__ __volatile__ ( "sti" );
}

extern inline void delay ( void )
{
    __asm__ __volatile__ ( "outb %al, $0x80" );
}

#ifdef notdef
void
delay ( void )
{
	outb ( 0, 0x80 );
}
#endif

#define __cli()                 __asm__ __volatile__("cli": : :"memory")
#define __sti()                 __asm__ __volatile__("sti": : :"memory")

/* The usual drill with the following is:
 *
 *	unsigned long flags;
 *
 *	save_flags ( flags );
 *	cli ();
 *	...
 *	restore_flags ( flags );
 *
 *	The first two instructions may be replaced with:
 *
 *	irq_save ( flags );
 */

#define save_flags(x)     __asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define restore_flags(x)  __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")

#define irq_save(x)       __asm__ __volatile__("pushfl ; popl %0 ; cli":"=g" (x): /* no input */ :"memory")


/*
 * This file contains the definitions for the x86 IO instructions
 * inb/inw/inl/outb/outw/outl and the "string versions" of the same
 * (insb/insw/insl/outsb/outsw/outsl). You can also use "pausing"
 * versions of the single-IO instructions (inb_p/inw_p/..).
 *
 * This file is not meant to be obfuscating: it's just complicated
 * to (a) handle it all in a way that makes gcc able to optimize it
 * as well as possible and (b) trying to avoid writing the same thing
 * over and over again with slight variations and possibly making a
 * mistake somewhere.
 */

/*
 * Thanks to James van Artsdalen for a better timing-fix than
 * the two short jumps: using outb's to a nonexistent port seems
 * to guarantee better timings even on fast machines.
 *
 * On the other hand, I'd like to be sure of a non-existent port:
 * I feel a bit unsafe about using 0x80 (should be safe, though)
 *
 *		Linus
 */

 /*
  *  Bit simplified and optimized by Jan Hubicka
  */

#ifdef SLOW_IO_BY_JUMPING
#define __SLOW_DOWN_IO "\njmp 1f\n1:\tjmp 1f\n1:"
#else
#define __SLOW_DOWN_IO "\noutb %%al,$0x80"
#endif

#ifdef REALLY_SLOW_IO
#define __FULL_SLOW_DOWN_IO __SLOW_DOWN_IO __SLOW_DOWN_IO __SLOW_DOWN_IO __SLOW_DOWN_IO
#else
#define __FULL_SLOW_DOWN_IO __SLOW_DOWN_IO
#endif

/*
 * Talk about misusing macros..
 */
#define __OUT1(s,x) \
extern inline void out##s(unsigned x value, unsigned short port) {

#define __OUT2(s,s1,s2) \
__asm__ __volatile__ ("out" #s " %" s1 "0,%" s2 "1"

#define __OUT(s,s1,x) \
__OUT1(s,x) __OUT2(s,s1,"w") : : "a" (value), "Nd" (port)); } \
__OUT1(s##_p,x) __OUT2(s,s1,"w") __FULL_SLOW_DOWN_IO : : "a" (value), "Nd" (port));} \

#define __IN1(s) \
extern inline RETURN_TYPE in##s(unsigned short port) { RETURN_TYPE _v;

#define __IN2(s,s1,s2) \
__asm__ __volatile__ ("in" #s " %" s2 "1,%" s1 "0"

#define __IN(s,s1,i...) \
__IN1(s) __IN2(s,s1,"w") : "=a" (_v) : "Nd" (port) ,##i ); return _v; } \
__IN1(s##_p) __IN2(s,s1,"w") __FULL_SLOW_DOWN_IO : "=a" (_v) : "Nd" (port) ,##i ); return _v; } \

#define __INS(s) \
extern inline void ins##s(unsigned short port, void * addr, unsigned long count) \
{ __asm__ __volatile__ ("rep ; ins" #s \
: "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count)); }

#define __OUTS(s) \
extern inline void outs##s(unsigned short port, const void * addr, unsigned long count) \
{ __asm__ __volatile__ ("rep ; outs" #s \
: "=S" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count)); }

#define RETURN_TYPE unsigned char
__IN(b,"")
#undef RETURN_TYPE
#define RETURN_TYPE unsigned short
__IN(w,"")
#undef RETURN_TYPE
#define RETURN_TYPE unsigned int
__IN(l,"")
#undef RETURN_TYPE

__OUT(b,"b",char)
__OUT(w,"w",short)
__OUT(l,,int)

__INS(b)
__INS(w)
__INS(l)

__OUTS(b)
__OUTS(w)
__OUTS(l)

/* The following from linux: include/asm/msr.h
 *  this allows reading and writing machine specific
 *  registers.
 */

#define rdmsr(msr,val1,val2) \
     __asm__ __volatile__("rdmsr" \
			  : "=a" (val1), "=d" (val2) \
			  : "c" (msr))

#define wrmsr(msr,val1,val2) \
     __asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" (msr), "a" (val1), "d" (val2))

#define rdtsc(low,high) \
     __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

#define rdtscl(low) \
     __asm__ __volatile__("rdtsc" : "=a" (low) : : "edx")

#define rdtscll(val) \
     __asm__ __volatile__("rdtsc" : "=A" (val))

#define write_tsc(val1,val2) wrmsr(0x10, val1, val2)

/* THE END */
