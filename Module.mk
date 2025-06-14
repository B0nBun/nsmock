obj-m = nsmock.o
nsmock-y += nsmock_main.o netdev.o sysfs.o
ccflags-y := -Wall -Wno-macro-redefined

PWD = $(shell pwd)

all:
	make -C /lib/modules/$(shell uname -r)/build M="$(PWD)" modules
