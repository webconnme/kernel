make ARCH=arm  CROSS_COMPILE=arm-generic-linux-gnueabi- menuconfig
make ARCH=arm  CROSS_COMPILE=arm-generic-linux-gnueabi- uImage
usb-downloader -t nxp2120 -n /opt/NSIH/DTK.700.350.175.087-192.NSIH.txt -f arch/arm/boot/uImage -a 81000000

