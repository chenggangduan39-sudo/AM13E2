extern "C" {
#include "qtk/cv/api/qtk_cv_frec_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

extern void* wtk_riff_new(void);
extern int wtk_riff_open(void *f, char *fn);
void wtk_riff_delete(void *f);
extern void* wtk_arg_new(int argc,char** argv);
extern int wtk_arg_delete(void *arg);
extern int wtk_arg_get_str(void *arg,const char *key,int bytes,char** pv);
extern int wtk_arg_get_float(void *arg,const char *key,int bytes,float* number);
#define wtk_arg_get_str_s(arg,k,pv) wtk_arg_get_str(arg,(char*)k,sizeof(k)-1,pv)
#define wtk_arg_get_float_s(arg,k,n) wtk_arg_get_float(arg,k,sizeof(k)-1,n)
extern char* file_read_buf(char* fn, int *n);
extern uint8_t *qtk_image_load(void *desc, const char *filename);
}

static void Usage(int argc, char *argv[])
{
	printf("Usage:");
	printf("\timgfrec_api -c cfg_fn -i image_path -u username -t thresh\n");
}

typedef enum {
    QBL_IMAGE_MJPEG,
    QBL_IMAGE_NV12,
    QBL_IMAGE_RGB24,
    QBL_IMAGE_BGR24,
} qtk_image_fmt_t;
typedef struct qtk_image_desc qtk_image_desc_t;
struct qtk_image_desc {
    qtk_image_fmt_t fmt;
    int height;
    int width;
    int channel;
};

static void register_(void *frec, uint8_t* img, char* username) {
	qtk_cv_frec_api_box_t *bbox;
    int nresult;

    //input username

    nresult = qtk_cv_frec_api_register(frec, img, username, strlen(username));
    if (nresult != 1) {
        printf("register face > 1\n");
        exit(-1);
    }

    //register rec
    printf("===>3.1 face rec result...\n");
    nresult=qtk_cv_frec_api_rstValue(frec, &bbox, 1);
    printf("trec x1=%f y1=%f x2=%f y2=%f name=[%s]\n", bbox[0].x1, bbox[0].y1, bbox[0].x2, bbox[0].y2, bbox[0].name);
}


static void rec_(void *frec, uint8_t* img) {
    int nresult, i;
    qtk_cv_frec_api_box_t *box;
    nresult = qtk_cv_frec_api_rec(frec, img);

    printf("===>4.1. face detect result...\n");
    nresult = qtk_cv_frec_api_rstValue(frec, &box, 0);
    for (i = 0; i < nresult; i++) {
    	printf("rec[%d] x1=%f y1=%f x2=%f y2=%f\n", i, box[i].x1, box[i].y1, box[i].x2, box[i].y2);
    }

    printf("===>4.2. face rec result...\n");
    nresult = qtk_cv_frec_api_rstValue(frec, &box, 1);
    for (i = 0; i < nresult; i++) {
    	printf("trec x1=%f y1=%f x2=%f y2=%f name=[%s]\n", box[i].x1, box[i].y1, box[i].x2, box[i].y2, box[i].name);
    }
}

int main(int argc, char *argv[]) {
    void *arg;
    char *cfn;
    char *dir_path=NULL;
    char *username=NULL;
    float thresh = 1;

    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfn);
    wtk_arg_get_float_s(arg, "t", &thresh);
    wtk_arg_get_str_s(arg,"i",&dir_path);
    wtk_arg_get_str_s(arg,"u",&username);

    if (cfn==NULL || username==NULL||dir_path==NULL)
    {
    	Usage(argc, argv);
    	exit(0);
    }

    void *frec;

    printf("===>1. build face rec engine...\n");
    frec = qtk_cv_frec_api_new(cfn);
    printf("===>2. set parameter[selected]...\n");
    qtk_cv_frec_api_setThresh(frec, thresh);

    const char* ImgFiles[10]={
    		"0.jpg",
			"1.jpg",
			"2.jpg",
			"3.jpg",
			"4.jpg",
			"5.jpg",
			"6.jpg",
			"7.jpg",
			"8.jpg",
			"9.jpg",
    };
    char name[1000];
    int i;
    char key;

    for (i=0; i < 10; i++){
        memset(name,'\0',1000);
        strcpy(name,dir_path);
        strcpy(name+strlen(dir_path),"/");
        strcpy(name+strlen(dir_path)+1,ImgFiles[i]);
        printf("======name:%s\n", name);
        qtk_image_desc_t desc;
        uint8_t *imagedata=qtk_image_load(&desc,name);
        printf("=======imagedata:%p\n",imagedata);
        printf("=======height:%d width:%d channel:%d\n", desc.height, desc.width, desc.channel);
        scanf("%c",&key);
        switch (key) {
        case 'r':
        	printf("===>3. register face:[%s]...\n", username);
            register_(frec, imagedata, username);
            printf("register finish\n");
            free(imagedata);
            imagedata=NULL;
            break;
        case 'p':
        	printf("===>4. face rec...\n");
            rec_(frec, imagedata);
            printf("rec finish\n");
            free(imagedata);
            imagedata=NULL;
            break;
        case 'q':
            goto end;
        }
        scanf("\n"); //discard \n
    }

end:
    printf("===>5. destroy engine...\n");
	qtk_cv_frec_api_delete(frec);
    wtk_arg_delete(arg);
}
