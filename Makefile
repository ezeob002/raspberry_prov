# The MIT License (MIT)
#
# Copyright (c) 2016 Philippe Proulx <pproulx@efficios.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

BARECTF ?= barectf
RM = rm -rf
MKDIR = mkdir
CC=gcc

PLATFORM_DIR = ../
CFLAGS = -O2 -Wall -pedantic -I$(PLATFORM_DIR) -I.

TARGET = sample_c
OBJS = $(TARGET).o barectf.o barectf-platform-linux-fs.o 

.PHONY: all view

all: $(TARGET)

pi_2_dht_read.o: pi_2_dht_read.c
	$(CC) -c $<

pi_2_dht_mmio.o: pi_2_dht_mmio.c
	$(CC) -c $<

common_dht_read.o: common_dht_read.c
	$(CC) -c $<


ctf:
	$(MKDIR) ctf

$(TARGET): $(OBJS)
	$(CC)  $^ common_dht_read.c pi_2_dht_read.c pi_2_mmio.c -o $@  
ctf/metadata barectf-bitfield.h barectf.h barectf.c: config.yaml ctf
	$(BARECTF) $< -m ctf

barectf.o: barectf.c
	$(CC) $(CFLAGS) -ansi -c $<

barectf-platform-linux-fs.o: $(PLATFORM_DIR)/barectf-platform-linux-fs.c barectf.h
	$(CC) $(CFLAGS) -c $<



clean:
	$(RM) $(TARGET) $(OBJS) ctf
	$(RM) *o
	$(RM) barectf.h barectf-bitfield.h barectf.c
