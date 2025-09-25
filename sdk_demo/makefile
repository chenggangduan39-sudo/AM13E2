shell=bash
pwd=$(shell pwd)

#===========================================
use_arm=0
use_engine=1

ifeq ($(use_arm),0)
CC=gcc
CXX=g++
AR=ar
STRIP=strip
else
BIN_PATH=/home/why/tool/AM32/rk3308_bmc10_host/host/bin

CC=${BIN_PATH}/aarch64-linux-gcc
CXX=${BIN_PATH}/aarch64-linux-g++
AS=${BIN_PATH}/aarch64-linux-as
STRIP=${BIN_PATH}/aarch64-linux-strip
endif


#===================== lib ===========================

sharelib=libs/android/libQvoice.so
EXEC=example

ifeq (${use_engine}, 1)
	exename = engine
else
	exename = module
endif

#===========================================
INC= -I . -I ./include

WFLAGS=#-Wall
CFLAGS= $(INC) -O3 -fpic -pie -fPIE

LDFLAGS= -pthread
ifeq ($(use_arm), 1)
LDFLAGS+=-ldl -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -lpthread -lm -lasound -L libs
endif

%.o:%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c -o $@ $<

all: $(EXEC) $(objs)

$(EXEC):
	${CC} $(CFLAGS) $(WFLAGS) -Wall -c ${exename}.c -o ${exename}.o
	
ifeq ($(use_arm),1)
	${CXX} $(CFLAGS) $(WFLAGS) -Wall -o ${EXEC} ${exename}.o libs/arm/libqvoice.so -ldl ${LDFLAGS}
else
	${CXX} $(CFLAGS) $(WFLAGS) -Wall -o ${EXEC} ${exename}.o libs/linux/libqvoice.so -ldl ${LDFLAGS}
endif

clean:
	rm -rf $(test_c_objs) $(EXEC) *.o

.PHONY: all clean

