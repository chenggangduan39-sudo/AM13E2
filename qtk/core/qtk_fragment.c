#include "qtk/core/qtk_fragment.h"
#include "wtk/core/wtk_alloc.h"

qtk_fragment_t *qtk_fragment_new(int fsize, int elem_size, void *upval,
                                 qtk_fragment_notifier_t notifier) {
    qtk_fragment_t *frag =
        wtk_malloc(sizeof(qtk_fragment_t) + elem_size * fsize);
    frag->elem_size = elem_size;
    frag->fsize = fsize;
    frag->upval = upval;
    frag->notifier = notifier;
    frag->frame = (void *)(frag + 1);
    frag->bpos = 0;
    return frag;
}

int qtk_fragment_feed(qtk_fragment_t *frag, void *data, int len) {
    char *ptr = (char *)data;
    int blen = len * frag->elem_size;
    int flen = frag->fsize * frag->elem_size;
    while (blen > 0) {
        int need_len = flen - frag->bpos;
        if (blen >= need_len) {
            if (frag->bpos == 0) {
                frag->notifier(frag->upval, ptr);
            } else {
                memcpy((char *)frag->frame + frag->bpos, ptr, need_len);
                frag->notifier(frag->upval, frag->frame);
                frag->bpos = 0;
            }
            ptr += need_len;
            blen -= need_len;
        } else {
            memcpy((char *)frag->frame + frag->bpos, ptr, blen);
            frag->bpos += blen;
            break;
        }
    }
    return 0;
}

void qtk_fragment_delete(qtk_fragment_t *frag) { wtk_free(frag); }
