#
# Makefile one driver
#

TARGET = intn

ifneq ($(KERNELRELEASE),)
# call from kernel build system
obj-m	:= $(TARGET).o

else

KERNELDIR ?= ~/src/linux
PWD       := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules
endif

clean:
	rm -rf *.o *.ko *~ core .depend *.mod.c .*.cmd .tmp_versions .*.o.d \
	*.order  *.symvers

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ins: default rem
	insmod $(TARGET).ko debug=1

rem:
	@if [ -n "`lsmod | grep -s $(TARGET)`" ]; then \
		rmmod $(TARGET); \
		echo "rmmod $(TARGET)"; \
	fi

ifeq (.depend,$(wildcard .depend))
include .depend
endif
