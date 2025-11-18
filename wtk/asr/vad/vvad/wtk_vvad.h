#ifndef WTK_VAD_VVAD_WTK_VVAD
#define WTK_VAD_VVAD_WTK_VVAD
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RATE 16000
#define WINS 512
#define FSIZE (WINS/2)
#define NBIN (WINS/2+1)
#define THRESH 500
#define END_FRAMES 20
#define MIN_FREQ 250
#define MAX_FREQ 800

typedef struct wtk_vvad wtk_vvad_t;
struct wtk_vvad
{
	wtk_drft_t *rfft;
	wtk_complex_t *fft;
	float *amp;
	float *window;
	float *rfft_in;
	float *mem;

	float thresh;
	int end_frames;
	int end_cnt;
	int min_idx;
	int max_idx;
};

wtk_vvad_t* wtk_vvad_new();
void wtk_vvad_delete(wtk_vvad_t *v);
void wtk_vvad_reset(wtk_vvad_t *v);
int wtk_vvad_feed(wtk_vvad_t *v,short *data);
void wtk_vvad_set_conf(wtk_vvad_t *v,float thresh,int end_frames);

#ifdef __cplusplus
};
#endif
#endif
