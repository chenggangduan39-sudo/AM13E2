#ifndef WTK_KSR_VAD_KVAD_WTK_KVAD
#define WTK_KSR_VAD_KVAD_WTK_KVAD
#include "wtk/core/wtk_type.h" 
#include "wtk_kvad_cfg.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/asr/vad/wtk_vframe.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kvad wtk_kvad_t;

#ifdef USE_KFRAME
typedef struct
{
	wtk_queue_node_t q_n;
	wtk_queue_node_t hoard_n;
	short *wav;
	float energy;
	unsigned int index;
	unsigned speech:1;
}wtk_kframe_t;
#endif


typedef enum
{
	WTK_KVAD_SIL,
	WTK_KVAD_SPEECH,
}wtk_kvad_state_t;

struct wtk_kvad
{
	wtk_kvad_cfg_t *cfg;
	wtk_kxparm_t *parm;
	wtk_hoard_t frame_hoard;
	wtk_queue_t input_q;
	int nframe;
	int pos;
#ifdef USE_KFRAME
	wtk_kframe_t *cur_frame;
#else
	wtk_vframe_t *cur_frame;
#endif
	wtk_queue_t trap_q;
	wtk_kvad_state_t state;
	wtk_kvad_state_t expand_state;
	wtk_queue_t expand_q;
	wtk_queue_t output_q;
};

int wtk_kvad_bytes(wtk_kvad_t *vad);
wtk_kvad_t* wtk_kvad_new(wtk_kvad_cfg_t *cfg);
void wtk_kvad_delete(wtk_kvad_t *vad);
void wtk_kvad_start(wtk_kvad_t *vad);
void wtk_kvad_reset(wtk_kvad_t *vad);
void wtk_kvad_feed(wtk_kvad_t *vad,short *data,int len,int is_end);
int wtk_kvad_get_left_data(wtk_kvad_t *vad,wtk_string_t *v);
void wtk_kvad_flush_frame(wtk_kvad_t *vad);
#ifdef USE_KFRAME
void wtk_kvad_push_frame(wtk_kvad_t *vad,wtk_kframe_t *f);
#else
void wtk_kvad_push_frame(wtk_kvad_t *vad,wtk_vframe_t *f);
wtk_kvad_t* wtk_kvad_new2(wtk_kvad_cfg_t *cfg,wtk_queue_t **output_queue);
#endif
void wtk_kvad_print_mlf(wtk_kvad_t *vad);
void wtk_kvad_print_mlf2(wtk_kvad_t *vad,FILE *s);
void wtk_kvad_print_mlf3(wtk_kvad_t *vad, FILE *s);

void wtk_kvad_print_prob(wtk_kvad_t *vad);
void wtk_kvad_set_prob(wtk_kvad_t *vad,float para,float para2,float para3);
void wtk_kvad_print_margin(wtk_kvad_t *vad);
void wtk_kvad_set_margin(wtk_kvad_t *vad,int para,int para2);
#ifdef __cplusplus
};
#endif
#endif
