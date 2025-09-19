#ifndef __QTK_STITCH_TIMELAPSER_H__
#define __QTK_STITCH_TIMELAPSER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch_timelapser{
    int type;
    void *timelapser;
    int do_timelapse;
    int timelapse_type;
    char *timelapse_prefix;
}qtk_stitch_timelapser_t;

qtk_stitch_timelapser_t* qtk_stitch_timelapser_new(int type,char *prefix);
void qtk_stitch_timelapser_delete(qtk_stitch_timelapser_t* timelapser);

#ifdef __cplusplus
}
#endif


#endif