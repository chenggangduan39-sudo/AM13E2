#ifndef SDK_AUDIO_RECORDER_EXTRACT_QTK_EXTRACT
#define SDK_AUDIO_RECORDER_EXTRACT_QTK_EXTRACT

#include "wtk/core/wtk_strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_extract qtk_extract_t;

struct qtk_extract
{
	wtk_strbuf_t *buf;
	int buf_size;
	unsigned align:1;
};

qtk_extract_t* qtk_extract_new(int buf_size);
void qtk_extract_delete(qtk_extract_t *ex);

void qtk_extract_start(qtk_extract_t *ex);
void qtk_extract_reset(qtk_extract_t *ex);

int qtk_extract_proc(qtk_extract_t *ex,char *data,int bytes,char *buffer,int len);

#ifdef __cplusplus
};
#endif
#endif
