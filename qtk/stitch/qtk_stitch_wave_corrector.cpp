#include "qtk_stitch_wave_corrector.h"
#include "qtk_stitch_def.h"
#include "wtk/core/wtk_alloc.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"

qtk_stitch_wave_corrector_t* qtk_stitch_wave_corrector_new(int type)
{
    qtk_stitch_wave_corrector_t* corrector = (qtk_stitch_wave_corrector_t*)wtk_malloc(sizeof(qtk_stitch_wave_corrector_t));
    corrector->type = type;
    wtk_debug("wave corrector %d\n",type);
    switch(corrector->type){
        case QTK_STITCH_WAVE_CORRECT_HORIZ:
            corrector->wave_correct_type = cv::detail::WAVE_CORRECT_HORIZ;
            break;
        case QTK_STITCH_WAVE_CORRECT_VERT:
            corrector->wave_correct_type = cv::detail::WAVE_CORRECT_VERT;
            break;
        case QTK_STITCH_WAVE_CORRECT_AUTO:
            corrector->wave_correct_type = cv::detail::WAVE_CORRECT_AUTO;
            break;
        default:
            corrector->wave_correct_type = -1;
    }
    return corrector;
}

void qtk_stitch_wave_corrector_delete(qtk_stitch_wave_corrector_t* corrector)
{
    wtk_free(corrector);
    return;
}

void qtk_stitch_wave_corrector_correct(qtk_stitch_wave_corrector_t* corrector,void *estimated_cameras)
{
    std::vector<cv::detail::CameraParams> *cameras = (std::vector<cv::detail::CameraParams>*)estimated_cameras;
    std::vector<cv::Mat> rmats;

    for(std::vector<cv::detail::CameraParams>::iterator it = cameras->begin();it!=cameras->end();it++){
        rmats.push_back(it->R);
    }
    
    cv::detail::waveCorrect(rmats,(cv::detail::WaveCorrectKind)corrector->wave_correct_type);
    // std::vector<cv::detail::CameraParams>::iterator it = cameras->begin();
    // for(;it < cameras->end();it++){
    //     cv::Mat R(it->R);
    //     float *d = (float*)R.data;
    //     for(int i = 0; i < it->R.cols; ++i){
    //         for(int j = 0; j < it->R.rows; ++j){
    //             printf("%.5f\n",d[i*it->R.rows+j]);
    //         }
    //     }
    // }
    // exit(1);
    return;
}