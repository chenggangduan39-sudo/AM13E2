#ifndef __QTK_BLENDER_FEATHER_H__
#define __QTK_BLENDER_FEATHER_H__

#include "opencv2/core/mat.hpp"

typedef struct qtk_blender_feather qtk_blender_feather_t;

qtk_blender_feather_t *qtk_blender_feather_new(float sharpness, float smooth);
int qtk_blender_feather_delete(qtk_blender_feather_t *feather);
void qtk_blender_feather_prepare(qtk_blender_feather_t *feather, cv::Rect dst_roi);
void qtk_blender_feather_feed(qtk_blender_feather_t *feather, cv::InputArray _img, 
                                cv::InputArray mask, cv::Point tl);
void qtk_blender_feather_feed2(qtk_blender_feather_t *feather, cv::InputArray _img, cv::InputArray mask, 
                    cv::Point tl, cv::InputArray _weight_map, cv::InputArray _yoffset);
void qtk_blender_feather_feed3(qtk_blender_feather_t *feather, cv::InputArray _img, cv::InputArray mask, 
                        cv::Point tl, cv::InputArray _weight_map, cv::InputArray _yoffset, 
                        char alpha,char beta, char gamma,float alpha_x,float beta_x,float gamma_x);
void qtk_blender_feather_feed2_1channal(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                            cv::Point tl, cv::InputArray &_weight_map, cv::InputArray &_yoffset);
void qtk_blender_feather_feed2_1channal_rect(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                            cv::Point tl, cv::InputArray &_weight_map, cv::InputArray &_yoffset, int s, int e);
void qtk_blender_feather_feed2_sp_2channal(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                            cv::Point tl, cv::InputArray &_weight_map, cv::InputArray &_yoffset);
void qtk_blender_feather_feed2_sp_2channal_rect(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &mask, 
                                cv::Point tl, cv::InputArray &_weight_map, cv::InputArray &_yoffset, int s, int e);
void qtk_blender_feather_blend(qtk_blender_feather_t *feather, cv::InputOutputArray dst, 
                                    cv::InputOutputArray dst_mask);
cv::Rect qtk_blender_feather_weightmaps(qtk_blender_feather_t *feather,const std::vector<cv::UMat> &masks, 
                    const std::vector<cv::Point> &corners,std::vector<cv::UMat> &weight_maps);
void qtk_blender_feather_uv_weightmaps(qtk_blender_feather_t *feather, std::vector<cv::UMat> &weight_maps, 
                    std::vector<cv::UMat> &uv_weight_maps);
void qtk_blender_feather_prepare2(qtk_blender_feather_t *feather, cv::Rect dst_roi, void *ptr);
void qtk_blender_feather_nv12_prepare2(qtk_blender_feather_t *feather, cv::Rect dst_roi, void *ptr);
void qtk_blender_feather_yoffsetmaps(qtk_blender_feather_t *feather,const std::vector<cv::UMat> &uxs, 
                    cv::Rect dst_roi,std::vector<cv::Point> &corners,std::vector<cv::Mat> &yoffset_maps, int channel);
void qtk_blender_feather_uv_yoffsetmaps(qtk_blender_feather_t *feather,const std::vector<cv::UMat> &uxs, 
                    cv::Rect dst_roi,std::vector<cv::Point> &corners,std::vector<cv::Mat> &yoffset_maps);
void qtk_blender_feather_weight_maps_update(qtk_blender_feather_t *feather,std::vector<cv::Mat> inupmaps,
                                                std::vector<cv::UMat> &weight_maps);
void qtk_feather_set_smooth(qtk_blender_feather_t *feather,float smooth);
// void qtk_feather_remap_feed(qtk_blender_feather_t *feather, cv::InputArray _img, cv::InputArray mask, 
//                     cv::Point tl, cv::InputArray _weight_map, cv::InputArray _yoffset,cv::Mat &offset_map);
void qtk_blender_feather_feed2_rect(qtk_blender_feather_t *feather, cv::InputArray _img, cv::InputArray mask, 
                    cv::Point tl, cv::InputArray _weight_map, cv::InputArray _yoffset,int s, int e);
void qtk_blender_feather_feed2_table_1channal(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &_weight_map, cv::InputArray &_table);
void qtk_blender_feather_feed2_table_1channal_rect(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &_weight_map, 
                    cv::InputArray &_table, int s, int e);
void qtk_blender_feather_feed2_table_2channal(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &_weight_map, cv::InputArray &_table);
void qtk_blender_feather_feed2_table_2channal_rect(qtk_blender_feather_t *feather, cv::InputArray &_img, cv::InputArray &_weight_map, 
                    cv::InputArray &_table, int s, int e);
#endif