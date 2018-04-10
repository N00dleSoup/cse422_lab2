# TODO: Change this to the location of your kernel source code
KERNEL_SOURCE=/tmp/compile/nathan.jarvis/linux_source/linux

obj-m := primes.o csv_primes.o atomic_primes.o csv_atomic_primes.o sundaram.o csv_sundaram.o

all:
	$(MAKE) -C $(KERNEL_SOURCE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- SUBDIRS=$(PWD) clean 
