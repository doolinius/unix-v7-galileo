/* V7/x86 source code: see www.nordier.com/v7x86 for details. */
/* Copyright (c) 2006 Robert Nordier.  All rights reserved. */

#define PTMAGIC	0xaa55	/* last two byte of mbr */
#define PTOFF	0x1be	/* partition table offset in mbr */
#define PTBOOT	0x80	/* bootable flag */
#define PTID	0x72	/* V7/x86 partition id */

/* partition table entry */
struct ptent {
	unsigned char bi;	/* boot indicator */
	unsigned char sh;	/* head: start */
	unsigned char ss;	/* sector: start */
	unsigned char sc;	/* cylinder: start */
	unsigned char id;	/* system id */
	unsigned char eh;	/* head: end */
	unsigned char es;	/* sector: end */
	unsigned char ec;	/* cylinder end */
	unsigned long off;	/* offset */
	unsigned long size;	/* size */
};
