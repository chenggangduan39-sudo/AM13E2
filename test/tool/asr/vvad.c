#include "wtk/asr/vad/vvad/wtk_vvad.h"
#include <stdio.h>
#include <sys/time.h>

static double time_get_ms()
{
    struct timeval tv;
    double ret;
    int err;

    err=gettimeofday(&tv,0);
    if(err==0)
    {
        ret=tv.tv_sec*1000.0+tv.tv_usec*1.0/1000;
        //ret maybe is NAN
        if(ret!=ret)
        {
            printf("NAN(%.0f,sec=%.d,usec=%.d).\n",ret,(int)tv.tv_sec,(int)tv.tv_usec);
            ret=0;
        }
    }else
    {
        perror(__FUNCTION__);
        ret=0;
    }
    return ret;
}

static void test_vvad_file(wtk_vvad_t *vvad,char *ifn)
{
    FILE *iwav;
    short *pv;
    int len,bytes;
    int ret;
    char tmp[44];
    int cnt;
	int vad=0;

    len=FSIZE;
    bytes=sizeof(short)*len;
    pv=(short *)malloc(FSIZE*sizeof(short));

    iwav=fopen(ifn,"rb");
    fread(tmp, 1, 44, iwav);
    double t=time_get_ms();
    cnt=0;
    while(1)
    {
        ret=fread((char *)pv, 1, bytes, iwav);
        if(ret<=0)
        {
            // printf("break ret=%d\n",ret);
            break;
        }
        len=ret/sizeof(short);
        if(len>=FSIZE){
            vad = wtk_vvad_feed(vvad,pv);
			printf("%d\n", vad);
        }
        cnt+=len;
    }
    t=time_get_ms()-t;
    // printf("rate=%f t=%f\n",t/(cnt/16.0),t);

    fclose(iwav);
    free(pv);
}


int main(int argc,char **argv)
{
    wtk_vvad_t *vvad;
    vvad=wtk_vvad_new();
    wtk_vvad_reset(vvad);
    test_vvad_file(vvad,argv[1]);
    wtk_vvad_delete(vvad);

    return 0;
}