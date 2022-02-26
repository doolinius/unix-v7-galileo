# unix-v7-galileo
A UNIX v7/x86 distribution for the Intel Galileo based on Robert Nordier's x86 port. 

## Planned Tasks
It would be great to get as much of the Galileo hardware working in v7 UNIX as possible, but this is obviously quite a long term challenge.  But some of these include:

* TCP/IP networking (from BSD Net/2)
* GPIO
    * Expose in v7 as /dev/gpio/ devices ?
* Additions and Modernization (done on a separate branch once the original is ported)
    * WWB (Writer's Work Bench; comes with v10 distributions)
    * More modern shell
    * shutdown commands
    * Modern df output
    * 'which'
    * Some kind of LISP (2.9 BSD has one)
    * I2C, USB, SPI, PCIe, even?  
    * ZORK!

## Milestones

1. Build and run a simple "bare metal" Hello World program on the Galileo, such as blinking an LED or printing a message to the UART. Printing messages over serial is the eventual goal regardless.
2. Boot the v7 kernel with the inevitable error messages going to the UART.
3. Network boot the v7 kernel (to allow for a faster development cycle)
4. Add the SD card driver to the kernel so it can read the filesystem
5. Boot into single user mode
6. Boot to a shell
7. GPIO
8. Keep on rockin'

## Directory Structure

```bash
├── bin
├── boot
├── contrib
│   ├── 8086
│   ├── boot
├── dev
├── etc
├── hd0unix
├── hd1unix
├── lib
├── md0unix
├── notes
├── README.md
├── structure.txt
└── usr
    ├── bin
    ├── boot
    ├── contrib
    ├── games
    ├── include
    ├── lib
    ├── pub
    ├── release
    ├── spool
    ├── src
    │   ├── cmd
    │   ├── games
    │   ├── libc
    │   ├── libdbm
    │   ├── libm
    │   ├── libmp
    │   └── libplot
    ├── sys
    │   ├── conf
    │   ├── dev
    │   ├── h
    │   └── sys
    └── ucb
```