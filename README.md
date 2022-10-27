![LisaEm Logo](resources/lisaem-banner.png)

### Copyright © 2022 by Ray Arachelian, All Rights Reserved.
### Released under the terms of the GNU Public License v3.

------------------------------------------------------------------------------
# Lisa Emulator Unstable Branch ReadMe
 
* [https://lisaem.sunder.net/]() 
* [https://lisafaq.sunder.net]() 
* [https://lisalist2.com]() 
* [https://github.com/rayarachelian/lisaem]()

------------------------------------------------------------------------------

## Note

Sadly, back in Feb 2022, I've been diagnosed with stage 4 cancer, my odds aren't great. I'll do what I can to finalize this release, but I may need help from the community to get help with maintaining LisaEm and future work. It's possible, though the odds are low, that I'll make it out ok. There's always hope, right?

## What is this UNSTABLE thing?

If you're looking at the UNSTABLE branch for the source code for LisaEm 1.2.7: 

This branch is for the impatient who have some reason to live dangerously and want to see the new features before the next RC or Release.

Expect bugs, compilation failures, and all sorts of bad behavior.

Think of this code as you, bringing in a wild wolf into your house, and trying to teach it be a dog.

It will shred your couch, eat your cat and kids, and pee on your rug, and it will bite you despite your attempts to feed it.

It will drink and smash all your 30 year old Auchentoshan and Laphroig bottles and then burp fire.

If you're still up for trying to ride the wolf the same way Harold and Kumar did with the Cheetah, by all means, go for it! 

This is *not* a release, it's a nightmare that you'll have to build it yourself at great risk to yourself, your machines, and sanity.

Bug reports are welcome, but most likely I'm already well aware.

Arroooo!

## What is the livedev branch thing?

If you're looking at the livedev branch, as opposed to the UNSTABLE branch:

Similar to the UNSTABLE branch, the livedev branch is even more unstable as it's the more up to date, latest, buggier, set of edits I'm working on, I plan to update this branch with any significant changes I make.
This branch is meant as insurance, a personal backup, and also to allow contributors to get the very latest stuff.

However, it is not even guaranteed to compile, and it may have junk in it that shouldn't see the light of day.

Why? Well, if I kick the bucket tomorrow, at least whatever I was working on can be picked up by whomever wants to take up the torch.


##  [Read the actual README.md as well](README-for-release.md)


## What is this thing?

LisaEm is a emulator that runs on a wide variety of modern and somewhat old systems by means of the wxWidgets framework, implementing an emulation of the (in)famous Apple Lisa Computer. The Apple Lisa computer is the predecessor of the Mac and the "inspiration" of many GUI environments from Windows 1.x to GEOS, GEM, VisiOn, etc. 

Many of the original ideas of the modern GUI, including copy and paste, were first envisioned in the Apple Lisa, so it's a historically very important machine. The seeds of these ideas were first found in the Xerox Alto, but the Lisa took them to their next logical step.

This document contains a few brief updates that are also found in the ChangeLog file, as well as some information on how to compile wxWidgets and LisaEm.

(The updates will go away once the bugs are removed as ChangeLog is better suited for them.)

## How do you pronounce LisaEm

The "Em" at the end of LisaEm is short for emulator, so therefore you say "Lisa" followed by the first syllable in Emulator, "Em", or if you're a native English speaker, sound out the letter M.
so: lee·suh·em,  \ ˈlē·sə·em \

Nomenclature wise, LisaEm is in line with the other Lisa related things such as Lisa Pascal Workshop, or Lisa Office System, or LisaWrite, or LisaList.


## Command Line Options

LisaEm accepts the following command line options which can be used to customize it in various situations such as running in a Kiosk mode on a Raspberry Pi inside a 3D printed Apple Lisa case, or for an automation pipeline:

```
Usage: lisaem [-h] [-p] [-q] [-f <str>] [-d] [-F[-]] [-z <double>] [-s[-]] [-c <str>] [-k] [-o[-]]
  -h, --help            show this help message
  -p, --power           power on as soon as LisaEm is launched
  -q, --quit            quit after Lisa shuts down
  -f, --floppy=<str>    boot from which floppy image ROMless only
  -d, --drive           boot from motherboard ProFile/Widget ROMless only
  -F, --fullscreen      fullscreen mode (-F- to turn off)
  -z, --zoom=<double>   set zoom level (0.50, 0.75, 1.0, 1.25,... 3.0)
  -s, --skin            turn on skin (-s- or --skin-- to turn off)
  -c, --config=<str>    Open which lisaem config file
  -k, --kiosk           kiosk mode (suitable for RPi Lisa case)
  -o, --originctr       skinless mode: center video(-o) vs topleft(-o-)
```
