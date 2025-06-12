obj-m = ifacesim.o
ifacesim-y += ifacesim_main.o netdev.o sysfs.o
ccflags-y := -Wall

PWD = $(shell pwd)

all:
	make -C /lib/modules/$(shell uname -r)/build M="$(PWD)" modules
