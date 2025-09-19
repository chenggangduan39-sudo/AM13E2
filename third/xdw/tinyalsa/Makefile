MY_CC?=arm-rockchip-linux-gnueabihf-gcc

CFLAGS += -Iinclude
CFLAGS += -Wall -Werror

LDFLAGS += -lm -ltinyalsa -L.

all:libtinyalsa tinymix tinycap tinyplay tinypcminfo

SRC=mixer.c pcm.c

libtinyalsa:${SRC}
	$(MY_CC) -O3 -shared -fPIC -o libtinyalsa.so ${SRC} ${CFLAGS}

tinymix:tinymix.c libtinyalsa.so
	$(MY_CC) -o tinymix tinymix.c ${CFLAGS} ${LDFLAGS}

tinypcminfo:tinypcminfo.c libtinyalsa.so
	$(MY_CC) -o tinypcminfo tinypcminfo.c ${CFLAGS} ${LDFLAGS}

tinycap:tinycap.c libtinyalsa.so
	$(MY_CC) -o tinycap tinycap.c ${CFLAGS} ${LDFLAGS}

tinyplay:tinyplay.c libtinyalsa.so
	$(MY_CC) -o tinyplay tinyplay.c ${CFLAGS} ${LDFLAGS}

install:

clean:
	@rm -vf tinycap tinyplay tinymix tinypcminfo *.o *.so *~
