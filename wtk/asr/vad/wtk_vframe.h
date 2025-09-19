#ifndef WTK_VAD_WTK_VFRAME_H_
#define WTK_VAD_WTK_VFRAME_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_str.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vframe wtk_vframe_t;
typedef void (*wtk_vframe_raise_f)(void *hook,wtk_vframe_t *f);

typedef enum
{
	wtk_vframe_sil=0,
	wtk_vframe_speech,
	/*
	 *	wtk_vframe_speech_end: the last speech frame of margin,used for wtk_vad2_t,which means the
	 *last right margin frame,which indicate silence end;
	 */
	wtk_vframe_speech_end,
}wtk_vframe_state_t;

struct wtk_vframe
{
	wtk_queue_node_t q_n;				//used for linked in output queue;
	wtk_queue_node_t hoard_n;
	wtk_vframe_state_t state;
	wtk_vframe_state_t raw_state;		//used for vad2, before expand;
	float energy;
	int index;							//[1,..]
	int frame_size;
	int frame_step;
	float speechlike;
	float *sample_data;	//reference to sample data in buffer(size is frame_size);
	short *wav_data;	//validate wave data (size is frame_step);
};

wtk_vframe_t* wtk_vframe_new(int frame_size,int frame_step);
wtk_vframe_t* wtk_vframe_new2(int frame_size,int frame_step);
wtk_vframe_t* wtk_vframe_new3(int frame_size,int frame_step);
int wtk_frame_reset(wtk_vframe_t *f);
int wtk_vframe_delete(wtk_vframe_t *v);
double wtk_vframe_wav_mean(wtk_vframe_t *v);
double wtk_vframe_wav_energy(wtk_vframe_t *v,double mean);
float wtk_vframe_calc_snr(wtk_vframe_t *v);
void wtk_vframe_calc_energy(wtk_vframe_t *v);
void wtk_vframe_calc_energy2(wtk_vframe_t *f);
void wtk_vframe_print(wtk_vframe_t *v);
float wtk_vframe_calc_energy3(wtk_vframe_t *f);
wtk_string_t* wtk_vframe_state_to_string(wtk_vframe_state_t state);
#ifdef __cplusplus
};
#endif
#endif
