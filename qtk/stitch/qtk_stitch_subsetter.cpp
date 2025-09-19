#include "qtk_stitch_subsetter.h"
#include "wtk/core/wtk_str.h"
#include "wtk/core/wtk_alloc.h"
#include "opencv2/stitching/detail/matchers.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "qtk_stitch_def.h"
#include <vector>

qtk_stitch_subsetter_t* qtk_stitch_subsetter_new(float confidece_threshold, char *save_file)
{
    qtk_stitch_subsetter_t *subset = (qtk_stitch_subsetter_t*)wtk_malloc(sizeof(qtk_stitch_subsetter_t));
    subset->confidece_threshold = confidece_threshold;
    subset->save_file = wtk_str_dup(save_file);
    subset->indices = NULL;
    return subset; 
}

void qtk_stitch_subsetter_subset(qtk_stitch_subsetter_t *subset,wtk_queue_t *queue,void *paiwise_watches)
{
    //设置feature list
    std::vector<cv::detail::ImageFeatures> features;
    std::vector<cv::detail::MatchesInfo> *pairwise_matches = (std::vector<cv::detail::MatchesInfo>*)paiwise_watches;
    int n = queue->length;
    int i = 0;
    wtk_queue_node_t *qn;
    qtk_stitch_queue_data_t *data;
    std::vector<int> ll;
    std::vector<int> *llp;

    for(i = 0;i < n;i++){
        qn = wtk_queue_peek(queue,i);
        data = data_offset2(qn,qtk_stitch_queue_data_t,node);
        cv::detail::ImageFeatures *feature = (cv::detail::ImageFeatures*)data->feature_data;
        features.push_back(*feature);
    }

    ll = cv::detail::leaveBiggestComponent(features,*pairwise_matches,subset->confidece_threshold);
    if(subset->indices){
        llp = (std::vector<int>*)subset->indices;
        delete llp;
        subset->indices = NULL;
    }
    subset->indices = (std::vector<int>*)new std::vector<int>(ll);
    return;
}

void qtk_stitch_subsetter_delete(qtk_stitch_subsetter_t *subset)
{
    std::vector<int> *llp;
    if(subset){
        if(subset->indices){
            llp = (std::vector<int>*)subset->indices;
            delete llp;
            subset->indices = NULL;
        }
        if(subset->save_file){
            wtk_free(subset->save_file);
        }
        wtk_free(subset);
    }
    return;
}