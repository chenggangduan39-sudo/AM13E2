/*
 * soundstretch.c
 *
 *  Created on: Mar 30, 2023
 *      Author: dm
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../qtk/soundtouch/qtk_soundtouch_api.h"

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

// Processing chunk size (size chosen to be divisible by 2, 4, 6, 8, 10, 12, 14, 16 channels ...)
#define BUFF_SIZE           6720

static int usage_print(int argc, char **argv)
{
	printf("Usage:\n"
			"    pl: soundstretch -i infile -o outfile [switch]\n\n"
			"    Parameter and Option:\n"
			"    -i infile, Name of the input sound data file (in .WAV audio file format).\n"
			"    -o outfile, Name of the output sound file (in .WAV audio file format)\n\n"
			"    control switches:\n"
			"    -t n Change the sound tempo by n percents (n = -50.0 .. +100.0 %%)\n"
			"    -p n, Change the sound pitch by n semitones (n = -12.0 .. + 12.0 semitones)\n"
			"    -r n, Change the sound playback rate by n percents (n = -50.0 .. +100.0 %%) that affects both tempo and pitch.\n\n\n");
	return 0;
}

static void test_soundtouch_api(int argc, char* argv[])
{
    void *args = NULL;
    FILE *inFile;
    void *outFile;
    float tempo, pitch, rate;
    char *ifn, *ofn;
    void* api=NULL;
    int nsample;
    short buf[SOUNDTOUCH_BUFF_SIZE];

    // Parse command line parameters
    args = wtk_arg_new(argc,argv);
    wtk_arg_get_float_s(args,"t",&tempo);
    wtk_arg_get_float_s(args,"p",&pitch);
    wtk_arg_get_float_s(args,"r",&rate);
    wtk_arg_get_str_s(args,"i",&ifn);
    wtk_arg_get_str_s(args,"o",&ofn);

    if (NULL == ifn || NULL == ofn)
    {
    	usage_print(argc, argv);
    	return;
    }

    api = qtk_soundtouch_api_new(NULL);

    if (wtk_arg_exist(args, "t", 1))
    	qtk_soundtouch_api_setTempoChange(api, tempo);
    if (wtk_arg_exist(args, "p", 1))
    	qtk_soundtouch_api_setPitchSemiTones(api, pitch);
    if (wtk_arg_exist(args, "r", 1))
    	qtk_soundtouch_api_setRateChange(api, rate);
    // Open input & output files
    outFile = wtk_wavfile_new(16000);
    if (wtk_wavfile_open(outFile,ofn)) {
        printf("Error Open %s\n", ofn);
        goto end;
    }
    if(NULL != (inFile = fopen(ifn, "r"))){
    	do{
    		nsample=fread(buf, sizeof(short), SOUNDTOUCH_BUFF_SIZE, inFile);
    		if (nsample<=0)
    			break;
    		qtk_soundtouch_api_feed(api, buf, nsample);
            do{
            	nsample=qtk_soundtouch_api_recv(api, buf, SOUNDTOUCH_BUFF_SIZE);
        		if (nsample <= 0)break;
        		wtk_wavfile_write(outFile, (char*)buf, nsample * 2);
            } while (1);

    	}while(1);

        qtk_soundtouch_api_flush(api);
        do
        {
        	nsample=qtk_soundtouch_api_recv(api, buf, BUFF_SIZE);
    		if (nsample <= 0)break;
    		wtk_wavfile_write(outFile, (char*)buf, nsample * 2);
        } while (nsample != 0);

    	wtk_wavfile_close(outFile);
    }

end:
	if (outFile)
		wtk_wavfile_delete(outFile);
	if (inFile)
		fclose(inFile);
	if (api)
		qtk_soundtouch_api_delete(api);
	if (args)
		wtk_arg_delete(args);
}

int main(const int argc, char* argv[])
{
//	test_soundtouch(argc, argv);
	test_soundtouch_api(argc, argv);

    return 0;
}




