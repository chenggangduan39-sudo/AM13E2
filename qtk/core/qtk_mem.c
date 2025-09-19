#include "qtk/core/qtk_mem.h"

void qtk_mem_init(qtk_mem_t *m, unsigned char *d, size_t cap) {
    m->d = d;
    m->cap = cap;
    m->pos = 0;
}

int qtk_mem_read(qtk_mem_t *m, char *d, int l) {
    if (m->pos > m->cap) {
        return -1;
    }
    int read_n = min(l, m->pos - m->cap);
    memcpy(d, m->d + m->pos, read_n);
    m->pos += read_n;
    return read_n;
}
