#ifndef WTK_FST_NET_KV_WTK_FST_BINET_H_
#define WTK_FST_NET_KV_WTK_FST_BINET_H_
#include "wtk/core/wtk_type.h"
#include "wtk_fst_binet_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fst_binet wtk_fst_binet_t;

typedef struct
{
	int fd;
	void *addr;
}wtk_fst_binet_map_t;

struct wtk_fst_binet
{
	wtk_fst_binet_cfg_t *cfg;
	union
	{
		FILE *file;
		int fd;
		wtk_fst_binet_map_t map;
	}f;
	char *buf;
	int buf_size;
};

wtk_fst_binet_t* wtk_fst_binet_new(wtk_fst_binet_cfg_t *cfg);
void wtk_fst_binet_delete(wtk_fst_binet_t *bin);
void wtk_fst_binet_reset(wtk_fst_binet_t *bin);
int wtk_fst_binet_get(wtk_fst_binet_t* bin,unsigned int idx,wtk_string_t *result);
#ifdef __cplusplus
};
#endif
#endif
