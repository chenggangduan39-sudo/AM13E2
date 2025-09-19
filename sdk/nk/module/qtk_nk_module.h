#ifndef QTK_MISC_NK_MODULE_QTK_NK_MODULE
#define QTK_MISC_NK_MODULE_QTK_NK_MODULE

#include "wtk/os/wtk_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*qtk_nk_module_new_func)(wtk_log_t *log,int size);
typedef void  (*qtk_nk_module_del_func)(void *handler);
typedef int   (*qtk_nk_module_run_func)(void *handler,int looptime,int *recvd);
typedef int   (*qtk_nk_module_add_event_func)(void *handler,int fd,qtk_event_t *event);
typedef int   (*qtk_nk_module_mod_event_func)(void *handler,int fd,qtk_event_t *event);
typedef int   (*qtk_nk_module_del_event_func)(void *handler,int fd,qtk_event_t *event);

typedef struct
{
	qtk_nk_module_new_func new_f;
	qtk_nk_module_del_func del_f;
	qtk_nk_module_run_func run_f;
	qtk_nk_module_add_event_func add_event_f;
	qtk_nk_module_mod_event_func mod_event_f;
	qtk_nk_module_del_event_func del_event_f;
}qtk_nk_module_action_t;

#define qtk_nk_module_mk_action(type) \
{\
	(qtk_nk_module_new_func)   	     CAT(type,_new),\
	(qtk_nk_module_del_func)   		 CAT(type,_del),\
    (qtk_nk_module_run_func)   		 CAT(type,_run),\
	(qtk_nk_module_add_event_func)   CAT(type,_add_event),\
	(qtk_nk_module_mod_event_func)   CAT(type,_mod_event),\
	(qtk_nk_module_del_event_func)   CAT(type,_del_event),\
}

#ifdef __cplusplus
};
#endif
#endif
