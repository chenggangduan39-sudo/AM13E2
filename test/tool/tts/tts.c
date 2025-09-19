/*
 * vits.c
 *
 *  Created on: Aug 23, 2022
 *      Author: dm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qtk/tts/module/qtk_tts_module_api.h"

extern void* wtk_wavfile_new(int sample_rate);
extern int wtk_wavfile_open(void *f,char *fn);
extern int wtk_wavfile_write(void *f,const char *data,int bytes);
extern int wtk_wavfile_close(void *f);
extern int wtk_wavfile_delete(void *f);
extern void* wtk_flist_it_new(char *fn);
extern char* wtk_flist_it_next(void *it);
extern void wtk_flist_it_delete(void *it);
extern void* wtk_arg_new(int argc,char** argv);
extern int wtk_arg_get_str(void *arg,const char *key,int bytes,char** pv);
extern int wtk_arg_get_int(void *arg,const char *key,int bytes,int* number);
extern int wtk_arg_get_float(void *arg,const char *key,int bytes,float* number);
extern int wtk_arg_delete(void *arg);
extern int wtk_arg_exist(void *arg,const char* key,int bytes);
extern double time_get_ms();

#define wtk_arg_get_str_s(arg,k,pv) wtk_arg_get_str(arg,(char*)k,sizeof(k)-1,pv)
#define wtk_arg_get_int_s(arg,k,n) wtk_arg_get_int(arg,k,sizeof(k)-1,n)
#define wtk_arg_get_float_s(arg,k,n) wtk_arg_get_float(arg,k,sizeof(k)-1,n)

static int usage_print(int argc, char **argv)
{
	printf("Usage:\n"
			" tts -c cfg use_bin [0|1] -s text -l text_list\n"
			"    -c cfg configure file\n"
			"    -b use_bin res format, default 0\n"
			"    -s text, optional\n"
			"    -l text list file, optional, don't permit null line(include space, tab) \n"
			"    -o  output dir, placing tts-wave \n"
			"    control switches:\n"
			"    -v n Change the sound volume by n percents (n = -80.0 .. +200.0 %%)\n"
			"    -t n Change the sound tempo(speed) by n percents (n = -50.0 .. +100.0 %%)\n"
			"    -p n, Change the sound pitch by n semitones (n = -5.0 .. + 5.0 semitones)\n"
			"    -r n, Change the sound playback rate by n percents (n = -50.0 .. +100.0 %%) that affects both tempo and pitch.\n"
			"    pl:\n"
			"    ./tts -b 1 -c res/bin/tts.bin -l data/text.len.2\n"
			"    ./tts -b 1 -c res/bin/tts.bin -s \"今天天气怎么样\"\n"
			"    ./tts -b 1 -c res/bin/tts.bin -s \"今天天气怎么样\" -v 50 \n\n");
	return 0;
}

static double st, t, wav_t, dt=0, twav_t=0;
static int count;
static void* wav;
static int wav_save=0;
static int test_notify(void* ths, short* data, int len, int is_end)
{
	if (is_end)
	{
		if (len==0)
		{
			printf("=====tts end=====\n");
			return 0;
		}
		t=(time_get_ms()-st)/1000.0;
		wav_t= len /16000.0;
		dt=t-twav_t;
		if (twav_t > 0 && dt < 0)
			dt=0;
		//...end for a time wave
		printf("run-time=%f(s) wav-dur= %f(s) delay-time= %f(s) rate= %f\n",t,wav_t, dt, t/wav_t);
		printf("=====tts end=====\n");
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
	if (wav_save && wav && len > 0)
		wtk_wavfile_write(wav, (const char*)data, len * 2);

	return 0;
}

static void tts_onetext(void* api, char* istr, char* odir)
{
    char *no_sep;
    char ofn[512];
    int ret;

	no_sep = strchr(istr, '|');
	if (no_sep == NULL)
	{
		sprintf(ofn, "%s/%d.wav", odir, count);
	}else
	{
		sprintf(ofn, "%s/%.*s.wav", odir, (int)(no_sep-istr), istr);
		istr = no_sep + 1;
	}

	if (wav_save){
		wav = wtk_wavfile_new(16000);
	    if (wtk_wavfile_open(wav,ofn)) {
	        printf("Error Open %s\n", ofn);
	    }
	}
    st=time_get_ms();
	ret=qtk_tts_module_api_feed(api, istr, strlen(istr));
	if (ret!=0)
	{
		exit(0);
	}
	if (wav_save)
	{
	    wtk_wavfile_close(wav);
	    wtk_wavfile_delete(wav);
	    printf("synthesis wav see %s\n", ofn);
	}
    qtk_tts_module_api_reset(api);
//    exit(0);
}

int main(int argc, char **argv)
{
    int ret = 0;
    void *args = NULL;
    void *api  = NULL;
    char *cfn = NULL;
    char *istr = NULL;
    char *scp = NULL;
    char *odir=NULL;
    int use_bin=0;
    void *it = NULL;
    float vol, tempo, pitch, rate;

    args = wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(args,"c",&cfn);
    wtk_arg_get_int_s(args,"b",&use_bin);
    wtk_arg_get_str_s(args,"s",&istr);
    wtk_arg_get_str_s(args,"l",&scp);
    wtk_arg_get_str_s(args,"o",&odir);
    wtk_arg_get_float_s(args,"v",&vol);
    wtk_arg_get_float_s(args,"t",&tempo);
    wtk_arg_get_float_s(args,"p",&pitch);
    wtk_arg_get_float_s(args,"r",&rate);

    if (NULL == cfn || (NULL==istr && NULL==scp))
    {
    	usage_print(argc, argv);
    	exit(0);
    }

    if (odir)
    {
    	wav_save = 1;
    }

    api = qtk_tts_module_api_new(cfn, use_bin);
    if (api==NULL)
    {
    	printf("tts module create failed\n");
    	exit(0);
    }
    if (wtk_arg_exist(args, "v", 1))
    	qtk_tts_module_api_setVolChanged(api, vol);
    if (wtk_arg_exist(args, "t", 1))
    	qtk_tts_module_api_setTempoChanged(api, tempo);
    if (wtk_arg_exist(args, "p", 1))
    	qtk_tts_module_api_setPitchChanged(api, pitch);
    if (wtk_arg_exist(args, "r", 1))
    	qtk_tts_module_api_setRateChanged(api, rate);

    qtk_tts_module_api_setNotify(api, test_notify, NULL);

    count=0;
    if(scp == NULL){
    	count++;
    	tts_onetext(api, istr, odir);
    }else{
        it = wtk_flist_it_new(scp);
        while(1){
            istr = wtk_flist_it_next(it);
            if(!istr || *istr=='#')  break;
            count++;
            tts_onetext(api, istr, odir);
        }
        wtk_flist_it_delete(it);
    }

    qtk_tts_module_api_delete(api);

    wtk_arg_delete(args);
    return ret;
}
