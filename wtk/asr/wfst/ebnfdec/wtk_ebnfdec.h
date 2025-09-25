#ifndef WTK_FST_EBNFDEC_WTK_EBNFDEC
#define WTK_FST_EBNFDEC_WTK_EBNFDEC
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk_ebnfdec_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ebnfdec wtk_ebnfdec_t;
#define wtk_ebnfdec_set_word_s(dec,word) wtk_ebnfdec_set_word(dec,word,sizeof(word)-1)

struct wtk_ebnfdec
{
	wtk_ebnfdec_cfg_t *cfg;
	wtk_egram_t *egram;
	wtk_fst_net_t *net;
	wtk_wfstdec_t *dec;
	wtk_vad_t *vad;
	wtk_queue_t vad_q;
};

wtk_ebnfdec_t* wtk_ebnfdec_new(wtk_ebnfdec_cfg_t *cfg);

void wtk_ebnfdec_delete(wtk_ebnfdec_t *dec);

/**
 * @brief set eval word or wakeup words,like:
 *   * "小白"
 *   * "小白|你好"
 */
int wtk_ebnfdec_set_word(wtk_ebnfdec_t *dec,char *s,int bytes);

/**
 * @brief start engine
 */
int wtk_ebnfdec_start(wtk_ebnfdec_t *dec);

/**
 * @breif reset engine
 */
void wtk_ebnfdec_reset(wtk_ebnfdec_t *dec);

/**
 * @brief feed audio;
 */
int wtk_ebnfdec_feed(wtk_ebnfdec_t *dec,char *data,int bytes,int is_end);

/**
 * @brief get reuslt;
 * {
	"rec": "小 盒",   //识别结果
	"conf": 76.70,    //置信度  0-100
	"wavetime": 1020,
	"systime": 16
    }
 */
wtk_string_t wtk_ebnfdec_get_result(wtk_ebnfdec_t *dec);
#ifdef __cplusplus
};
#endif
#endif
