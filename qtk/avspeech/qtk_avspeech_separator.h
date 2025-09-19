#ifndef G_16893D295632436D9997128FEEF5DF22
#define G_16893D295632436D9997128FEEF5DF22

#include "qtk/avspeech/qtk_avspeech_lip.h"
#include "qtk/avspeech/qtk_avspeech_separator_cfg.h"
#include "qtk/avspeech/qtk_avspeech_visual_voice.h"
#include "wtk/asr/vad/wtk_vad2.h"
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse.h"
#include "wtk/bfio/qform/wtk_qform9.h"
#include "wtk/core/wtk_stack.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_thread.h"

typedef enum {
    QTK_AVSPEECH_SEPARATOR_AVATOR,
    QTK_AVSPEECH_SEPARATOR_AUDIO,
    QTK_AVSPEECH_SEPARATOR_FACE_ROI,
    QTK_AVSPEECH_SEPARATOR_NOPERSON,
} qtk_avspeech_separator_result_type_t;

typedef struct {
    qtk_avspeech_separator_result_type_t type;
    union {
        struct {
            short *wav;
            int len;
        } audio;
        qtk_cv_bbox_t face_roi;
        struct {
            uint8_t *I;
            int width;
            int height;
        } avator;
    };
} qtk_avspeech_separator_result_t;

typedef struct qtk_avspeech_separator qtk_avspeech_separator_t;
typedef int (*qtk_avspeech_separator_notifier_t)(
    void *upval, int id, uint32_t frame_idx,
    qtk_avspeech_separator_result_t *result);

#ifdef QTK_USE_MPP
#include "mpp_env.h"
#include "mpp_mem.h"
#include "rk_mpi.h"
#include "mpi_dec_utils.h"
#include "mpp_common.h"
typedef struct
{
    MppCodingType   type;
    RK_U32          width;
    RK_U32          height;
    MppFrameFormat  format;
    RK_U32          num_frames;
} MpiEncTestCmd;
typedef struct
{
    //global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_U32 frame_count;
    RK_U64 stream_size;
    //input ang output file
    FILE *fp_input;
    FILE *fp_output;
    //input and output
    MppBuffer frm_buf;
    MppEncSeiMode sei_mode;
    //base flow context
    MppCtx ctx;
    MppApi *mpi;
    MppEncPrepCfg prep_cfg;
    MppEncRcCfg rc_cfg;
    MppEncCodecCfg codec_cfg;
    //paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride; //horizontal stride
    RK_U32 ver_stride; //vertical stride
    MppFrameFormat fmt;
    MppCodingType type;
    RK_U32 num_frames;
    //resources
    size_t frame_size;
    //NOTE: packet buffer may overflow
    size_t packet_size;
    //rate control runtime parameter
    RK_S32 gop;
    RK_S32 fps;
    RK_S32 bps;
} MpiEncTestData;

#endif

typedef struct {
    wtk_cmask_pse_t *pse;
    wtk_vad2_t *vad;
    wtk_vad_cfg_t *vad_cfg;
    wtk_strbuf_t *speech_segement;
    wtk_strbuf_t *denoised_buf;
    unsigned speaking : 1;
    qtk_avspeech_separator_t *sep;
} vp_extractor_t;

struct qtk_avspeech_separator {
    qtk_avspeech_separator_cfg_t *cfg;
    qtk_avspeech_lip_t *lip;
    qtk_nnrt_t *dnsmos;
    wtk_qform9_t *qform9;
    wtk_hoard_t tracklet_hub;
    wtk_queue_t cur_tracklets;
    uint8_t *video_segment;
    short *audio_segment;
    wtk_stack_t *audio_buf;
    wtk_stack_t *audio_buf_raw;
    uint32_t audio_poped_frames;
    uint32_t raw_audio_poped_frames;
    wtk_thread_t lip_thread;
    wtk_thread_t sep_thread;
    wtk_blockqueue_t lip_input;
    wtk_blockqueue_t sep_input;
    wtk_lock_t audio_buf_guard;
    wtk_lockhoard_t lip_msgs;
    wtk_lockhoard_t image_msgs;
    uint32_t empty_end_frame_idx;

    wtk_strbuf_t *composed_audio;

    void *upval;
    qtk_avspeech_separator_notifier_t notifier;

    float *dnsmos_segment;
    qtk_nnrt_value_t dnsmos_input;
    qtk_avspeech_visual_voice_t *visual_voice;
    wtk_complex_t *vv_feature;

    float *vp;
    int vp_len;
    vp_extractor_t *vp_extractor;
    void *vp_fake_tracklet;
    uint32_t vp_frame_cursor;
    unsigned in_vp : 1;

#ifdef QTK_USE_MPP
    MpiEncTestData encoder_params;
    MpiEncTestData *encoder_params_ptr;
    MppApi *mpi;
    MppCtx ctx;
    FILE *fp_output;
    // MpiEncTestData *encoder_params_ptr = &encoder_params;
#endif

};

qtk_avspeech_separator_t *
qtk_avspeech_separator_new(qtk_avspeech_separator_cfg_t *cfg, void *upval,
                           qtk_avspeech_separator_notifier_t notifier);
void qtk_avspeech_separator_delete(qtk_avspeech_separator_t *m);
int qtk_avspeech_separator_feed_image(qtk_avspeech_separator_t *sep,
                                      uint8_t *image);
int qtk_avspeech_separator_feed_audio(qtk_avspeech_separator_t *sep, short *wav,
                                      int len);
int qtk_avspeech_separator_reset(qtk_avspeech_separator_t *sep);

#endif

