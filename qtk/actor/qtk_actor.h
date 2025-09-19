#ifndef C888272F_4EAF_40B7_9DD4_0FEC394E8801
#define C888272F_4EAF_40B7_9DD4_0FEC394E8801

#include "wtk/os/wtk_lockqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_actor qtk_actor_t;

struct qtk_actor {
    wtk_lockqueue_t ctx;
};

#ifdef __cplusplus
};
#endif
#endif /* C888272F_4EAF_40B7_9DD4_0FEC394E8801 */