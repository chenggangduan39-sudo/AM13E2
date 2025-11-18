#include "qtk_stitch_feature_matcher.h"
#include "wtk/core/wtk_alloc.h"
#include "opencv2/stitching/detail/matchers.hpp"
#include <vector>

static float _get_default_match_conf(int type);
static void* _feature_matcher(int matcher_type, int range_width, float match_conf,int try_use_gpu);

qtk_stitch_feature_matcher_t* qtk_stitch_feature_matcher_new(int matcher_type, 
                                                            int range_width,
                                                            float match_conf)
{
    qtk_stitch_feature_matcher_t* matcher = (qtk_stitch_feature_matcher_t*)wtk_malloc(sizeof(qtk_stitch_feature_matcher_t));
    memset(matcher,0,sizeof(qtk_stitch_feature_matcher_t));
    matcher->matcher_type = matcher_type;
    matcher->range_width = range_width;
    matcher->match_conf = match_conf;
    matcher->try_use_gpu = 0;
    matcher->match = _feature_matcher(matcher_type, range_width, match_conf, matcher->try_use_gpu);
    matcher->pairwise_matches = new std::vector<cv::detail::MatchesInfo>;

    return matcher;
}

static void* _feature_matcher(int matcher_type, int range_width, float match_conf,int try_use_gpu)
{
    wtk_debug("match type %d %d\n",matcher_type,range_width);
    if(matcher_type == QTK_STITCH_FEATURE_MATCHER_AFFINE){
        return (void*)new cv::detail::AffineBestOf2NearestMatcher(try_use_gpu,match_conf);
    }else if(range_width == -1){
        return (void*)new cv::detail::BestOf2NearestMatcher(try_use_gpu,match_conf);
    }else{
        return (void*)new cv::detail::BestOf2NearestRangeMatcher(range_width,try_use_gpu,match_conf);
    }
    return NULL;
}

void qtk_stitch_feature_matcher_delete(qtk_stitch_feature_matcher_t* matcher)
{
    if(matcher->match){
        delete (std::vector<cv::detail::MatchesInfo>*)matcher->pairwise_matches;
        delete (cv::detail::FeaturesMatcher*)matcher->match;
        matcher->match = NULL;
    }
    wtk_free(matcher);
    return;
}

//得到特征匹配步骤的置信度
float qtk_stitch_feature_get_match_conf(float match_conf, int type)
{
    if(match_conf < 0.001){
        return _get_default_match_conf(type);
    }
    return match_conf;
}

static float _get_default_match_conf(int type)
{
    if(type == QTK_STITCH_FEATURE_DETECTOR_ORB){
        return 0.3;
    }
    return 0.65;
}

void qtk_stitch_feature_matcher_match_feature(qtk_stitch_feature_matcher_t* matcher,wtk_queue_t *queue)
{
    cv::detail::FeaturesMatcher *fmatch = (cv::detail::FeaturesMatcher*)matcher->match;
    //设置feature list
    std::vector<cv::detail::ImageFeatures> features;
    std::vector<cv::detail::MatchesInfo> *pairwise_matches = (std::vector<cv::detail::MatchesInfo>*)matcher->pairwise_matches;
    int i = 0;
    int n = queue->length;
    qtk_stitch_queue_data_t *data;
    wtk_queue_node_t *qn;

    for(i = 0;i < n;i++){
        qn = wtk_queue_peek(queue,i);
        data = data_offset2(qn,qtk_stitch_queue_data_t,node);
        cv::detail::ImageFeatures *feature = (cv::detail::ImageFeatures*)data->feature_data;
        features.push_back(*feature);
    }
    
    pairwise_matches->clear();
    (*fmatch)(features,*pairwise_matches);
    // std::vector<cv::detail::MatchesInfo>::iterator it = pairwise_matches->begin();
    // for(;it < pairwise_matches->end();it++){
    //     printf("confidence %f %d\n",it->confidence, it->num_inliers);
    // }
    // exit(1);
    fmatch->collectGarbage();
    return;
}