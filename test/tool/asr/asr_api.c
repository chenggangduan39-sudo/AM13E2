#include "wtk/asr/api/qvoice_asr_api.h"
#include <stdlib.h>
#include <stdio.h>

extern void* wtk_riff_new(void);
extern int wtk_riff_open(void *f, char *fn);
extern int wtk_riff_read(void *f, char *buf, int size);
extern void wtk_riff_delete(void *f);
extern void* wtk_arg_new(int argc,char** argv);
extern int wtk_arg_delete(void *arg);
extern int wtk_arg_get_str(void *arg,const char *key,int bytes,char** pv);
#define wtk_arg_get_str_s(arg,k,pv) wtk_arg_get_str(arg,(char*)k,sizeof(k)-1,pv)
extern char* file_read_buf(char* fn, int *n);

void test_wav_file(void *asr,char *fn, int wakeup)
{
    void *riff = NULL;
    char buf[1024];
    char out[1024];
    int read_cnt;
    int is_end = 0;
    int len, is_wakeup;

    riff = wtk_riff_new();
    wtk_riff_open(riff, fn);
    printf("%s\n",fn);
    printf("===>3. start asr engine...\n");
    qvoice_asr_api_start(asr);
    while (is_end == 0) {
        read_cnt = wtk_riff_read(riff, buf, sizeof(buf));
        if (read_cnt < sizeof(buf)) {
            is_end = 1;
        }
        printf("===>4. feed data to asr engine...\n");
        qvoice_asr_api_feed(asr, buf, read_cnt,0);
        //get result
        printf("===>5. get result...\n");
        if (wakeup)
        {
        	printf("===>5.1. get wakeup result...\n");
        	is_wakeup = qvoice_asr_api_get_wake(asr);
        	if (is_wakeup)
        	{
        		printf("===>5.2. get cmd word result...\n");
        		len = qvoice_asr_api_get_result(asr,out, 1024);
        		if (len > 0)
        		{
        			printf("asr result: %.*s\n",len, out);
        			printf("===>6. reset engine...\n");
        			qvoice_asr_api_reset(asr);
        		}
        	}
        }
    }
    qvoice_asr_api_feed(asr,0,0,1);
    qvoice_asr_api_reset(asr);
	wtk_riff_delete(riff);
}

int main(int argc,char **argv)
{
	void *arg;
	void* asr;
	char *cfg_fn=0;
	char *vcfg_fn=0;
	char *wav_fn=0;
	char *h_fn=0;
	char *h2_fn=0;
	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"v",&vcfg_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"hw",&h_fn);
	wtk_arg_get_str_s(arg,"hw2",&h2_fn);

	if (cfg_fn==NULL || vcfg_fn==NULL|| wav_fn==NULL || h_fn==NULL)
	{
		printf("Usage:\n"
				"\tasr_api -c cfgfile -v vcfgfile -hw h_fn [-hw2 h2_fn] -wav input.wav\n");
		exit(0);
	}

	printf("===>1. build asr engine...\n");
	asr = qvoice_asr_api_new(cfg_fn, vcfg_fn);
    if(!asr)
	{
    	printf("asr create failed\n");
		goto end;
	}

    char *data;
    int len;
    printf("===>2. set parameter...\n");
    if (h_fn && h2_fn){
		data = file_read_buf(h_fn,&len);
		printf("===>2.1. set parameter for wakeup...\n");
		qvoice_asr_api_setContext_wakeup(asr,data,len);
		if(data)free(data);
		data = file_read_buf(h2_fn,&len);
		printf("===>2.2. set parameter for cmd...\n");
		qvoice_asr_api_setContext_asr(asr,data,len);
		if(data)free(data);
    }else if(h_fn){
		data = file_read_buf(h_fn,&len);
		qvoice_asr_api_setContext(asr,data,len);
		if(data)free(data);
	}

	if(wav_fn)
	{
		test_wav_file(asr,wav_fn, 1);
	}

end:

    printf("===>7. destroy engine...\n");
	if(asr)
	{
		qvoice_asr_api_delete(asr);
	}
	wtk_arg_delete(arg);

	return 0;
}


