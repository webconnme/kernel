#!/bin/sh
#make ARCH=arm  CROSS_COMPILE=arm-generic-linux-gnueabi- menuconfig 
make ARCH=arm  CROSS_COMPILE=arm-generic-linux-gnueabi- menuconfig O=../build/
