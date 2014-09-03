#
# Makefile one driver
#

TARGET = intn_sysfs

ifneq ($(KERNELRELEASE),)
# call from kernel build system
obj-m	:= $(TARGET).o

else

KERNELDIR ?= ~/src/linux
PWD       := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules
endif

test:
	gcc -g -Wall -pthread -o test_intn_sysfs test_intn_sysfs.c

clean:
	rm -rf *.o *.ko *~ core .depend *.mod.c .*.cmd .tmp_versions .*.o.d \
	*.order  *.symvers test_intn_sysfs

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
