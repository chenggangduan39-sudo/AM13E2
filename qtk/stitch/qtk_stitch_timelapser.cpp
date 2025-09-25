#include "qtk_stitch_timelapser.h"
#include "opencv2/stitching/detail/timelapsers.hpp"
#include "qtk_stitch_def.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_str.h"

qtk_stitch_timelapser_t* qtk_stitch_timelapser_new(int type,char *prefix)
{
    qtk_stitch_timelapser_t *timelapser = NULL;
    
    timelapser = (qtk_stitch_timelapser_t*)wtk_malloc(sizeof(*timelapser));

    memset(timelapser,0,sizeof(*timelapser));
    if(prefix){
        timelapser->timelapse_prefix = wtk_str_dup(prefix);
    }
    timelapser->type = type;
    timelapser->do_timelapse = 1;
    wtk_debug("timelapser type %d %s\n",type,prefix);
    if(type == QTK_STITCH_TIMELAPSE_AS_IS){
        timelapser->timelapse_type = cv::detail::Timelapser::AS_IS;
    }else if(type == QTK_STITCH_TIMELAPSE_CROP){
        timelapser->timelapse_type = cv::detail::Timelapser::CROP;
    }else{ //不用
        timelapser->do_timelapse = 0;
    }

    if(timelapser->do_timelapse){
        timelapser->timelapser = cv::detail::Timelapser::createDefault(timelapser->timelapse_type);
    }
    

    return timelapser;
}

void qtk_stitch_timelapser_delete(qtk_stitch_timelapser_t* timelapser)
{
    if(timelapser){
        if(timelapser->timelapser){
            timelapser->timelapser = NULL;
        }
        if(timelapser->timelapse_prefix) 
            wtk_free(timelapser->timelapse_prefix);
        wtk_free(timelapser);
    }
}