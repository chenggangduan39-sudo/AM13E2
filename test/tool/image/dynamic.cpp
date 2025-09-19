#include <bits/stdint-uintn.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>
#include <dirent.h>

#include "qtk/cv/detection/qtk_cv_detection_dynamic.h"
using namespace std;
using namespace cv;

void data_process(const Mat &src,uint8_t *data,int width,int height,int channel){
    if (src.isContinuous()){
        memcpy(data,src.ptr(),src.total()*src.elemSize());
    }
    else {
        for (int i=0;i<height;i++)
            memcpy(data+i*width*channel,src.ptr()+i*src.step,src.elemSize()*src.cols);
    }
}

void resize_process(uint8_t *src,qtk_image_desc_t *src_desc,uint8_t *dst,qtk_image_desc_t *dst_desc){
    src_desc->fmt=QBL_IMAGE_RGB24;
    qtk_image_resize(src_desc,src,dst_desc->height,dst_desc->width,dst);
}

void GetFiles(std::string path, std::vector<std::string>& files)
{
    DIR *dir;
    struct dirent *ptr;
    std::string p;
 
    if((dir= opendir(path.c_str()) )==NULL)
    {
        std::cout << "cannot open:" << path << std::endl;
        return ;
    }
 
    while ((ptr = readdir(dir)) != NULL)
    {
        if (ptr->d_type == 4)
        {
            if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0)
            {
                GetFiles(p.assign(path).append("/").append(ptr->d_name), files);
            }
 
        }
 
        else
        {
            std::string fname(ptr->d_name);
            std::string imgType = fname.substr(fname.rfind("."), fname.length());
            if (imgType == ".jpg" || imgType == ".jpeg" || imgType == ".JPG" || imgType == ".JPEG" || imgType == ".bmp" || imgType == ".BMP" || imgType == ".png" || imgType == ".PNG")
            // if (imgType == ".txt" )
            {
                files.push_back(p.assign(path).append("/").append(ptr->d_name));  //文件名和路径一起包装好
                //files.push_back(p.assign(ptr->d_name));  //只放入文件名
            }
        }
    }
    closedir(dir);
}
 
bool GreaterEqSort(std::string filePath1, std::string filePath2)
{
    int len1 = filePath1.length();
    int len2 = filePath2.length();
    if(len1 < len2)
    {
        return false;
    }
    else if(len1 > len2)
    {
        return true;
    }
    else
    {
        int iter = 0;
        while(iter < len1)
        {
            if(filePath1.at(iter) < filePath2.at(iter))
            {
                return false;
            }
            else if(filePath1.at(iter) > filePath2.at(iter))
            {
                return true;
            }
            ++iter;
        }
    }
    return true;
}
 
bool LessSort(std::string filePath1, std::string filePath2)
{
    return (!GreaterEqSort(filePath1, filePath2));
}
 
 
void pathSort(std::vector<std::string> &paths, int sortMode)
{
 
    if(sortMode == 1)
    {
        std::sort(paths.begin(), paths.end(), LessSort);
    }
}

static void print(){
    cout<<"error! please enter:"<<endl;
    cout<<"                     -d  dymamic model"<<endl;
    cout<<"                     -p  image path(dir)"<<endl;
}
int main(int argc ,char *argv[]){

    wtk_arg_t *arg=nullptr;
    char *dynamic_model=nullptr;
    char *dir_path=nullptr;
    arg=wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"d",&dynamic_model);
    wtk_arg_get_str_s(arg,"p",&dir_path);
    if (!dynamic_model && !dir_path){
        wtk_arg_delete(arg);
        print();
        return 0;
    }

    std::vector<std::string> ImgFiles;
    std::string imgPath=dir_path;
    GetFiles(imgPath,ImgFiles);
    pathSort(ImgFiles,1);
    qtk_cv_detection_dynamic_t *dy=qtk_cv_detection_dynamic_new();
    dy->score=20.0;
    dy->invo=qtk_cv_detection_onnxruntime_new(dynamic_model);

    for (auto &it:ImgFiles){
        cout<<it<<endl;
        char name[1000];
        memset(name,'\0',1000);
        strcpy(name,it.c_str());
        qtk_image_desc_t desc;
        uint8_t *imagedata=qtk_image_load(&desc,name);
        qtk_cv_detection_dynamic_process_data(dy,imagedata,&desc,resize_process,qtk_cv_detection_onnxruntime_invoke_multiple);
        qtk_cv_detection_dynamic_show(dy->num);
        free(imagedata);
        imagedata=NULL;
    }

    qtk_cv_detection_onnxruntime_delete(dy->invo);
    qtk_cv_detection_dynamic_delete(dy);
    wtk_arg_delete(arg);
}