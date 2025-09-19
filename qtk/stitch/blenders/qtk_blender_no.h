#ifndef __QTK_BLENDER_NO_H__
#define __QTK_BLENDER_NO_H__

#include "opencv2/core/mat.hpp"

typedef struct qtk_blender_no qtk_blender_no_t;

qtk_blender_no_t* qtk_blender_no_new(void);
int qtk_blender_no_delete(qtk_blender_no_t *blender);
void qtk_blender_no_prepare(qtk_blender_no_t *blender,cv::Rect &dst_roi);
void qtk_blender_no_prepare2(qtk_blender_no_t *blender, cv::Rect dst_roi, void *ptr);
void qtk_blender_no_nv12_prepare2(qtk_blender_no_t *feather, cv::Rect dst_roi, void *ptr);
void qtk_blender_no_feed(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, cv::Point &tl);
void qtk_blender_no_feed2(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, cv::Point &tl);
void qtk_blender_no_feed2_1channel(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, 
                            cv::Point &tl,cv::InputArray &_yoffset);
void qtk_blender_no_feed2_sp_2channel(qtk_blender_no_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                            cv::Point tl,cv::InputArray &_yoffset);
void qtk_blender_no_feed2_rect(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, cv::Point &tl,
                            int s, int e);
void qtk_blender_no_feed2_1channel_rect(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_mask, 
                            cv::Point &tl,cv::InputArray &_yoffset,int s,int e);
void qtk_blender_no_feed2_sp_2channel_rect(qtk_blender_no_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                            cv::Point tl,cv::InputArray &_yoffset,int s, int e);
void qtk_blender_no_blend(qtk_blender_no_t *blender,cv::InputOutputArray dst, cv::InputOutputArray dst_mask);

void qtk_blender_no_yoffsetmaps(qtk_blender_no_t *blender_no,const std::vector<cv::UMat> &uxs, 
                    cv::Rect dst_roi,std::vector<cv::Point> &corners,std::vector<cv::Mat> &yoffset_maps, int channel);
void qtk_blender_no_uv_yoffsetmaps(qtk_blender_no_t *blender_no,const std::vector<cv::UMat> &uxs, 
                    cv::Rect dst_roi,std::vector<cv::Point> &corners,std::vector<cv::Mat> &yoffset_maps);
//曝光一起用
void qtk_blender_no_weightmaps(qtk_blender_no_t *blender,const std::vector<cv::UMat> &masks,
                        const std::vector<cv::Point> &corners,std::vector<cv::UMat> &weight_maps);
void qtk_blender_no_weight_maps_update(qtk_blender_no_t *blender,std::vector<cv::Mat> inupmaps,
                        std::vector<cv::UMat> &weight_maps);
void qtk_blender_no_feed2_exposure(qtk_blender_no_t *blender, cv::InputArray &_img, cv::InputArray &_weight_map, cv::Point &tl,
                        cv::InputArray &_yoffset);
void qtk_blender_no_feed2_exposure_rect(qtk_blender_no_t *blender, cv::InputArray _img, cv::Point tl,
                        cv::InputArray _weight_map, cv::InputArray _yoffset,int s, int e);
void qtk_blender_no_feed2_1channel_exposure_rect(qtk_blender_no_t *feather, cv::InputArray &_img, cv::Point tl,
                        cv::InputArray &_weight_map, cv::InputArray &_yoffset, int s, int e);
#endif