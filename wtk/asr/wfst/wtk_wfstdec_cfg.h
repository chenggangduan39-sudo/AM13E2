#ifndef WTK_FST_FST_DECODER_CFG_H_
#define WTK_FST_FST_DECODER_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/asr/model/wtk_hmmset_cfg.h"
#include "wtk/asr/wfst/rec/wtk_wfstr.h"
#include "wtk/asr/wfst/egram/wtk_egram.h"
#include "wtk/asr/wfst/rec/rescore/wtk_rescore.h"
#include "wtk/asr/wfst/wtk_wfstdec_output.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/asr/wfst/usrec/wtk_usrec.h"
#include "wtk/asr/wfst/ebnfdec/wtk_ebnfdec2.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnf_rec.h"
#ifdef USE_DNNC
#include "wtk/dnnc/wtk_dnnc.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfstdec_cfg wtk_wfstdec_cfg_t;


struct wtk_wfstdec_cfg
{
	wtk_ebnfdec2_cfg_t ebnfdec2;
	wtk_rescore_cfg_t rescore;
	wtk_vad_cfg_t vad;
	wtk_fst_net_cfg_t net;
	wtk_usrec_cfg_t usrec;
	wtk_fst_net_cfg_t usrnet;
	wtk_fextra_cfg_t extra;
#ifdef USE_DNNC
	wtk_dnnc_cfg_t dnnc;
#endif
	wtk_hmmset_cfg_t hmmset;
	wtk_wfstrec_cfg_t rec;
	wtk_wfstdec_output_cfg_t output;
	wtk_xbnf_rec_cfg_t xbnf;
	int	label_slot_hint;
	char *usr_bin;
	wtk_fst_net_t *usr_net;
	wtk_label_t *label;
	int pad_sil_start_time;
	int pad_sil_end_time;
	int pad_sil_vad_end_time;
	wtk_string_t *sil_start_data;
	wtk_string_t *sil_end_data;
	wtk_string_t *sil_vad_end_data;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	unsigned int use_vad:1;
	unsigned int use_rescore:1;
	unsigned int flush_parm:1;
	unsigned int use_mt:1;
	unsigned int debug:1;
	unsigned int use_ebnf:1;
	unsigned int use_usrec:1;
	unsigned int usrec_is_active:1;
	unsigned int use_ebnfdec2:1;
	unsigned int use_dnnc:1;
	unsigned int use_xbnf:1;
};

int wtk_wfstdec_cfg_init(wtk_wfstdec_cfg_t *cfg);
int wtk_wfstdec_cfg_clean(wtk_wfstdec_cfg_t *cfg);
int wtk_wfstdec_cfg_update_local(wtk_wfstdec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wfstdec_cfg_update(wtk_wfstdec_cfg_t *cfg);
int wtk_wfstdec_cfg_update2(wtk_wfstdec_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_wfstdec_cfg_bytes(wtk_wfstdec_cfg_t *cfg);
int wtk_wfstdec_cfg_set_ebnf_net(wtk_wfstdec_cfg_t *cfg,char *fn);

wtk_wfstdec_cfg_t* wtk_wfstdec_cfg_new_bin(char *bin_fn);
wtk_wfstdec_cfg_t* wtk_wfstdec_cfg_new_bin2(char *bin_fn,unsigned int seek_pos);
void wtk_wfstdec_cfg_delete_bin(wtk_wfstdec_cfg_t *cfg);

wtk_main_cfg_t* wtk_wfstdec_cfg_new(char *cfg_fn);
void wtk_wfstdec_cfg_delete(wtk_main_cfg_t *main_cfg);
#ifdef __cplusplus
};
#endif
#endif
