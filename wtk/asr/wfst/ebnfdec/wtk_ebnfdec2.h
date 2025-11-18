#ifndef WTK_FST_EBNFDEC_WTK_EBNFDEC2
#define WTK_FST_EBNFDEC_WTK_EBNFDEC2
#include "wtk/core/wtk_type.h" 
#include "wtk_ebnfdec2_cfg.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ebnfdec2 wtk_ebnfdec2_t;
struct wtk_ebnfdec2
{
	wtk_ebnfdec2_cfg_t *cfg;
	struct wtk_wfstdec *dec;
};

wtk_ebnfdec2_t* wtk_ebnfdec2_new(wtk_ebnfdec2_cfg_t *cfg);
void wtk_ebnfdec2_delete(wtk_ebnfdec2_t *dec);

int wtk_ebnfdec2_start(wtk_ebnfdec2_t *dec);
void wtk_ebnfdec2_reset(wtk_ebnfdec2_t *dec);
int wtk_ebnfdec2_feed(wtk_ebnfdec2_t *dec,char *data,int bytes,int is_end);
wtk_string_t wtk_ebnfdec2_get_result(wtk_ebnfdec2_t *dec);

#ifdef __cplusplus
};
#endif
#endif
