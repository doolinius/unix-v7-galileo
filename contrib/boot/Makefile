A86 = /usr/own/lib/pcc/a86
AS = /usr/own/bin/as
CCOM = /usr/own/lib/pcc/ccom
CCP = /usr/bin/cpp
LD = /usr/own/bin/ld

boot: boot.x
	dd if=boot.x of=boot bs=1 skip=32

boot.x: m16.o boot.b crt.o
	$(LD) -N -s -T 8000 -o boot.x m16.o boot.b crt.o

m16.o: m16.s
	$(AS) -o m16.o m16.s

crt.o: crt.s
	$(AS) -o crt.o crt.s

.SUFFIXES:	.b .c

.c.b:
	$(CPP) -I. ${.PREFIX}.c > ${.PREFIX}.i
	$(CCOM) < ${.PREFIX}.i > ${.PREFIX}.a86
	$(A86) ${.PREFIX}

clean:
	rm -f boot boot.x m16.o boot.b crt.o boot.a86 boot.i
