# Makefile for vxworks pci "drivers"

# began 2/24/2005 with stuff from skidoo.

CC = cc386
LD = ld386

# to use the inline stuff with gcc 2.8.1 you cannot have either
#of these things enabled (and you must have -O2 on)
# -pedantic only at least gives a clear error message about it.
# (in Skidoo, I never used ansi or pedantic either).
#CKOPTS = -ansi -pedantic
#CKOPTS = -pedantic
#CKOPTS = -Wstrict-prototypes
CKOPTS =
DEFOPTS = -DCPU=I80486 -DVXWORKS -DPC486 -fno-builtin -nostdinc -O2

ANSIOPTS = $(DEFOPTS) $(CKOPTS) -c
SOPTS = $(DEFOPTS) -S -O
EOPTS = $(DEFOPTS) -E -O

VX_INCLUDE = -I$(WIND_BASE)/target/h

ACC = $(CC) $(VX_INCLUDE) $(ANSIOPTS)
SCC = $(CC) $(VX_INCLUDE) $(SOPTS)
ECC = $(CC) $(VX_INCLUDE) $(EOPTS)
#VCC = $(CC) $(VX_INCLUDE) -fvolatile $(ANSIOPTS)

.c.o:
	$(ACC) $<

.c.s:
	$(SCC) $<

.c.e:
	$(ECC) $<

# ---------------------------------

all:	vxtarget

# ---------------------------------

OBJS = pci.o tsc.o x86_reset.o dac6703.o ik220.o

vxtarget:
	vxmake target

target: $(OBJS)

clean:
	rm -f $(OBJS)

install: vxtarget
#	scp x86_reset.o hacksaw:/home/vwuser/Vwstuff/Mount_x86
#	scp pci.o hacksaw:/home/vwuser/Vwstuff/Mount_x86
#	scp dac6703.o hacksaw:/home/vwuser/Vwstuff/Mount_x86
	scp ik220.o cholla:/home/vwuser/Mount

# THE END
