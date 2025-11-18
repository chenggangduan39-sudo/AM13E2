#ifndef __PUBLIC_INC_QVOICE_EGRAM_H__
#define __PUBLIC_INC_QVOICE_EGRAM_H__

#ifdef __cplusplus
extern "C" {
#endif

void *qvoice_egram_new();
void qvoice_egram_delete(void *inst);
int qvoice_egram_feed(void *inst, char *str, int len);

#ifdef __cplusplus
};
#endif
#endif
