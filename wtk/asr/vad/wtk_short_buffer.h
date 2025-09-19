#ifndef WTK_VAD_WTK_SHORT_BUFFER_H_
#define WTK_VAD_WTK_SHORT_BUFFER_H_
#include "wtk/core/wtk_type.h"
#include "wtk_vframe.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_short_buffer wtk_short_buffer_t;
#define wtk_short_buffer_unuse_samples(b) (b->end-b->cur)
#define wtk_short_buffer_used_samples(b) (b->cur-b->start)

struct wtk_short_buffer
{
	short *rstart;	//memory start
	short *start;	//current data start.
	short *cur;		//current push position.
	short *end;		//memory end;
	char odd_char;
	unsigned odd:1;
};

wtk_short_buffer_t* wtk_short_buffer_new(int size);
int wtk_short_buffer_delete(wtk_short_buffer_t *b);
void wtk_short_buffer_reset(wtk_short_buffer_t *b);
int wtk_short_buffer_push(wtk_short_buffer_t *b,short *data,int samples);
int wtk_short_buffer_push_c(wtk_short_buffer_t *b,char *data,int bytes);
int wtk_short_buffer_peek(wtk_short_buffer_t *b,wtk_vframe_t *frame);
void wtk_short_buffer_skip(wtk_short_buffer_t *b,int samples,int left_enough);
#ifdef __cplusplus
};
#endif
#endif
