#ifndef __WTK_XVPRINT_FILE2_H__
#define __WTK_XVPRINT_FILE2_H__
#include "wtk/core/wtk_queue.h"
#include "wtk_xvprint_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wtk_xvprint_file2_name_notify)(void *hook,wtk_string_t *name);

int wtk_xvprint_file2_dump(char *fn,wtk_queue_t *feat_q);
int wtk_xvprint_file2_dump_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node);
int wtk_xvprint_file2_query(char *fn,wtk_string_t *name);
int wtk_xvprint_file2_delete_feat(char *fn,wtk_string_t *name);
int wtk_xvprint_file2_delete_all(char *fn);
int wtk_xvprint_file2_delete(char *fn);
int wtk_xvprint_file2_append_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node);
int wtk_xvprint_file2_insert_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node);
int wtk_xvprint_file2_load(char *fn,wtk_queue_t *q);
wtk_xvprint_cfg_feat_node_t* wtk_xvprint_file2_load_feat(char *fn,wtk_string_t *name);
int wtk_xvprint_file2_update_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node);
void wtk_xvprint_file2_query_all(char *fn,void *hook,wtk_xvprint_file2_name_notify notify);
#ifdef __cplusplus
};
#endif
#endif
