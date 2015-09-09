#!/bin/sh
#make ARCH=arm  CROSS_COMPILE=arm-generic-linux-gnueabi- uImage
make ARCH=arm  CROSS_COMPILE=arm-generic-linux-gnueabi- uImage O=../build/

