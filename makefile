shell=bash
pwd=$(shell pwd)

#===========================================
use_arm=0
use_engine=0
use_test=0
use_asr=0
use_kvad=0
use_semdlg=0
use_vad=0
use_wakeup=0
use_wdec=0


ifeq ($(use_arm),0)
CC=gcc
CXX=g++
AR=ar
STRIP=strip
else
BIN_PATH=/home/why/tool/arm_tools/r311/toolchain-sunxi-musl/toolchain/bin
CC=${BIN_PATH}/arm-openwrt-linux-gcc
CXX=${BIN_PATH}/arm-openwrt-linux-g++
AS=${BIN_PATH}/arm-openwrt-linux-as
STRIP=${BIN_PATH}/arm-openwrt-linux-strip
endif


#===================== lib ===========================

sharelib=libs/android/libQvoice.so
EXEC=example

ifeq (${use_engine}, 1)
	exename = engine
else
	exename = module
endif

ifeq ($(use_asr)_$(use_vad)_$(use_kvad)_$(use_semdlg)_$(use_wakeup)_$(use_wdec), 0_0_0_0_0_0)
$(warning "=====================================")
endif

#===========================================
INC= -I . -I ./include 

WFLAGS=
CFLAGS= $(INC) -O3 -fpic -pie -fPIE

LDFLAGS= -pthread 
ifeq ($(use_arm), 1)
LDFLAGS+=-ldl -Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -lpthread -lm -lasound
endif

%.o:%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c -o $@ $<

all: $(EXEC) $(objs)

$(EXEC):
	${CC} $(CFLAGS) $(WFLAGS) -Wall -c ${exename}.c -o ${exename}.o
	
ifeq ($(use_arm),1)
	${CXX} $(CFLAGS) $(WFLAGS) -Wall -o ${EXEC} ${exename}.o -ldl ${LDFLAGS}
endif

clean:
	rm -rf $(test_c_objs) $(EXEC) *.o

.PHONY: all clean

