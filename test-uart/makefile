shell=bash
pwd=$(shell pwd)

#===========================================
CC=gcc
CXX=g++
AR=ar
#===========================================
src_dir = qtk qtk/core 
src_c=$(shell find $(src_dir) -name "*.c")
src_cpp = $(shell find $(src_dir) -name "*.cpp")
c_objs=$(patsubst %.c,%.o,${src_c})
cpp_objs = $(patsubst %.cpp, %.o,$(src_cpp))
objs =$(c_objs) $(cpp_objs)
#===========================================
libs = 
#===========================================

test_c:=test/uart_read2.c
test_c_objs:=$(patsubst %.c,%.o,$(test_c))
EXEC=a.out
libso = libqload.so
#===========================================
INC=-I . -I ./qtk  -I ./qtk/core 
WFLAGS=-Wall
CFLAGS=$(INC) -g #-Werror

LDFLAGS= -lpthread -ludev $(libs)

%.o:%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c -o $@ $<

all: $(EXEC) $(objs)

$(EXEC):$(test_c_objs) $(objs)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(libs)

clean:
	rm -rf $(test_c_objs) $(EXEC) $(objs)

.PHONY: all clean
