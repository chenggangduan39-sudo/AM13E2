#ifndef WTK_FST_NET_WTK_LAT_NET_H_
#define WTK_FST_NET_WTK_LAT_NET_H_
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lat_net wtk_lat_net_t;
struct wtk_lat_net
{
	wtk_lat_t *input;
	wtk_fst_net2_t *output;
	wtk_heap_t *heap;
};

wtk_lat_net_t* wtk_lat_net_new();
void wtk_lat_net_delete(wtk_lat_net_t *n);
void wtk_lat_net_reset(wtk_lat_net_t *n);
void wtk_lat_net_process(wtk_lat_net_t *n,wtk_lat_t *input,wtk_fst_net2_t *output);
#ifdef __cplusplus
};
#endif
#endif
