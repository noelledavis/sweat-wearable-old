# Wearable Circuits Documentation

### Table of Contents
1. [What's in the repo?](#toolchain)
2. [Programming Toolchain](#programming)

### What's in the repo?

This repo contains the board designs, firmware, and accompanying documentation for the Eric 4.0 and for the iontophoresis/sweat analysis projects. Documentation for each design is placed in the respective folder, and includes design notes, component datasheets, and programming instructions.

### Programming Toolchain

All of the firmware is written in C++ using the Arduino framework, compiled for the ATMega328P running at 16 MHz on a 5V supply. All of the code can be viewed/edited/compiled in the Arduino IDE (just copy the source into a new sketch).

I have been using [platformio](http://platformio.org/) as my programming toolchain for firmware, and the repo is currently set up for easy compilation and programming using platformio, GNU Make, and the Unix command line. To take advantage of this, install platformio and make sure that the board "pro16MHzatmega328" is installed. After installing platformio, you can use Make to compile the code and to program the microcontroller directly from the command line.
