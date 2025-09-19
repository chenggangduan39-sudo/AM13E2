#include "acoustic/devicetts/qtk_devicetts.h"

extern void* wtk_wavfile_new(int sample_rate);
extern int wtk_wavfile_open(void *f,char *fn);
extern int wtk_wavfile_write(void *f,const char *data,int bytes);
extern int wtk_wavfile_close(void *f);
extern int wtk_wavfile_delete(void *f);
extern double time_get_ms();

#define wtk_arg_get_str_s(arg,k,pv) wtk_arg_get_str(arg,(char*)k,sizeof(k)-1,pv)
#define wtk_arg_get_int_s(arg,k,n) wtk_arg_get_int(arg,k,sizeof(k)-1,n)
#define wtk_arg_get_float_s(arg,k,n) wtk_arg_get_float(arg,k,sizeof(k)-1,n)

void print_arg(void)
{
    printf("the function is :\n");
    printf("\t\t -c cfg file fn\n");
    printf("\t\t -scp scp file fn\n");
    printf("\t\t -o out dir/file\n");
    return;
}

double st, t, wav_t, dt=0, twav_t=0;
static void* wav;

static int test_notify(void* ths, short* data, int len, int is_end)
{
	printf("=========data: %d=======is_end: %d\n", len, is_end);
	if (is_end)
	{
		if (len==0)
		{
			printf("end\n");
			return 0;
		}
		t=(time_get_ms()-st)/1000.0;
		wav_t= len /16000.0;
		dt=t-twav_t;
		if (twav_t > 0 && dt < 0)
			dt=0;
		//...end for a time wave
		printf("run-time=%f(s) wav-dur= %f(s) delay-time= %f(s) rate= %f\n",t,wav_t, dt, t/wav_t);
	}else
	{
		t=(time_get_ms()-st)/1000.0;
		wav_t=len /16000.0;
		if (twav_t < t)
			printf("run-time=%f(s) wav-dur= %f(s) delay-time= %f(s) rate= %f\n", t, wav_t, t-twav_t, t/wav_t);
		else
			printf("run-time=%f(s) wav-dur= %f(s) delay-time= 0.00(s) rate= %f\n", t, wav_t, t/wav_t);
		twav_t += wav_t;
	}
	st = time_get_ms();
	if (wav && len > 0)
		wtk_wavfile_write(wav, (const char*)data, len * 2);

	return 0;
}

int id1[] = {48,  5, 27, 15, 38,  7,  5, 11, 20, 42, 38,  6,  5, 26, 15, 38,  6,  5,
         28, 15, 20,  9,  3, 30, 15,  9,  5, 19, 12,  8,  5, 36, 31, 38,  6,  5,
         17, 15,  7,  5, 36, 15,  9,  5, 47, 15,  9,  3, 16, 21,  8,  5, 41, 42,
         38,  6,  4, 32, 13, 23,  8,  5, 34, 20, 46,  7,  5, 47, 15,  9,  3, 30,
         15,  7,  5, 37, 31, 38,  9,  5, 30, 15,  9,  5, 40, 15, 31, 38,  6,  2,
         36, 31, 38,  6,  5, 19, 12,  7,  5, 47, 15,  9,  4, 11, 20,  9,  5, 28,
         15,  7,  2, 30, 15,  8,  5, 33, 46,  7,  5, 18, 42, 23,  7,  4, 26, 15,
         43,  6,  5, 34, 43,  9,  2, 30, 15,  8,  5, 33, 46,  7,  5, 18, 42, 23,
          7,  4, 40, 15, 23,  8,  5, 18, 46,  7,  5, 17, 24,  6,  4, 30, 24,  8,
          5, 17, 42, 38,  7,  5, 16, 29, 10,  2, 37, 42, 38,  9,  5, 22, 15,  7,
          3, 40, 15, 23,  8,  5, 11, 15,  7,  5, 17, 15,  7,  2, 39, 24,  6,  5,
         22, 13, 23,  6,  4, 48,  5};

int main(int argc,char **argv)
{
    char *ofn,*cfn;
    wtk_arg_t *arg = NULL;
    // int is_bin;
    char *scp;
    char *text;
    qtk_devicetts_cfg_t *cfg = NULL;
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_devicetts_t *dev = NULL;
    wtk_veci_t *token_id = NULL;

    arg = wtk_arg_new(argc,argv);
    if(arg == NULL) goto end;
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_str_s(arg,"scp",&scp);
    wtk_arg_get_str_s(arg,"s",&text);

    if(!cfn ||!ofn){
        print_arg();
        goto end;
    }
    main_cfg = wtk_main_cfg_new_type(qtk_devicetts_cfg,cfn);
    cfg = main_cfg->cfg;
    token_id = wtk_veci_new(sizeof(id1)/sizeof(int));
    memcpy(token_id->p,id1,sizeof(id1));
    struct timeval start,end;
    
    dev = qtk_devicetts_new(cfg);
//    qtk_devicetts_set_notify(dev, test_notify, NULL);
    gettimeofday(&start,NULL);
    qtk_devicetts_process_id(dev,token_id, 1);
    gettimeofday(&end,NULL);
    printf("use time %lf\n", (1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec) /1000000.0);
end:
    if(dev) qtk_devicetts_delete(dev);
    if(token_id) wtk_veci_delete(token_id);
    if(main_cfg) wtk_main_cfg_delete(main_cfg);
    if(arg) wtk_arg_delete(arg);
    
    return 0;
}
