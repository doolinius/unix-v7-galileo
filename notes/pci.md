# How PCI Works

It all starts with the Memory Mapped Configuration space.  

## MMConfig

The Memory Mapped Configuration Space is where you start looking for PCI devices. This is set up by the BIOS, which stores the address and length in ACPI tables. Xinu shortcuts this by using the known values on the Galileo.

This happens to be:

MMC_BASE: 0xe0000000
MMC_LEN:  0x10000000

This memory area will hold the configuration data for all of the PCI devices on the system.

So to do this The Right Way, you would want to read from the ACPI table to get these values.  But since I'm only doing this on the Galileo, it will always be the same.  

## Scanning the PCI Bus

Next, scan the PCI bus.  This is another situation where on a real system, you would not be able to assume a number of buses or devices. A real system can have:

* 256 busses 
* 32 devices per bus
* 8 functions per device

To scan the bus, you would loop through each function, within each device, within each bus (a nested loop). 

Each function is size 0x1000 in memory.  With a little math using bus, device and function number, you can calculate the memory mapped address of a single device's configuration in the MMConfig space.  

So for bus 0, device 14, function 5, this works out to a memory address of:
0xe0075000

This happens to be the address of the UART controller config

## Using the Config Header

Once we have a PCI config header pointing to a valid memory address, we can learn about it.  For example, we can get Vendor IDs (such Intel's 0x8086 ID) and unique device ID. We can also get interrupt pins/lines, power management registers and of particular importance, BAR registers.

Each Config header has 6 Base Address Registers.  Either the BIOS or the OS talks to each PCI device during enumeration and reads how much memory it requires for I/O. It will then allocate the requested memory in its I/O address space and write this address back to the BAR register.  

I think the Galileo's firmware (EDK II, an open source UEFI implementation) does this so the OS doesn't have to. I should read the source code for EDKII on the Galileo.  

## The Galileo's PCI Devices

root@galileo:~# lspci 
00:00.0 Host bridge: Intel Corporation Device 0958
00:14.0 SD Host controller: Intel Corporation Device 08a7 (rev 10)
00:14.1 Serial controller: Intel Corporation Device 0936 (rev 10)
00:14.2 USB controller: Intel Corporation Device 0939 (rev 10)
00:14.3 USB controller: Intel Corporation Device 0939 (rev 10)
00:14.4 USB controller: Intel Corporation Device 093a (rev 10)
00:14.5 Serial controller: Intel Corporation Device 0936 (rev 10)
00:14.6 Ethernet controller: Intel Corporation Device 0937 (rev 10)
00:14.7 Ethernet controller: Intel Corporation Device 0937 (rev 10)
00:15.0 Serial bus controller [0c80]: Intel Corporation Device 0935 (rev 10)
00:15.1 Serial bus controller [0c80]: Intel Corporation Device 0935 (rev 10)
00:15.2 Serial bus controller [0c80]: Intel Corporation Device 0934 (rev 10)
00:17.0 PCI bridge: Intel Corporation Device 11c3
00:17.1 PCI bridge: Intel Corporation Device 11c4
00:1f.0 ISA bridge: Intel Corporation Device 095e

These all have vendor ID 0x8086, the infamous and cleverly chosen value for Intel. The following lists descriptions and chapter in the Quark datasheet:

    Device 0958 is a host bridge - chapter 12
    Device 11C3 is a PCI bridge (PCI Express) - chapter 14
    Device 0937 is the ethernet controller (10/100) - chapter 15 (2 of these)
    Device 0939 is the USB controller (USB 2.0 ehci) - chapter 16 (2 of these)
    Device 093a is the USB controller (USB 2.0 ohci) - chapter 16
    Device 08a7 is the SD/eMMC controller - chapter 17
    Device 0936 is the 16550 uart - chapter 18 (2 of these)
    Device 0935 is the SPI controller - chapter 20 (2 of these)
    Device 0934 is the GPIO/i2c controller - chapter 19
    Device 095E is the "Legacy bridge" - chapter 21 

## Interesting Devices

The most interesting and immediately useful devices listed her are the UART, the GPIO/i2c controller, the SD/eMMC controller and the ethernet controller.  Later on, USB and PCI Express could be cool. 

## What does this mean for v7 UNIX?

It probably means I could find the I/O addresses and hard code them from there, PDP-11 style.  But, it would be cooler to have a real PCI subsystem that scanned for devices and such.  