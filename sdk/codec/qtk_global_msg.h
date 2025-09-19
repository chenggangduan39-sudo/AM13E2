#ifndef __QTK_GLOBAL_MSG_H__
#define __QTK_GLOBAL_MSG_H__

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_global_msg qtk_global_msg_t;
typedef struct qtk_global_msg_node qtk_global_msg_node_t;

qtk_global_msg_t *qtk_global_msg_get_handle();
void qtk_global_msg_free();
int qtk_global_msg_push(qtk_global_msg_t *m, char *data, int len, int theta);
char *qtk_global_msg_pop(qtk_global_msg_t *m, int *len, int *theta);

#ifdef __cplusplus
};
#endif
#endif
