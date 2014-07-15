MSP430 Firmware
===
Firmware to control the MSP430.

Environment configuration
---

Installing dependencies:

```sh
sudo apt-get install git-core gcc-4.4 texinfo patch libncurses5-dev zlibc zlib1g-dev libx11-dev libusb-dev libreadline6-dev 
```

Installing compiler and mspdebug:

```sh
sudo apt-get install gcc-msp430 binutils-msp430 msp430-libc mspdebug
```

Now you're ready to compile and deploy to MSP430.

Compiling and deploying
---

Compiling you C code to MSP430:

```sh
msp430-gcc -Os -mmcu=msp430g2553 -o output.elf input.c
```

Running the code on MSP430:

```sh
# Opening mspdebug
sudo mspdebug rf2500
# Deleting the code on the MSP
erase
# Deploying ELF file to MSP
prog led.elf
# Running the code
run
```
