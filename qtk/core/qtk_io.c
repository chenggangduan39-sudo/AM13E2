#include "qtk/core/qtk_io.h"
#include "qtk/core/qtk_type.h"

int qtk_io_copy(void *rval, qtk_io_reader r, void *wval, qtk_io_writer w) {
    int len, ret;
    char buf[4096];
    for (;;) {
        len = r(rval, buf, sizeof(buf));
        if (len < 0) {
            break;
        }
        ret = w(wval, buf, len);
        if (ret != len) {
            goto err;
        }
    }
    return 0;
err:
    return -1;
}

int qtk_io_getc(void *rval, qtk_io_reader r) {
    int ret;
    char c;
    ret = r(rval, &c, 1);
    return ret != 1 ? -1 : cast(unsigned char, c);
}

int qtk_io_readn(void *rval, qtk_io_reader r, char *d, int n) {
    int ret;

    while (n > 0) {
        ret = r(rval, d, n);
        if (ret < 0) {
            goto err;
        }
        d += ret;
        n -= ret;
    }

    return 0;
err:
    return -1;
}

int qtk_io_writen(void *wval, qtk_io_writer w, char *d, int n) {
    int ret;

    while (n > 0) {
        ret = w(wval, d, n);
        if (ret < 0) {
            goto err;
        }
        d += ret;
        n -= ret;
    }
    return 0;
err:
    return -1;
}
