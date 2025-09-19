#ifndef BC81602A_9710_E765_F655_E5FF3F03EEC1
#define BC81602A_9710_E765_F655_E5FF3F03EEC1

typedef struct qtk_fragment qtk_fragment_t;
typedef int (*qtk_fragment_notifier_t)(void *upval, void *frame);

struct qtk_fragment {
    void *upval;
    qtk_fragment_notifier_t notifier;
    void *frame;
    int bpos;
    int fsize;
    int elem_size;
};

qtk_fragment_t *qtk_fragment_new(int fsize, int elem_size, void *upval,
                                 qtk_fragment_notifier_t notifier);
int qtk_fragment_feed(qtk_fragment_t *frag, void *data, int len);
void qtk_fragment_delete(qtk_fragment_t *frag);

#endif /* BC81602A_9710_E765_F655_E5FF3F03EEC1 */
