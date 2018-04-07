# TODO: Change this to the location of your kernel source code
KERNEL_SOURCE=/tmp/compile/pgardner/linux_source/linux

obj-m := primes.o

all:
	$(MAKE) -C $(KERNEL_SOURCE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- SUBDIRS=$(PWD) clean 
