#ifndef WTK_FST_LM_WTK_FBIN_H_
#define WTK_FST_LM_WTK_FBIN_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_os.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fbin wtk_fbin_t;

#if (defined __APPLE__) || (defined __WIN32__)
#define __USE_FILE__
#endif

struct wtk_fbin
{
	wtk_strbuf_t *buf;
#ifdef __USE_FILE__
	FILE *fd;
#else
	int fd;
#endif
	char *cache;
	int cache_bytes;
	unsigned int of;
};

wtk_fbin_t* wtk_fbin_new(char *fn);
void wtk_fbin_delete(wtk_fbin_t *f);
int wtk_fbin_get(wtk_fbin_t *f,uint64_t of,uint64_t bytes,wtk_string_t *v);
#ifdef __cplusplus
};
#endif
#endif
