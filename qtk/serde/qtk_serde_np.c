#include "qtk/serde/qtk_serde_np.h"
#include "qtk/core/qtk_type.h"
#include "wtk/core/wtk_alloc.h"
#include <stdarg.h>

#include <ctype.h>

static int _check_magic(unsigned char magic[6]) {
    return magic[0] == 0x93 && magic[1] == 'N' && magic[2] == 'U' &&
                   magic[3] == 'M' && magic[4] == 'P' && magic[5] == 'Y'
               ? 0
               : -1;
}

static qtk_serde_val_type_t _get_val_type(int elem_sz, char desc) {
    if (desc == 'f' && elem_sz == 4) {
        return QBL_SERDE_VAL_FLOAT32;
    }
    if (desc == 'i') {
        if (elem_sz == 4) {
            return QBL_SERDE_VAL_INT32;
        }
        if (elem_sz == 1) {
            return QBL_SERDE_VAL_INT8;
        }
        if (elem_sz == 2) {
            return QBL_SERDE_VAL_INT16;
        }
    }
    return QBL_SERDE_VAL_UNKNOWN;
}

static int type2elme_sz_(qtk_serde_val_type_t t) {
    switch (t) {
    case QBL_SERDE_VAL_FLOAT32:
        return 4;
    case QBL_SERDE_VAL_INT32:
        return 4;
    case QBL_SERDE_VAL_INT8:
        return 1;
    case QBL_SERDE_VAL_UINT8:
        return 1;
    case QBL_SERDE_VAL_INT64:
        return 8;
    case QBL_SERDE_VAL_INT16:
        return 2;
    case QBL_SERDE_VAL_UNKNOWN:
        return -1;
    }
    return -1;
}

static const char *type2desc_(qtk_serde_val_type_t t) {
    switch (t) {
    case QBL_SERDE_VAL_FLOAT32:
        return "<f4";
    case QBL_SERDE_VAL_INT32:
        return "<i4";
    case QBL_SERDE_VAL_INT64:
        return "<i8";
    case QBL_SERDE_VAL_INT8:
        return "|i1";
    case QBL_SERDE_VAL_UINT8:
        return "|u1";
    case QBL_SERDE_VAL_INT16:
        return "|i2";
    case QBL_SERDE_VAL_UNKNOWN:
        return NULL;
    }
    return NULL;
}

static int _get_val_size(qtk_serde_np_t *np) {
    int elem_sz, shape, i;
    elem_sz = type2elme_sz_(np->hdr.val_t);
    if (np->hdr.shape[0] > 0) {
        np->nmemb = 1;
        for (i = 0; (shape = np->hdr.shape[i]) > 0; i++) {
            np->nmemb *= shape;
        }
    } else {
        np->nmemb = 0;
    }
    return elem_sz * np->nmemb;
}

static int _parse_hdr(qtk_serde_np_t *np) {
    int elem_sz;
    char *s, *e, *digit = NULL;
    int len, i, shape_idx = 0;
    char buf[1024] = {0};

    s = strstr(np->tmp_buf->data, "descr");
    s = strchr(s, ':') + 1;
    s = strchr(s, '\'') + 1;
    e = strchr(s, '\'');
    len = e - s;
    memcpy(buf, s, len);
    buf[len] = '\0';
    elem_sz = buf[2] - '0';
    qtk_assert(len == 3 && (buf[0] == '<' || buf[0] == '|'));
    np->hdr.val_t = _get_val_type(elem_sz, buf[1]);
    qtk_assert(np->hdr.val_t != QBL_SERDE_VAL_UNKNOWN);

    s = strstr(np->tmp_buf->data, "fortran_order");
    s = strchr(s, ':') + 1;
    for (; isspace(*s); s++)
        ;
    e = strchr(s, ',');
    len = e - s;
    memcpy(buf, s, len);
    buf[len] = '\0';
    qtk_assert(0 == strcmp(buf, "False"));

    s = strstr(np->tmp_buf->data, "'shape'");
    s = strchr(s, '(') + 1;
    e = strchr(s, ')');
    len = e - s;
    memcpy(buf, s, len);
    buf[len] = '\0';

    for (i = 0; i < len; i++) {
        if (isspace(buf[i])) {
            continue;
        }
        if (isdigit(buf[i])) {
            if (digit == NULL) {
                digit = buf + i;
            }
        } else if (buf[i] == ',') {
            buf[i] = '\0';
            np->hdr.shape[shape_idx++] = atoi(digit);
            digit = NULL;
        }
    }

    if (digit) {
        np->hdr.shape[shape_idx++] = atoi(digit);
    }
    np->hdr.shape[shape_idx] = -1;

    return 0;
}

/* https://numpy.org/doc/stable/reference/generated/numpy.lib.format.html#module-numpy.lib.format
 * */
void *qtk_serde_np_load(qtk_serde_np_t *np, qtk_io_reader r, void *upval) {
    int val_sz;
    void *val;
    unsigned char magic[6];
    unsigned short hdr_len;
    qtk_serde_np_hdr_t *hdr = &np->hdr;
    qtk_io_readn(upval, r, cast(char *, magic), sizeof(magic));
    if (_check_magic(magic) != 0) {
        return NULL;
    }
    qtk_io_readn(upval, r, cast(char *, &hdr->major), sizeof(hdr->major));
    qtk_io_readn(upval, r, cast(char *, &hdr->minor), sizeof(hdr->minor));
    if (hdr->major == 2) {
        qtk_io_readn(upval, r, cast(char *, &hdr->hdr_len),
                     sizeof(hdr->hdr_len));
    } else {
        qtk_io_readn(upval, r, cast(char *, &hdr_len), sizeof(hdr_len));
        hdr->hdr_len = hdr_len;
    }

    wtk_strbuf_reset(np->tmp_buf);
    wtk_strbuf_expand(np->tmp_buf, hdr->hdr_len);
    qtk_io_readn(upval, r, np->tmp_buf->data, hdr->hdr_len);
    np->tmp_buf->pos += hdr->hdr_len;
    wtk_strbuf_push_c(np->tmp_buf, '\0');
    _parse_hdr(np);
    val_sz = _get_val_size(np);
    if (val_sz <= 0) {
        return NULL;
    }
    val = wtk_malloc(val_sz);
    qtk_io_readn(upval, r, cast(char *, val), val_sz);

    return val;
}

int qtk_serde_np_init(qtk_serde_np_t *np) {
    np->tmp_buf = wtk_strbuf_new(256, 1);
    return 0;
}

void qtk_serde_np_reset(qtk_serde_np_t *np) { wtk_strbuf_reset(np->tmp_buf); }

int qtk_serde_np_clean(qtk_serde_np_t *np) {
    if (np->tmp_buf) {
        wtk_strbuf_delete(np->tmp_buf);
        np->tmp_buf = NULL;
    }
    return 0;
}

int qtk_serde_np_save1(void *data, qtk_serde_val_type_t type, qtk_io_writer w,
                       void *upval, int *shape, int rank) {
    int tot_elems = 1;
    int tot_bytes;
    char buf1[512] = {0};
    int offset = 0, hdr_offset;
    int hdr_round_len;
    uint16_t sz_v1;
    char buf2[1024] = {0x93, 'N', 'U', 'M', 'P', 'Y', 1, 0}; // version 1
    hdr_offset = 10;

    if (shape[0] == 0) {
        offset += snprintf(buf1 + offset, sizeof(buf1) - offset, "%d,", 0);
        tot_bytes = 0;
    } else {
        tot_elems = 1;
        for (int i = 0; i < rank; i++) {
            tot_elems *= shape[i];
            offset +=
                snprintf(buf1 + offset, sizeof(buf1) - offset, "%d,", shape[i]);
        }
        tot_bytes = type2elme_sz_(type) * tot_elems;
        qtk_assert(tot_bytes > 0);
    }

    hdr_offset += sz_v1 =
        snprintf(buf2 + hdr_offset, sizeof(buf2) - hdr_offset,
                 "{'descr':'%s','fortran_order':False,'shape':(%s),}",
                 type2desc_(type), buf1);
    hdr_round_len = wtk_round(hdr_offset + 1, 64);
    qtk_assert(hdr_round_len <= 1024);
    sz_v1 += hdr_round_len - hdr_offset;
    memset(buf2 + hdr_offset, ' ', hdr_round_len - hdr_offset);
    memcpy(buf2 + 8, &sz_v1, 2);

    buf2[hdr_round_len - 1] = '\n';
    qtk_io_writen(upval, w, buf2, hdr_round_len);
    if (tot_bytes > 0) {
        qtk_io_writen(upval, w, data, tot_bytes);
    }

    return 0;
}

int qtk_serde_np_save(void *data, qtk_serde_val_type_t type, qtk_io_writer w,
                      void *upval, int shape, ...) {
    va_list valist;
    int shape_a[32] = {0};
    int rank = 0;

    va_start(valist, shape);
    if (shape > 0) {
        shape_a[rank++] = shape;
        for (; 1;) {
            int dim = va_arg(valist, int);
            if (dim <= 0) {
                break;
            }
            shape_a[rank++] = dim;
        }
    }

    return qtk_serde_np_save1(data, type, w, upval, shape_a, rank);
}
