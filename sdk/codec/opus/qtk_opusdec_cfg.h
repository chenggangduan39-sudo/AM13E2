#ifndef __QTK_UTIL_QTK_OPUSDEC_H__
#define __QTK_UTIL_QTK_OPUSDEC_H__

#ifdef __cplusplus
extern "C" {
#endif

struct qtk_opusdec_cfg {
    int max_payload_bytes;
    int use_inbandfec;
};

#ifdef __cplusplus
};
#endif
#endif