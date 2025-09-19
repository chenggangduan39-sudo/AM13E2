#include "qtk/serde/qtk_serde_msgpack.h"
#include "qtk/core/qtk_io.h"

static void pack_str_(uint32_t len, void *upval, qtk_io_writer w) {
    if (len < 32) {
        unsigned char d = 0xa0 | (uint8_t)len;
        qtk_io_writen(upval, w, (char *)&d, 1);
    } else if (len < 256) {
        unsigned char buf[2];
        buf[0] = 0xd9;
        buf[1] = (uint8_t)len;
        qtk_io_writen(upval, w, (char *)buf, 2);
    } else if (len < 65536) {
        unsigned char buf[3];
        buf[0] = 0xda;
        buf[1] = ((uint8_t *)&len)[1];
        buf[2] = ((uint8_t *)&len)[0];
        qtk_io_writen(upval, w, (char *)buf, 3);
    } else {
        unsigned char buf[5];
        buf[0] = 0xdb;
        buf[1] = ((uint8_t *)&len)[3];
        buf[2] = ((uint8_t *)&len)[2];
        buf[3] = ((uint8_t *)&len)[1];
        buf[4] = ((uint8_t *)&len)[0];
        qtk_io_writen(upval, w, (char *)buf, 5);
    }
}

int qtk_serde_msgpack_pack_map(uint32_t n, void *upval, qtk_io_writer w) {
    if (n < 16) {
        unsigned char d = 0x80 | (uint8_t)n;
        qtk_io_writen(upval, w, (char *)&d, 1);
    } else if (n < 65536) {
        unsigned char buf[3];
        buf[0] = 0xde;
        buf[1] = ((uint8_t *)&n)[1];
        buf[2] = ((uint8_t *)&n)[0];
        qtk_io_writen(upval, w, (char *)buf, 3);
    } else {
        unsigned char buf[5];
        buf[0] = 0xdf;
        buf[1] = ((uint8_t *)&n)[3];
        buf[2] = ((uint8_t *)&n)[2];
        buf[3] = ((uint8_t *)&n)[1];
        buf[4] = ((uint8_t *)&n)[0];
        qtk_io_writen(upval, w, (char *)buf, 5);
    }
    return 0;
}

int qtk_serde_msgpack_pack_str(const char *data, uint32_t len, void *upval,
                               qtk_io_writer w) {
    pack_str_(len, upval, w);
    return qtk_io_writen(upval, w, (char *)data, len);
}

int qtk_serde_msgpack_pack_array(size_t n, void *upval, qtk_io_writer w) {

    if (n < 16) {
        unsigned char d = 0x90 | (uint8_t)n;
        qtk_io_writen(upval, w, (char *)&d, 1);
    } else if (n < 65536) {
        unsigned char buf[3];
        buf[0] = 0xdc;
        buf[1] = ((uint8_t *)&n)[1];
        buf[2] = ((uint8_t *)&n)[0];
        qtk_io_writen(upval, w, (char *)buf, 3);
    } else {
        unsigned char buf[5];
        buf[0] = 0xdd;
        buf[1] = ((uint8_t *)&n)[3];
        buf[2] = ((uint8_t *)&n)[2];
        buf[3] = ((uint8_t *)&n)[1];
        buf[4] = ((uint8_t *)&n)[0];
        qtk_io_writen(upval, w, (char *)buf, 5);
    }
    return 0;
}

int qtk_serde_msgpack_pack_float(float f, void *upval, qtk_io_writer w) {
    unsigned char buf[5];
    buf[0] = 0xca;
    buf[1] = ((uint8_t *)&f)[3];
    buf[2] = ((uint8_t *)&f)[2];
    buf[3] = ((uint8_t *)&f)[1];
    buf[4] = ((uint8_t *)&f)[0];
    qtk_io_writen(upval, w, (char *)buf, 5);
    return 0;
}

int qtk_serde_msgpack_pack_bin(const void *d, uint32_t len, void *upval,
                               qtk_io_writer w) {

    if (len < 256) {
        unsigned char buf[2];
        buf[0] = 0xc4;
        buf[1] = (uint8_t)len;
        qtk_io_writen(upval, w, (char *)buf, 2);
    } else if (len < 65536) {
        unsigned char buf[3];
        buf[0] = 0xc5;
        buf[1] = ((uint8_t *)&len)[1];
        buf[2] = ((uint8_t *)&len)[0];
        qtk_io_writen(upval, w, (char *)buf, 3);
    } else {
        unsigned char buf[5];
        buf[0] = 0xc6;
        buf[1] = ((uint8_t *)&len)[3];
        buf[2] = ((uint8_t *)&len)[2];
        buf[3] = ((uint8_t *)&len)[1];
        buf[4] = ((uint8_t *)&len)[0];
        qtk_io_writen(upval, w, (char *)buf, 5);
    }
    qtk_io_writen(upval, w, (char *)d, len);
    return 0;
}

int qtk_serde_msgpack_pack_int32(int32_t d, void *upval, qtk_io_writer w) {
    if (d < -(1 << 5)) {
        if (d < -(1 << 15)) {
            unsigned char buf[5];
            buf[0] = 0xd2;
            buf[1] = ((uint8_t *)&d)[3];
            buf[2] = ((uint8_t *)&d)[2];
            buf[3] = ((uint8_t *)&d)[1];
            buf[4] = ((uint8_t *)&d)[0];
            qtk_io_writen(upval, w, (char *)buf, 5);
        } else if (d < -(1 << 7)) {
            unsigned char buf[3];
            buf[0] = 0xd1;
            buf[1] = ((uint8_t *)&d)[1];
            buf[2] = ((uint8_t *)&d)[0];
            qtk_io_writen(upval, w, (char *)buf, 3);
        } else {
            unsigned char buf[2] = {0xd0, ((uint8_t *)&d)[0]};
            qtk_io_writen(upval, w, (char *)buf, 2);
        }
    } else if (d < (1 << 7)) {
        qtk_io_writen(upval, w, (char *)&d, 1);
    } else {
        if (d < (1 << 8)) {
            unsigned char buf[2] = {0xcc, ((uint8_t *)&d)[0]};
            qtk_io_writen(upval, w, (char *)buf, 2);
        } else if (d < (1 << 16)) {
            unsigned char buf[3];
            buf[0] = 0xcd;
            buf[1] = ((uint8_t *)&d)[1];
            buf[2] = ((uint8_t *)&d)[0];
            qtk_io_writen(upval, w, (char *)buf, 3);
        } else {
            unsigned char buf[5];
            buf[0] = 0xce;
            buf[1] = ((uint8_t *)&d)[3];
            buf[2] = ((uint8_t *)&d)[2];
            buf[3] = ((uint8_t *)&d)[1];
            buf[4] = ((uint8_t *)&d)[0];
            qtk_io_writen(upval, w, (char *)buf, 5);
        }
    }
    return 0;
}
