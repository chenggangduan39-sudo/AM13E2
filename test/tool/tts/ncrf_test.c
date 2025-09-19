#include "qtk/tts/parse/ncrf/qtk_tts_ncrf.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
// charid  BMES pwi pwbi cwi cwbi  
//  start 将哪吒传奇更新到最新版本 end 2 1 2 1 3 1 1 2 1 2 1 4
// int ids[]={
//     71, 1562, 4679, 362, 387, 440, 284, 129, 61, 284, 1987, 73,
//     7, 7, 7, 4, 5, 4, 5, 7, 4, 5, 4, 5,
//     4, 4, 4, 4, 5, 4, 5, 4, 4, 5, 4, 5,
//     5, 5, 5, 4, 5, 4, 5, 5, 4, 5, 4, 5,
//     4, 5, 6, 7, 7, 8, 8, 9, 10, 10, 11, 11,
//     20, 21, 22, 23, 23, 24, 24, 25, 26, 26, 27, 27,
// };
/*
*/
//而连日疲软的电子股也出现了长期投资买盘   2 1 2 1 1 2 1 1 4 2 1 1 3 1 2 1 2 1 4
int ids[]={
667, 465, 47, 1483, 2452, 12, 622, 568, 1466, 33, 478, 193, 57, 205, 186, 348, 422, 1006, 1439,
7, 4, 5, 4, 5, 7, 4, 6, 5, 7, 4, 5, 7, 4, 5, 4, 5, 4, 5,
4, 4, 5, 4, 5, 4, 4, 5, 6, 4, 4, 5, 4, 4, 5, 4, 5, 4, 5,
5, 4, 5, 4, 5, 5, 7, 4, 5, 5, 4, 5, 5, 4, 5, 4, 5, 4, 5,
4, 5, 5, 6, 6, 7, 8, 8, 8, 9, 10, 10, 11, 12, 12, 13, 13, 14, 14,
17, 18, 18, 19, 19, 20, 21, 21, 21, 22, 23, 23, 24, 25, 25, 26, 26, 27, 27,
};

int main(int argc,char **argv)
{
    wtk_mati_t *ids_i = NULL,*ids_i1 = NULL;
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_tts_ncrf_cfg_t *cfg = NULL;
    qtk_tts_ncrf_t *ncrf = NULL;
    wtk_arg_t *arg = NULL;
    char *cfn = NULL;
    wtk_veci_t *out = NULL;

    arg = wtk_arg_new(argc,argv);
    if(arg == NULL) goto end;
    wtk_arg_get_str_s(arg,"c",&cfn);

    main_cfg = wtk_main_cfg_new_type(qtk_tts_ncrf_cfg,cfn);
    if(NULL == main_cfg) goto end;
    cfg = main_cfg->cfg;
    ncrf = qtk_tts_ncrf_new(cfg);

    ids_i = wtk_mati_new(6,sizeof(ids)/4/6);

    memcpy(ids_i->p,ids,sizeof(ids));
    ids_i1 = wtk_mati_transpose(ids_i);
    out = wtk_veci_new(ids_i1->row);
    qtk_tts_ncrf_process(ncrf,ids_i1,out);

    wtk_veci_print(out);

    wtk_mati_delete(ids_i1);
    wtk_veci_delete(out);
    wtk_mati_delete(ids_i);
    qtk_tts_ncrf_delete(ncrf);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
end:
    return 0;
}
