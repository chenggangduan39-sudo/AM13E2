#include <istream>
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <stdint.h>
#include <string>
#include "qtk/cv/qtk_cv_portrait_segmentation.h"
#include "Parse.h"

using namespace cv;
using namespace std;

extern "C"{


static char* jstring2string(JNIEnv* env, jstring jstr)
{
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr= (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
 
    if (alen > 0)
    {
        rtn = (char*)malloc(alen + 1);
 
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

static Mat cap; 

JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_image_Parse_readVec
  (JNIEnv *env, jclass thiz, jstring path)
{
    char *jpath=jstring2string(env,path);
    FILE *f=fopen(jpath,"r");
    uint8_t *data=new uint8_t[860*1031*3];
    if(fread(data,860*1031*3,sizeof(uint8_t),f));
    fclose(f);
    jbyteArray ret_data=env->NewByteArray(860*1031*3);
    env->SetByteArrayRegion(ret_data, 0, 860*1031*3, (jbyte*)data);

    free(jpath);
    delete []data;
    return ret_data;
}

JNIEXPORT void JNICALL Java_com_qdreamer_image_Parse_showVec
  (JNIEnv *env, jclass thiz, jlong pa)
{
    qtk_cv_portrait_segmentation_t *p=(qtk_cv_portrait_segmentation_t*)pa;
    Mat dst=Mat(p->img_desc.height,p->img_desc.width,CV_8UC3,p->data);
    //imshow("vec",dst);
    imwrite("/home/cgq/test.jpg",dst);
    //waitKey(5);
}

JNIEXPORT jlong JNICALL Java_com_qdreamer_image_Parse_newParse
  (JNIEnv *env, jclass thiz, jstring model, jint width, jint height)
{
    qtk_cv_portrait_segmentation_t *pa=qtk_cv_portrait_segmentation_new();
    pa->img_desc.width=(int)width;
    pa->img_desc.height=(int)height;
    char *pa_model=jstring2string(env,model);
    pa->invo=qtk_cv_detection_onnxruntime_new(pa_model);
    free(pa_model);
    return (jlong)pa;
}

JNIEXPORT void JNICALL Java_com_qdreamer_image_Parse_deleteParse
  (JNIEnv *env, jclass thiz, jlong jpa)
{
    qtk_cv_portrait_segmentation_t *pa=(qtk_cv_portrait_segmentation_t*)jpa;
    qtk_cv_detection_onnxruntime_delete(pa->invo);
    qtk_cv_portrait_segmentation_delete(pa);
    cap.release();
}

void parse_data_process(const Mat &src, uint8_t *data, int width, int height,int channel){ 
    if (src.isContinuous()) {
        memcpy(data, src.ptr(), src.total() * src.elemSize());
    } else {
        for (int i = 0; i < height; i++)
            memcpy(data + i * width * channel, src.ptr() + i * src.step,
                   src.elemSize() * src.cols);
    }
}

JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_image_Parse_processParse
  (JNIEnv *env, jclass thiz, jlong jpa, jbyteArray bk_data, jbyteArray jdata)
{
    qtk_cv_portrait_segmentation_t *pa=(qtk_cv_portrait_segmentation_t*)jpa;


    //auto beforeTime = std::chrono::steady_clock::now();
    jbyte *sz =(jbyte*)(*env).GetByteArrayElements(jdata, nullptr);
    uint8_t *data=(uint8_t*)sz;
    if (data==nullptr){
        return nullptr;
    }
    
    jbyte *bk=(jbyte*)(*env).GetByteArrayElements(bk_data, nullptr);
    uint8_t *bk_y2k=(uint8_t*)bk;
    if (bk_y2k==nullptr){
        return nullptr;
    }


    
    qtk_cv_portrait_segmentation_process(pa,data,qtk_cv_detection_onnxruntime_invoke_single,bk_y2k);
    env->ReleaseByteArrayElements(jdata, sz, JNI_ABORT);
    env->ReleaseByteArrayElements(bk_data, bk, JNI_ABORT);
    jbyteArray ret_data=env->NewByteArray(pa->img_desc.width*pa->img_desc.height*3);
    env->SetByteArrayRegion(ret_data, 0, pa->img_desc.width*pa->img_desc.height*3, (jbyte*)pa->data);
    
    /*auto afterTime = std::chrono::steady_clock::now();
    double duration_millsecond = std::chrono::duration<double, std::milli>(afterTime - beforeTime).count();
	std::cout << duration_millsecond << "毫秒" << std::endl;*/
    return ret_data;
}

}