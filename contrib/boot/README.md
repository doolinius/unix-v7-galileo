# Second Stage Bootloader

This is the code for Robert Nordier's 2nd Stage bootloader for v7/x86.  It's a 16 bit bootloader that handles switching the CPU to Protected Mode then reads the v7 kernel from either a hard drive or floppy drive. 

This bootloader will likely not be necessary on the Galileo, since we can use the Xinu method of using the Galileo's custom GRUB to chainload GRUB2 from the SD card, which can then boot whatever we want, including a v7 kernel.  

The real use of this code will be to reference it if/when we need to read the kernel and boot from the v7 root partition on the SD card.

Or perhaps we can use this as a base for our own bootloader to replace **bootia32.efi** as Xinu does.

## Building

I have not spent the time yet getting this to build, but it requires the 8086 build tools that were part of the Portable C Compiler (PCC).

See contrib/8086/ for those. 