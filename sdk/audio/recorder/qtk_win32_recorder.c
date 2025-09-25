#ifdef WIN32

#include "qtk_win32_recorder.h" 

static void qtk_win32_recorder_set_fmt(qtk_win32_recorder_t *r, int channel, int sample_rate, int bytes_per_sample)
{
	WAVEFORMATEXTENSIBLE *fmt;

	fmt = &(r->fmt);
	ZeroMemory(fmt, sizeof(WAVEFORMATEXTENSIBLE));
	fmt->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	fmt->Format.nChannels = channel;
	fmt->Format.nSamplesPerSec = sample_rate;
	fmt->Format.nBlockAlign = bytes_per_sample * channel;
	fmt->Format.nAvgBytesPerSec = fmt->Format.nBlockAlign *sample_rate;
	fmt->Format.wBitsPerSample = bytes_per_sample * 8;
	fmt->Format.cbSize = 22;

	fmt->Samples.wValidBitsPerSample = bytes_per_sample * 8;
	fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
};

static void qtk_win32_recorder_set_buffer(qtk_win32_recorder_t *r, int buf_size)
{
	WAVEHDR *hdr;
	int i;


	wtk_debug("buffer size = %d\n", buf_size);
	hdr = (WAVEHDR*)wtk_malloc(sizeof(WAVEHDR)* r->cache);
	ZeroMemory(hdr, sizeof(WAVEHDR)*r->cache);
	for (i = 0; i < r->cache; ++i) {
		hdr[i].lpData = (char*)wtk_malloc(buf_size);
		hdr[i].dwBufferLength = buf_size;
		hdr[i].dwBytesRecorded = 0;
		hdr[i].dwUser = i;
		hdr[i].dwFlags = 0;
		hdr[i].dwLoops = 1;
		hdr[i].lpNext = NULL;
		hdr[i].reserved = 0;
	}
	r->recorder_buffer = hdr;
}

qtk_win32_recorder_t* qtk_win32_recorder_start(void *ths,
		char *dev,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		)
{
	qtk_win32_recorder_t *r;
	MMRESULT result;
	int buf_size;
	int dev_id;
	int ret;
	int i;

	buf_size = channel * sample_rate * buf_time * bytes_per_sample / 1000;
	r = (qtk_win32_recorder_t*)wtk_malloc(sizeof(qtk_win32_recorder_t));

	qtk_win32_recorder_set_fmt(r, channel, sample_rate, bytes_per_sample);

	r->cache = 10;
	qtk_win32_recorder_set_buffer(r, buf_size);

	//if (dev) {
	//	dev_id = ((int*)dev)[0];
	//	result = waveInOpen(&(r->handler), dev_id, &(r->fmt), (DWORD)NULL, 0L, CALLBACK_NULL);
	//}
	//else {
		result = waveInOpen(&(r->handler), WAVE_MAPPER, &(r->fmt), (DWORD)NULL, 0L, CALLBACK_NULL);
	//}
	if (result != MMSYSERR_NOERROR) {
		ret = -1;
		wtk_debug("waveInOpen failed result = %d.\n",result);
		goto end;
	}

	for (i = 0; i < r->cache; ++i) {
		result = waveInPrepareHeader(r->handler, &(r->recorder_buffer[i]), sizeof(WAVEHDR));
		if (result != MMSYSERR_NOERROR) {
			ret = -1;
			wtk_debug("waveInPrepareHeader failed and i = %d.\n", i);
			goto end;
		}

		result = waveInAddBuffer(r->handler, &(r->recorder_buffer[i]), sizeof(WAVEHDR));
		if (result != MMSYSERR_NOERROR) {
			wtk_debug("waveInAddBuffer failed.\n");
			ret = -1;
			goto end;
		}
	}

	result = waveInStart(r->handler);
	if (result != MMSYSERR_NOERROR) {
		ret = -1;
		wtk_debug("waveInStart failed.\n");
		goto end;
	}

	r->pos = 0;
	wtk_sem_init(&(r->sem), 0);
	ret = 0;
end:
	if (ret != 0) {
		qtk_win32_recorder_stop(NULL, r);
		r = NULL;
	}
	return r;
}

int qtk_win32_recorder_read(void *h, qtk_win32_recorder_t *r, char *buf, int bytes)
{
	MMRESULT result;
	int ret;

	wtk_debug("bytes = %d\n",bytes);

	while (1) {
		if (r->recorder_buffer[r->pos].dwFlags & WHDR_DONE) {
			ret = r->recorder_buffer[r->pos].dwBytesRecorded;
			wtk_debug("readed = %d.\n", ret);
			memcpy(buf, r->recorder_buffer[r->pos].lpData, ret);
			result = waveInAddBuffer(r->handler, &(r->recorder_buffer[r->pos]), sizeof(WAVEHDR));
			if (result != MMSYSERR_NOERROR) {
				ret = -1;
				wtk_debug("read waveInAddBuffer failed.\n");
				goto end;
			}
			r->pos = (r->pos + 1) % r->cache;
			break;
		}
		wtk_sem_acquire(&(r->sem), 1);
	}
end:
	return ret;
}

int qtk_win32_recorder_stop(void *h, qtk_win32_recorder_t *r)
{
	MMRESULT result;
	int i, ret;

	result = waveInReset(r->handler);
	if (result != MMSYSERR_NOERROR) {
		ret = -1;
		wtk_debug("waveInReset failed.\n");
		goto end;
	}

	for (i = 0; i < r->cache; ++i) {
		result = waveInUnprepareHeader(r->handler, &(r->recorder_buffer[i]), sizeof(WAVEHDR));
		if (result != MMSYSERR_NOERROR) {
			ret = -1;
			wtk_debug("waveInUnprepareHeader failed. i = %d result = %d\n",i,result);
			goto end;
		}
		wtk_free(r->recorder_buffer[i].lpData);
	}

	result = waveInClose(r->handler);
	if (result != MMSYSERR_NOERROR) {
		ret = -1;
		wtk_debug("waveInClose failed.\n");
		goto end;
	}

	wtk_free(r->recorder_buffer);
	wtk_free(r);
	ret = 0;
end:
	return ret;
}

#endif
