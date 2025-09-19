#ifndef QTK__DECODER_WRAPPER_CFG_H_
#define QTK__DECODER_WRAPPER_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/asr/wfst/kaldifst/qtk_kwfstdec_cfg.h"
#include "wtk/asr/fextra/wtk_fextra_cfg.h"
#include "wtk/core/cfg/wtk_version_cfg.h"
#include "wtk/asr/wfst/ebnfdec/wtk_ebnfdec2.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnf_rec.h"
#include "wtk/asr/wfst/egram/wtk_egram.h"
#include "wtk/core/strlike/wtk_chnlike_cfg.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/asr/xvprint/wtk_xvprint_cfg.h"
#include "wtk/lex/wtk_lex.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnf_rec.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_decoder_wrapper_cfg qtk_decoder_wrapper_cfg_t;


struct qtk_decoder_wrapper_cfg
{
	qtk_kwfstdec_cfg_t kwfstdec;
	qtk_kwfstdec_cfg_t kwfstdec2;
	wtk_xvprint_cfg_t xvprint;
	wtk_lex_cfg_t lex;
    wtk_vad_cfg_t vad;
	wtk_ebnfdec2_cfg_t ebnfdec2;
	wtk_fextra_cfg_t extra;
	wtk_kxparm_cfg_t parm;
	wtk_chnlike_cfg_t chnlike;	
	wtk_xbnf_rec_cfg_t xbnf;
	wtk_version_cfg_t version;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	char *lex_fn;
	int flush_parm;
	int asr_route;
	int f_cnt;
	wtk_string_t res;
	unsigned use_mt;
	unsigned use_ebnfdec2;
    unsigned int use_vad:1;
    unsigned int use_kxparm:1;
	unsigned int use_xvprint:1;
	unsigned int use_lex:1;
	unsigned int use_lite:1;
	unsigned int use_xbnf:1;
	//wtk_vector_t **vec;
};

int qtk_decoder_wrapper_cfg_init(qtk_decoder_wrapper_cfg_t *cfg);
int qtk_decoder_wrapper_cfg_clean(qtk_decoder_wrapper_cfg_t *cfg);
int qtk_decoder_wrapper_cfg_update_local(qtk_decoder_wrapper_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_decoder_wrapper_cfg_update(qtk_decoder_wrapper_cfg_t *cfg);
int qtk_decoder_wrapper_cfg_update2(qtk_decoder_wrapper_cfg_t *cfg,wtk_source_loader_t *sl);
int qtk_decoder_wrapper_cfg_bytes(qtk_decoder_wrapper_cfg_t *cfg);

wtk_main_cfg_t* qtk_decoder_wrapper_cfg_new(char *cfg_fn);
void qtk_decoder_wrapper_cfg_delete(wtk_main_cfg_t *main_cfg);
qtk_decoder_wrapper_cfg_t* qtk_decoder_wrapper_cfg_new_bin(char *bin_fn);
qtk_decoder_wrapper_cfg_t* qtk_decoder_wrapper_cfg_new_bin2(char *bin_fn,unsigned int seek_pos);
void qtk_decoder_wrapper_cfg_delete_bin(qtk_decoder_wrapper_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
