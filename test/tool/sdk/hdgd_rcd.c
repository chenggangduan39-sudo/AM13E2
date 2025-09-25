#include "sdk/qtk_api.h"
#include <pthread.h>
#include <stdio.h>

typedef struct strbuf_tt
{
    char *data;					//raw data;
    int pos;					//valid data size;
    int length;					//memory size of raw data;
    float rate;					//memory increase rate;
}strbuf_t;

extern void* qtk_record_cfg_new();
extern void qtk_record_cfg_delete(void *cfg);
extern void *qtk_record_new(void *cfg);
extern void qtk_record_delete(void *rcd);
extern void *qtk_record_read(void *rcd);

int run;
void* rcd = NULL;
void* wav;
int cnt=0;
void* record_thread_entry(void *rcd)
{
	strbuf_t *buf;

	while(run == 1){
		cnt++;
		if (cnt > 100) break;
		buf = qtk_record_read(rcd);
		if(buf->pos == 0){
			printf("buf->pos = %d\n", buf->pos);
			continue;
		}
		wtk_wavfile_write(wav, buf->data, buf->pos);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	void* rcd_cfg;
	pthread_t rcd_t;
	char*ofn;
	int ret=0;

	//rcd
	rcd_cfg=qtk_record_cfg_new();
	rcd = qtk_record_new(rcd_cfg);
	if(!rcd){
		printf("rcd new failed.\n");
		goto end;
	}

    ofn="./tmp.wav";

    wav = wtk_wavfile_new(16000);
    wtk_wavfile_set_channel(wav, 10);
    if (wtk_wavfile_open(wav,ofn)) {
        printf("Error Open %s\n", ofn);
    }

	//rcd
	run = 1;
	ret=pthread_create(&rcd_t, NULL, record_thread_entry, rcd);
	if (ret!=0) goto end;


	getchar();
	run = 0;
	pthread_join(rcd_t, NULL);

end:
	if (wav)
		wtk_wavfile_delete(wav);
	if(rcd){
		qtk_record_delete(rcd);	
	}
	if (rcd_cfg)
		qtk_record_cfg_delete(rcd_cfg);
}
