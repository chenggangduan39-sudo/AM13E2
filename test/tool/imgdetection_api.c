#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "qtk/cv/api/qtk_cv_detection_api.h"

extern double time_get_ms(void);
extern uint8_t *qtk_image_load(void *desc, const char *filename);
extern void* wtk_arg_new(int argc,char** argv);
extern int wtk_arg_delete(void *arg);
extern int wtk_arg_get_str(void *arg,const char *key,int bytes,char** pv);
extern int wtk_arg_get_int(void *arg,const char *key,int bytes,int* number);
#define wtk_arg_get_str_s(arg,k,pv) wtk_arg_get_str(arg,(char*)k,sizeof(k)-1,pv)
#define wtk_arg_get_int_s(arg,k,n) wtk_arg_get_int(arg,k,sizeof(k)-1,n)

static void Usage(int argc, char *argv[])
{
	printf("Usage:");
	printf("\tcvdetection_api -c cfg_fn -i test_image_path -t type");
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

double st;
int main(int argc ,char *argv[]){

    void *arg=NULL;
    char *cfn=NULL;
    char *dir_path=NULL;
    int type=-1;
    qtk_cv_detection_api_box_t *box;

    //tools parameter parser
    arg=wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_str_s(arg,"i",&dir_path);
    wtk_arg_get_int_s(arg,"t",&type);
    if (!cfn && !dir_path){
        wtk_arg_delete(arg);
        Usage(argc, argv);
        return 0;
    }

    void *detect;

    printf("===>1. build detect engine[type=%d]...\n", type);
    detect = qtk_cv_detection_api_new(cfn, (qtk_cv_detection_api_type_t)type);

    printf("===>2. set param...\n");
    switch(type){
    case QTK_CV_DETECTION_HEAD:
    case QTK_CV_DETECTION_FACE:
    	qtk_cv_detection_api_setProb(detect, 0.45, 0.3);
    	qtk_cv_detection_api_setAttrs(detect, 960, 540, 0);
    	break;
    }


    //input and output prepare
    uint8_t *data1 = (uint8_t *)malloc(960 * 544 * 3);
    const int diff = 2 * 960 * 3;
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
        memset(data1, 127, 960 * 544 * 3);
        memcpy(data1 + diff, imagedata, 960 * 540 * 3);

        //3. feed data

        st=time_get_ms();
        qtk_cv_detection_api_process(detect, data1);
	printf("=====spent time: %f\n",time_get_ms()-st);
        //4. obtain result data, next to do (point or other action)
        double w_dio = 1; //(double)width / (double)960;
        double h_dio = 1; //(double)height / (double)540;
        for (int i = 0; i < qtk_cv_detection_api_rstCount(detect); i++){
        	box=qtk_cv_detection_api_rstValue(detect);
        	printf("======[%d](%d, %d) (%d, %d)[%d]\n", i,
        			(int)(box[i].x1 * w_dio),
                    (int)(box[i].y1 * h_dio),
					(int)(box[i].x2 * w_dio),
					(int)(box[i].y2 * h_dio),
					box[i].no);
        }

        free(imagedata);
        imagedata=NULL;
    }

    //5. destroy engine
    printf("===>end and destroy engine...\n");
    qtk_cv_detection_api_delete(detect);
    free(data1);
    wtk_arg_delete(arg);
    return 0;
}
