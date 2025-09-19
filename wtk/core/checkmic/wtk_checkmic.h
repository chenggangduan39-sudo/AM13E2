#ifndef WTK_CHECKMIC_WTK_CHECKMIC
#define WTK_CHECKMIC_WTK_CHECKMIC
#include "wtk/core/checkmic/wtk_checkmic_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    int channel[2];
    int st_phase;
    int phase;
}wtk_checkmic_diff_t;



typedef void(*wtk_checkmic_notify_t)(void *ths, wtk_checkmic_diff_t *phase_diff);

typedef struct wtk_checkmic wtk_checkmic_t;

struct wtk_checkmic
{
    wtk_checkmic_cfg_t *cfg;
    wtk_checkmic_diff_t *phase_diff;
    wtk_strbuf_t **mic;
    
    int channel;
    int combination;

    void *ths;
    wtk_checkmic_notify_t notify;
};

wtk_checkmic_t* wtk_checkmic_new(wtk_checkmic_cfg_t *cfg);
void wtk_checkmic_delete(wtk_checkmic_t *checkmic);
void wtk_checkmic_reset(wtk_checkmic_t *checkmic);
void wtk_checkmic_feed(wtk_checkmic_t *checkmic, short **data, int len, int is_end);
void wtk_checkmic_print(wtk_checkmic_t *checkmic);
void wtk_checkmic_set_notify(wtk_checkmic_t *checkmic, void *ths, wtk_checkmic_notify_t notify);

#ifdef __cplusplus
};
#endif
#endif