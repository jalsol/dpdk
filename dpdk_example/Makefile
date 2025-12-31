# Makefile for DPDK UDP multicast receiver

# Binary name
APP = udp_multicast_receiver

# Source files
SRCS = udp_multicast_receiver.c

# DPDK configuration
ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target on a linux environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# CFLAGS
CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -DALLOW_EXPERIMENTAL_API

# Build using DPDK Makefile
include $(RTE_SDK)/mk/rte.extapp.mk

# Alternative: using pkg-config (for newer DPDK versions)
.PHONY: build-pkgconfig
build-pkgconfig:
	gcc -O3 -o $(APP) $(SRCS) \
		$$(pkg-config --cflags --libs libdpdk) \
		-DALLOW_EXPERIMENTAL_API

.PHONY: clean-all
clean-all:
	rm -f $(APP) *.o

