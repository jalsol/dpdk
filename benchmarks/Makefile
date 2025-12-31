# Makefile for benchmark tools

CC = gcc
CFLAGS = -O3 -Wall -Wextra
LDFLAGS = -lrt

.PHONY: all clean

all: kernel_receiver

kernel_receiver: kernel_receiver.c
	$(CC) $(CFLAGS) -o kernel_receiver kernel_receiver.c $(LDFLAGS)

clean:
	rm -f kernel_receiver *.o

