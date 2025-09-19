#ifndef WTK_BFIO_RESAMPLE_WTK_RESAMPLE
#define WTK_BFIO_RESAMPLE_WTK_RESAMPLE
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_resample wtk_resample_t;

typedef void(*wtk_resample_notify_f)(void *ths,char *data,int len);

struct wtk_resample
{
	wtk_strbuf_t *tmp;
	float *input;
	float *output;
	short *soutput;
	int input_size;
	int output_size;
	void *handle;
	double factor;
	void *notify_ths;
	wtk_resample_notify_f notify;

	float max_out;
	unsigned int use_control_bs;
};

//cache like 4096
wtk_resample_t* wtk_resample_new(int cache);
int wtk_resample_start(wtk_resample_t *r,int src_rate,int dst_rate);
int wtk_resample_close(wtk_resample_t *r);
void wtk_resample_set_notify(wtk_resample_t *r,void *ths,wtk_resample_notify_f notify);
int wtk_resample_delete(wtk_resample_t *r);
int wtk_resample_process(wtk_resample_t *r,int is_last,short *data,int len,int *consumed,wtk_strbuf_t *output_buf);
int wtk_resample_feed(wtk_resample_t *f,char *data,int len,int is_end);
void wtk_resample_set_control_bs(wtk_resample_t *r, int use_control_bs);
void wtk_resample_set_max_out(wtk_resample_t *r, float max_out);

wtk_resample_t* wtk_resample_new2(int char_len, int src_rate, int dst_rate);
int wtk_resample_start2(wtk_resample_t *r,int src_rate,int dst_rate);
int wtk_resample_delete2(wtk_resample_t *r);
void wtk_resample_set_notify2(wtk_resample_t *r,void *ths,wtk_resample_notify_f notify);
int wtk_resample_feed2(wtk_resample_t *f,short *data,int len,int is_end);
#ifdef __cplusplus
};
#endif
#endif
