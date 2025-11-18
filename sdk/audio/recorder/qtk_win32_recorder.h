#ifdef WIN32
#ifndef SDK_AUDIO_RECORDER_QTK_WIN32_RECORDER
#define SDK_AUDIO_RECORDER_QTK_WIN32_RECORDER

#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib,"Winmm.lib")

#include "wtk/os/wtk_sem.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_win32_recorder qtk_win32_recorder_t;
struct qtk_win32_recorder
{
	HWAVEIN handler;
	WAVEFORMATEXTENSIBLE fmt;
	WAVEHDR* recorder_buffer;
	int cache;
	int pos;
	wtk_sem_t sem;
};

qtk_win32_recorder_t* qtk_win32_recorder_start(void *ths,
		char *dev,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		);
int qtk_win32_recorder_read(void *h, qtk_win32_recorder_t *r, char *buf, int bytes);
int qtk_win32_recorder_stop(void *h, qtk_win32_recorder_t *r);


#ifdef __cplusplus
};
#endif
#endif

#endif
