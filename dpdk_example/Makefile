# Makefile for DPDK UDP multicast receiver

# Binary name
APP = udp_multicast_receiver

# Source files
SRCS = udp_multicast_receiver.c

# Default: use pkg-config (modern DPDK)
.PHONY: all
all:
	gcc -O3 -o $(APP) $(SRCS) \
		$$(pkg-config --cflags --libs libdpdk) \
		-DALLOW_EXPERIMENTAL_API

.PHONY: clean
clean:
	rm -f $(APP) *.o

