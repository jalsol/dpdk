# Makefile for simple character device driver

obj-m += simple_chardev.o

# Get kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Helper targets for loading/unloading the module
load:
	sudo insmod simple_chardev.ko
	sudo chmod 666 /dev/simple_chardev
	@echo "Module loaded. Device available at /dev/simple_chardev"

unload:
	sudo rmmod simple_chardev
	@echo "Module unloaded"

test:
	@echo "Testing write..."
	@echo "Hello from userspace!" | sudo tee /dev/simple_chardev > /dev/null
	@echo "Testing read..."
	@sudo cat /dev/simple_chardev
	@echo ""
	@echo "Check kernel log with: sudo dmesg | tail -20"

.PHONY: all clean load unload test

