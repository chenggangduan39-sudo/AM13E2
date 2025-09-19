#ifndef F9DDA489_4E64_98B1_3A7D_FD8675CC5A1E
#define F9DDA489_4E64_98B1_3A7D_FD8675CC5A1E

#include "qtk/core/qtk_type.h"

typedef struct qtk_mem qtk_mem_t;

struct qtk_mem {
    unsigned char *d;
    size_t cap;
    size_t pos;
};

void qtk_mem_init(qtk_mem_t *m, unsigned char *d, size_t cap);
int qtk_mem_read(qtk_mem_t *m, char *d, int l);

#endif /* F9DDA489_4E64_98B1_3A7D_FD8675CC5A1E */
