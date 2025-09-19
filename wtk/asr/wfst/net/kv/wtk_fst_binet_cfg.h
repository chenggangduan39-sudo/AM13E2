#ifndef WTK_FST_NET_KV_WTK_FST_BINET_CFG_H_
#define WTK_FST_NET_KV_WTK_FST_BINET_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fst_binet_cfg wtk_fst_binet_cfg_t;

/**
 *	FILE format:
 *	4: "X==Y"
 *	8: idx size;
 */

typedef enum
{
	WTK_FST_BINET_MEM,
	WTK_FST_BINET_FILE,
	WTK_FST_BINET_FD,
	WTK_FST_BINET_MAP,
}wtk_fst_binet_type_t;

struct wtk_fst_binet_cfg
{
	wtk_rbin2_t *rbin;
	char *bin_fn;
	uint64_t *offset;		//offset of idx;	memory=[0,ndx] and use with[0,ndx)
	//unsigned int bytes;	//bytes of file;
	unsigned int ndx;	//number of index; states+1
	uint64_t idx_offset;
	uint64_t data_offset;
	uint64_t filesize;
	int cache_size;

	char *data;
	uint64_t data_bytes;

	//-------------- update section -------------
	wtk_fst_binet_type_t type;
	//-------------------------------------------
	unsigned use_idx:1;

	unsigned use_memory:1;
	unsigned use_file:1;
	unsigned use_fd:1;
	unsigned use_map:1;
	unsigned use_pack_bin:1;
	unsigned use_vmem:1;
	unsigned use_rbin:1;
};

int wtk_fst_binet_cfg_init(wtk_fst_binet_cfg_t *cfg);
int wtk_fst_binet_cfg_clean(wtk_fst_binet_cfg_t *cfg);
int wtk_fst_binet_cfg_update_local(wtk_fst_binet_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_fst_binet_cfg_update(wtk_fst_binet_cfg_t *cfg);
int wtk_fst_binet_cfg_update2(wtk_fst_binet_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_fst_binet_cfg_bytes(wtk_fst_binet_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
