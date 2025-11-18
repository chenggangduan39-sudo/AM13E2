#include "qtk/serde/qtk_kaldiio.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk/core/qtk_type.h"

typedef enum {
    ap_init,
    ap_prenum,
    ap_num,
    ap_end,
    ap_will_end,
} ascii_parser_state_t;

typedef struct {
    ascii_parser_state_t state;
    unsigned has_bracket : 1;
    unsigned err : 1;
    char num[128];
    int num_pos;
    int col;
    int nelem;
    qtk_kaldiio_t *ki;
} ascii_parser_t;

static int ascii_parser_feed(ascii_parser_t *p, int c) {
    float f;
    int push_float = 0;
    switch (p->state) {
    case ap_init:
        if (c == '[') {
            p->state = ap_prenum;
            p->has_bracket = 1;
        } else {
            if (c != ' ' && c != '\n') {
                if (c == -1) {
                    p->state = ap_end;
                } else {
                    p->num[p->num_pos++] = c;
                    p->state = ap_num;
                }
            }
        }
        break;
    case ap_prenum:
        if (c == '\n' || c == ' ') {
        } else if (c == -1) {
            p->state = ap_end;
        } else {
            p->num[p->num_pos++] = c;
            p->state = ap_num;
        }
        break;
    case ap_num:
        if (p->has_bracket) {
            switch (c) {
            case -1:
                p->err = 1;
                break;
            case ']':
                push_float = 1;
                p->state = ap_will_end;
                break;
            case '\n':
                push_float = 1;
                if (p->col == 0) {
                    p->col = p->nelem + (p->num_pos > 0);
                }
                break;
            case ' ':
                push_float = 1;
                break;
            default:
                p->num[p->num_pos++] = c;
            }
        } else {
            switch (c) {
            case -1:
            case '\n':
                p->state = ap_end;
                push_float = 1;
                break;
            case ' ':
                push_float = 1;
                break;
            default:
                p->num[p->num_pos++] = c;
            }
        }

        if (push_float && p->num_pos > 0) {
            p->num[p->num_pos] = 0;
            f = atof(p->num);
            wtk_strbuf_push(p->ki->buf, cast(char *, &f), sizeof(float));
            p->nelem++;
            p->num_pos = 0;
        }
        break;
    case ap_will_end:
        if (c == '\n' || c == -1) {
            p->state = ap_end;
        } else {
            p->err = 1;
        }
        break;
    case ap_end:
        break;
    }
    return 0;
}

static int kaldiio_ensure_buf_(qtk_kaldiio_t *ki, int sz) {
    wtk_strbuf_reset(ki->buf);
    wtk_strbuf_push(ki->buf, NULL, sz);
    return 0;
}

static int read_token_(qtk_io_reader r, void *upval, char *token, int *len) {
    int cap = *len;
    *len = 0;
    while (1) {
        int c = qtk_io_getc(upval, r);
        if (c == -1 || c == ' ') {
            break;
        }
        if (*len == cap) {
            goto err;
        }
        token[*len] = c;
        (*len)++;
    }

    return 0;
err:
    return -1;
}

static int read_int32_vec_(qtk_kaldiio_t *ki, qtk_io_reader r, void *upval,
                           qtk_kaldiio_elem_t *elem) {
    int32_t len;
    int32_t *data;
    if (qtk_io_readn(upval, r, cast(char *, &len), sizeof(len))) {
        goto err;
    }

    kaldiio_ensure_buf_(ki, sizeof(int32_t) * len);
    data = cast(int32_t *, ki->buf->data);
    if (qtk_io_readn(upval, r, cast(char *, data), len * sizeof(int32_t))) {
        goto err;
    }

    elem->fmt = QBL_KALDIIO_I32V;
    elem->i32v.data = data;
    elem->i32v.len = len;

    return 0;
err:
    return -1;
}

static int load_binary_mode_(qtk_kaldiio_t *ki, qtk_io_reader r, void *upval,
                             qtk_kaldiio_elem_t *elem, char *buf, int buf_len) {
    int len;
    int32_t rows, cols, nelem, elem_sz;
    int is_mat = 0;
    void *data;

    if (qtk_io_readn(upval, r, buf, 1)) {
        goto err;
    }
    if (buf[0] == 4) {
        if (read_int32_vec_(ki, r, upval, elem)) {
            goto err;
        }
    } else {
        if (buf[0] == ' ') {
            goto err;
        }
        len = buf_len - 1;
        if (read_token_(r, upval, buf + 1, &len)) {
            goto err;
        }
        len++;
        switch (len) {
        case 2:
            if (buf[0] == 'F' && buf[1] == 'M') {
                elem_sz = 4;
                is_mat = 1;
                elem->fmt = QBL_KALDIIO_FM;
            } else if (buf[0] == 'F' && buf[1] == 'V') {
                elem_sz = 4;
                is_mat = 0;
                elem->fmt = QBL_KALDIIO_FV;
            }
            break;
        default:
            qtk_assert(0);
        }
        len = qtk_io_getc(upval, r);
        qtk_assert(len == 4);
        if (qtk_io_readn(upval, r, cast(char *, &rows), sizeof(rows))) {
            goto err;
        }
        nelem = rows;
        if (is_mat) {
            len = qtk_io_getc(upval, r);
            qtk_assert(len == 4);
            if (qtk_io_readn(upval, r, cast(char *, &cols), sizeof(cols))) {
                goto err;
            }
            nelem *= cols;
        }
        kaldiio_ensure_buf_(ki, elem_sz * nelem);
        data = ki->buf->data;
        if (qtk_io_readn(upval, r, data, nelem * elem_sz)) {
            goto err;
        }
        switch (elem->fmt) {
        case QBL_KALDIIO_FM:
            elem->fm.col = cols;
            elem->fm.row = rows;
            elem->fm.data = data;
            break;
        case QBL_KALDIIO_FV:
            elem->fv.len = rows;
            elem->fv.data = data;
            break;
        default:
            qtk_assert(0);
        }
    }

    return 0;
err:
    return -1;
}

int qtk_kaldiio_load_ark(qtk_kaldiio_t *ki, qtk_io_reader r, void *upval) {
    char buf[1024];
    char token[1024];
    int len, row;
    qtk_kaldiio_elem_t elem;

    while (1) {
        len = sizeof(token);
        if (read_token_(r, upval, token, &len)) {
            goto err;
        }
        if (len == 0) {
            break;
        }
        elem.token.data = token;
        elem.token.len = len;
        if (qtk_io_readn(upval, r, buf, 1)) {
            goto err;
        }
        if (buf[0] == 0) {
            if (qtk_io_getc(upval, r) != 'B' ||
                load_binary_mode_(ki, r, upval, &elem, buf, sizeof(buf))) {
                goto err;
            }
        } else {
            ascii_parser_t p;
            memset(&p, 0, sizeof(p));
            wtk_strbuf_reset(ki->buf);
            p.ki = ki;
            ascii_parser_feed(&p, buf[0]);
            while (p.state != ap_end && p.err == 0) {
                ascii_parser_feed(&p, qtk_io_getc(upval, r));
            }
            if (p.err) {
                goto err;
            }

            if (p.col == 0) {
                p.col = p.nelem;
            }
            row = p.nelem / p.col;
            if (row > 1) {
                elem.fmt = QBL_KALDIIO_FM;
                elem.fm.data = cast(float *, ki->buf->data);
                elem.fm.row = row;
                elem.fm.col = p.col;
            } else {
                elem.fmt = QBL_KALDIIO_FV;
                elem.fv.data = cast(float *, ki->buf->data);
                elem.fv.len = p.nelem;
            }
        }
        if (ki->notifier(ki->upval, &elem)) {
            break;
        }
    }

    return 0;
err:
    return -1;
}

static int ark_save_int_(qtk_io_writer w, void *upval, int n) {
    char len = 4;
    qtk_io_writen(upval, w, cast(char *, &len), 1);
    qtk_io_writen(upval, w, cast(char *, &n), 4);
    return 0;
}

static int save_ark_binary(qtk_kaldiio_t *ki, qtk_kaldiio_elem_t *elem,
                           qtk_io_writer w, void *upval) {
    int elem_sz, nelem = 0;
    char bin_header[2] = {0, 'B'};
    char *data = NULL;
    qtk_io_writen(upval, w, bin_header, sizeof(bin_header));

    switch (elem->fmt) {
    case QBL_KALDIIO_FM:
        elem_sz = 4;
        nelem = elem->fm.col * elem->fm.row;
        data = cast(char *, elem->fm.data);
        qtk_io_write_strliteral(upval, w, "FM ");
        ark_save_int_(w, upval, elem->fm.row);
        ark_save_int_(w, upval, elem->fm.col);
        break;
    case QBL_KALDIIO_FV:
        elem_sz = 4;
        nelem = elem->fv.len;
        data = cast(char *, elem->fv.data);
        qtk_io_write_strliteral(upval, w, "FV ");
        ark_save_int_(w, upval, elem->fv.len);
        break;
    case QBL_KALDIIO_I32V:
        elem_sz = 4;
        nelem = elem->i32v.len;
        data = cast(char *, elem->i32v.data);
        ark_save_int_(w, upval, nelem);
        break;
    case QBL_KALDIIO_SCP_ITEM:
        goto err;
    }
    qtk_io_writen(upval, w, data, nelem * elem_sz);
    return 0;
err:
    return -1;
}

static int save_ark_text(qtk_kaldiio_t *ki, qtk_kaldiio_elem_t *elem,
                         qtk_io_writer w, void *upval) {
    char buf[128];
    int len;
    switch (elem->fmt) {
    case QBL_KALDIIO_FM:
        qtk_io_write_strliteral(upval, w, "[\n");
        for (int i = 0; i < elem->fm.row; i++) {
            for (int j = 0; j < elem->fm.col; j++) {
                len = snprintf(buf, sizeof(buf), "%f ",
                               elem->fm.data[i * elem->fm.col + j]);
                qtk_io_writen(upval, w, buf, len - (j == elem->fm.col - 1));
            }
            qtk_io_write_strliteral(upval, w,
                                    i == elem->fm.row - 1 ? "]\n" : "\n");
        }
        break;
    case QBL_KALDIIO_FV:
        for (int i = 0; i < elem->fv.len; i++) {
            len = snprintf(buf, sizeof(buf), "%f", elem->fv.data[i]);
            qtk_io_write_strliteral(upval, w,
                                    i == elem->fv.len - 1 ? "\n" : " ");
        }
        break;
    case QBL_KALDIIO_I32V:
        for (int i = 0; i < elem->i32v.len; i++) {
            len = snprintf(buf, sizeof(buf), "%d", elem->i32v.data[i]);
            qtk_io_write_strliteral(upval, w,
                                    i == elem->i32v.len - 1 ? "\n" : " ");
        }
        break;
    case QBL_KALDIIO_SCP_ITEM:
        goto err;
    }
    return 0;
err:
    return -1;
}

int qtk_kaldiio_load_scp(qtk_kaldiio_t *ki, qtk_io_reader r, void *upval) {
    int c;
    char token[1024];
    qtk_kaldiio_elem_t elem;
    while (1) {
        int tok_len = sizeof(token);
        if (read_token_(r, upval, token, &tok_len)) {
            goto err;
        }
        if (tok_len == 0) {
            break;
        }
        while ((c = qtk_io_getc(upval, r)) == ' ')
            ;
        if (c == -1) {
            goto err;
        }
        wtk_strbuf_reset(ki->buf);
        do {
            wtk_strbuf_push_c(ki->buf, c);
            c = qtk_io_getc(upval, r);
        } while (c != '\n' && c != -1);
        if (ki->buf->pos == 0) {
            break;
        }
        wtk_strbuf_push_c(ki->buf, '\0');
        elem.fmt = QBL_KALDIIO_SCP_ITEM;
        elem.token.data = token;
        elem.token.len = tok_len;
        elem.xfilename.data = ki->buf->data;
        elem.xfilename.len = ki->buf->pos - 1;
        if (ki->notifier(ki->upval, &elem)) {
            break;
        }
    }

    return 0;
err:
    return -1;
}

int qtk_kaldiio_save_ark(qtk_kaldiio_t *ki, qtk_kaldiio_elem_t *elem,
                         int binary, qtk_io_writer w, void *upval) {
    qtk_io_writen(upval, w, elem->token.data, elem->token.len);
    qtk_io_write_strliteral(upval, w, " ");
    if (binary) {
        return save_ark_binary(ki, elem, w, upval);
    }
    return save_ark_text(ki, elem, w, upval);
}

int qtk_kaldiio_init(qtk_kaldiio_t *ki,
                     int (*notifier)(void *upval, qtk_kaldiio_elem_t *elem),
                     void *upval) {
    ki->notifier = notifier;
    ki->upval = upval;
    ki->buf = wtk_strbuf_new(1024, 1);
    return 0;
}

int qtk_kaldiio_clean(qtk_kaldiio_t *ki) {
    if (ki->buf) {
        wtk_strbuf_delete(ki->buf);
    }
    return 0;
}
