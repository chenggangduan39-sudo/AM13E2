#ifndef __WTK_XVPRINT_FILE_H__
#define __WTK_XVPRINT_FILE_H__
#include "wtk/core/wtk_queue.h"
#include "wtk_xvprint_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif


int wtk_xvprint_file_dump(char *fn,wtk_queue_t *feat_q);
int wtk_xvprint_file_dump_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node);
int wtk_xvprint_file_insert_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node);
int wtk_xvprint_file_delete_feat(char *fn,wtk_string_t *name);
int wtk_xvprint_file_load(char *fn,wtk_queue_t *q);
int wtk_xvprint_file_query(char *fn,wtk_string_t *name);
wtk_xvprint_cfg_feat_node_t* wtk_xvprint_file_load_feat(char *fn,wtk_string_t *name);
int wtk_xvprint_file_update_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node);
int wtk_xvprint_file_append_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node);
int wtk_xvprint_file_append_feat2(char *fn,wtk_queue_t  *q);

///////////////////////////////////////////////////////////////////////////////
void wtk_xvprint_file_print_head(char *fn);
#ifdef __cplusplus
};
#endif
#endif
