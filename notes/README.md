# Project Notes

## Why v7 on the Intel Galileo?

Feb 24, 2021

I was recently reminded of the Xinu operating system, one my OS professor in undergrad would refer to every now and then, as he was a student of Douglas Comer's at Perdue.  Out of curiosity I looked it up and found that it was not only still being used at Perdue, but that the textbook had been updated in 2015 and the primary development platforms were now the Intel Galileo and the Beaglebone Black ARM development board. 

Wait, Intel made a development board? How had I not heard of this? I have a bit of a weakness for small board computers and development boards, so I was surprised I was just now finding out about the Galileo's existence. I then proceeded to look up this board and was immediately intrigued. I don't typically get interested in anything x86, but this was a strange animal. 

* A 400 MHz Quark processor (something like a P3 without MMX)
* 512 MB of RAM
* 100 MB Ethernet
* SD Card slot
* PCIe slot
* USB host and client ports
* A real time clock (but you need a coin battery)
* GPIO pins arranged in the Arduino form factor (so you can use Arduino shields)
* Programmable in the Arduino IDE

What **is** this thing supposed to be? It's far more powerful than an Arduino, but not as powerful as most Raspberry Pi boards. It _seems_ like it's trying to be a Super Arduino, yet it has the hardware to run an Operating System like Xinu. I was too curious and snagged one on ebay for $25. 

### Running Xinu

The original plan for this board was to experiment with Xinu and learn some more about Operating Systems, a topic that I had always enjoyed. I even teach a basic OS class at a community college. However, I always tended to track back to this topic of interest every so often and this seemed like an opportunity to learn even more. 

I followed the steps here:

https://xinu.cs.purdue.edu/files/Xinu_Galileo_Manual_v2.pdf

... but I sadly had no luck getting it to boot. I was getting the following error:

```
error: "prefix" is not set.
WARNING: no console will be available to OSerror: no suitable mode found
```

The Xinu page at Perdue mentioned the notes of a developer named Tom Trebisky who had joined the Xinu project in 2016 and had experience with both the Galileo and Beaglebone Black. After reading his notes, it was pretty clear that he knew what he was about in such matters, so I contacted him.

On the same day I found others who had the same issue who posted it to the real-xinu github page. Thinking that it may be an SD card issue like on old Raspberry Pis, I ordered a couple more. 

### A New Idea Takes Hold

Within 24 hours, a new idea occurred to me.  Several months prior, I had discovered that v7 UNIX had been ported to x86 by Robert Nordier back in the late 90's and polished by the mid-2000's.  I had installed a virtual machine to play around with it.  I also have v7 running on a Raspberry Pi 1, a port portted by Mike van der Westhuizen.

This gave me an idea. Why not port v7/x86 to the Galileo? 

This project brings together many current and previous interests: small board computers, UNIX, retro computing, operating systems and hardware. It would give me the chance to learn about low-level systems programming in a way that I had always wanted to but never got around to, on a small board computer, involving a historic version of my favorite operating system. 

It was the perfect project for me. 