#include "qvoice_cosynthesis.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>


typedef struct struct_outths{
	void *cs;
	char *outfn;
}struct_outths_t;

//#include "wtk/core/wtk_wavfile.h"
//
//static void notify(char*ths, short *d, int l,short *uit,int len) {
//    wtk_wavfile_t *wav;
//    struct_outths_t* user_ths;
//
//    user_ths = (struct_outths_t*)ths;
//    cosynthesis_info_t* output;
//
//    output = qvoice_cosynthesis_getOutput(user_ths->cs);
//
//    if (output->log > 0)
//    {
//    	printf("===>Result: {'status': %d, 'cost': %f, 'log': \"%s\"}\n", output->state, output->loss, output->log);
//    }
//    else
//    {
//    	printf("===>Result: {'status': %d, 'cost': %f}\n", output->state, output->loss);
//    }
//
//    if (output->state  && l > 0)
//    {
//    	wav = wtk_wavfile_new(16000);
//	    if (wtk_wavfile_open(wav, user_ths->outfn)) {
//	        wtk_debug("Error Open %s\n", user_ths->outfn);
//	        exit(-1);
//	    }
//        int i;
//        printf("unit list:[");
//        for(i=0;i<len;++i)
//        {
//            printf("%d ",uit[i]);
//        }
//        printf("]\n");
//        wtk_wavfile_write(wav, (char *) d, l * 2);
//        wtk_wavfile_close(wav);
//        wtk_wavfile_delete(wav);
//    }
//}

static void notify(void* ths, short *d, int l,short *uit,int len) {
	FILE *fp_pcm=NULL;
	struct_outths_t* user_ths;

    user_ths = (struct_outths_t*)ths;
    cosynthesis_info_t* output;

    output = qvoice_cosynthesis_getOutput(user_ths->cs);

    if (output->log > 0)
    {
    	printf("===>Result: {'status': %d, 'cost': %f, 'log': \"%s\"}\n", output->state, output->loss, output->log);
    }
    else
    {
    	printf("===>Result: {'status': %d, 'cost': %f}\n", output->state, output->loss);
    }

    if (output->state  && l > 0)
    {
        if (user_ths->outfn && NULL==(fp_pcm = fopen(user_ths->outfn, "wb"))) {
            printf("Error Open %s\n", user_ths->outfn);
            return;
        }
        int i;
        printf("unit list:[");
        for(i=0;i<len;++i)
        {
            printf("%d ",uit[i]);
        }
        printf("]\n");
        if (fp_pcm)
        	fwrite(d,sizeof(short),l,fp_pcm);
        fclose(fp_pcm);
    }
}

void _do(void *cs, const char *textfn, const char *outdir, char* sep)
{
	FILE *fp;
	char *p;
	char ref[512];
	char out[512];
	int i;

    //user define struct ths and set value, pl:struct_ths_t
    struct_outths_t user_ths;
    user_ths.cs = cs;
    user_ths.outfn = NULL;

    qvoice_cosynthesis_set_notify(cs, (qvoice_cosynthesis_notify)notify, &user_ths);

	fp = fopen(textfn, "r");
    if (!fp) {
        printf("fail to open refscp: %s\n", textfn);
        return;
    }

    if (access(outdir, 0)!=0)
    {
    	mkdir(outdir, 0777);
    }
    i=0;
    for (;;) {
    	i++;
        if (fgets(ref, sizeof(ref), fp) == NULL) {
            break;
        }
        if (ref[0] == '#') {
            continue;
        }

        if ((p = strrchr(ref, '\n')) != NULL) *p = 0;
        if ((p = strrchr(ref, '\r')) != NULL) *p = 0;

        printf("===>input text: [%d]: %s\n", i, ref);
        //sprintf(out, "%s/%d.pcm",outdir, i);
        sprintf(out, "%s/%d.wav",outdir, i);
        user_ths.outfn=out;
        qvoice_cosynthesis_process(cs, ref, sep);
        qvoice_cosynthesis_reset(cs);
    }

    fclose(fp);
}

static void print_usage(int argc, char **argv)
{
    printf("usage: %s cfg ifn outpath [usebin]\n", argv[0]);
}

int main(int argc, char *argv[]) {
    void *cs;
    char *text, *outdir;
    char *cfn;
    char *sep=".";
    int use_bin=0;

    if (argc < 4) {
        print_usage(argc, argv);
        return -1;
    }
    if (argc == 5)
    	use_bin=atoi(argv[4]);

    cfn = argv[1];
    text = argv[2];
    outdir = argv[3];
    if (use_bin)
    	cs = qvoice_cosynthesis_newbin(cfn);
    else
    	cs = qvoice_cosynthesis_new(cfn);
    if (NULL==cs)
    	printf("engine failed\n");
    else
    	_do(cs, text, outdir, sep);

    qvoice_cosynthesis_delete(cs);

    return 0;
}
