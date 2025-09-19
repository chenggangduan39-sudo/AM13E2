#include "qtk/avspeech/qtk_avspeech_separator.h"
#include "wtk/asr/vad/wtk_vad2.h"
#include "wtk/bfio/agc/wtk_agc.h"
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse.h"
#include "wtk/core/wtk_type.h"

#define MAX_QFORM9_CHN 16

typedef struct {
    uint8_t lip_buf[88 * 88];
    wtk_queue_node_t q_n;
} lip_buf_node_t;

static void on_vp_feat_(vp_extractor_t *vp, float *data, int len) {
    vp->sep->vp = wtk_malloc(sizeof(float) * len);
    memcpy(vp->sep->vp, data, len * sizeof(float));
    vp->sep->vp_len = len;
}

static void on_vp_extractor_vad_(vp_extractor_t *vp, wtk_vad2_cmd_t cmd,
                                 short *data, int len) {
    int i;
    if (vp->sep->vp) {
        return;
    }
    switch (cmd) {
    case WTK_VAD2_START:
        vp->speaking = 1;
        break;
    case WTK_VAD2_DATA:
        if (vp->speaking) {
            wtk_strbuf_push(vp->speech_segement, (char *)data,
                            len * sizeof(short));
        }
        break;
    case WTK_VAD2_END:
        vp->speaking = 0;
        if (vp->speech_segement->pos >=
            vp->sep->cfg->vp_extractor_duration_min_s * 16000 * sizeof(short)) {
            qtk_nnrt_value_t output;
            for (i = 0; i < vp->sep->cfg->dnsmos_nsample; i++) {
                vp->sep->dnsmos_segment[i] =
                    ((short *)vp->speech_segement
                         ->data)[i %
                                 (vp->speech_segement->pos / sizeof(short))];
            }
            qtk_nnrt_feed(vp->sep->dnsmos, vp->sep->dnsmos_input, 0);
            qtk_nnrt_run(vp->sep->dnsmos);
            qtk_nnrt_get_output(vp->sep->dnsmos, &output, 0);
            float *score = qtk_nnrt_value_get_data(vp->sep->dnsmos, output);
            if (score[2] > vp->sep->cfg->vp_dnsmos_thresh) {
                wtk_cmask_pse_new_vp(vp->pse);
                wtk_cmask_pse_set_notify2(vp->pse, vp,
                                          (wtk_cmask_pse_notify_f2)on_vp_feat_);
                wtk_cmask_pse_feed_vp(
                    vp->pse, (short *)vp->speech_segement->data,
                    vp->speech_segement->pos / sizeof(short), 0);
                wtk_cmask_pse_feed_vp(vp->pse, NULL, 0, 1);
                wtk_cmask_pse_delete_vp(vp->pse);
            }
            qtk_nnrt_value_release(vp->sep->dnsmos, output);
        }
        wtk_strbuf_reset(vp->speech_segement);
        break;
    case WTK_VAD2_CANCEL:
        break;
    }
}

static void on_vp_audio_(vp_extractor_t *vp, short *data, int len) {
    wtk_strbuf_push(vp->denoised_buf, (char *)data, len * sizeof(short));
}

static void vp_extractor_enter_(vp_extractor_t *vp) {
    wtk_vad2_start(vp->vad);
    vp->speaking = 0;
}

static void vp_extractor_leave_(vp_extractor_t *vp) {
    wtk_cmask_pse_reset(vp->pse);
    wtk_vad2_reset(vp->vad);
    wtk_strbuf_reset(vp->speech_segement);
    wtk_strbuf_reset(vp->denoised_buf);
}

static vp_extractor_t *vp_extractor_new_(qtk_avspeech_separator_t *sep) {
    vp_extractor_t *vp = wtk_malloc(sizeof(vp_extractor_t));
    vp->pse = wtk_cmask_pse_new(&sep->cfg->pse);
    wtk_cmask_pse_set_notify(vp->pse, vp, (wtk_cmask_pse_notify_f)on_vp_audio_);
    vp->vad_cfg = wtk_vad_cfg_new(sep->cfg->vp_extractor_vad_fn);
    vp->vad = wtk_vad2_new(vp->vad_cfg);
    wtk_vad2_set_notify(vp->vad, vp, (wtk_vad2_notify_f)on_vp_extractor_vad_);
    vp->speech_segement = wtk_strbuf_new(1024, 1);
    vp->denoised_buf = wtk_strbuf_new(1024, 1);
    vp->sep = sep;
    vp_extractor_enter_(vp);
    return vp;
}

static void vp_extractor_delete_(vp_extractor_t *vp) {
    vp_extractor_leave_(vp);
    wtk_cmask_pse_delete(vp->pse);
    wtk_vad2_delete(vp->vad);
    wtk_vad_cfg_delete(vp->vad_cfg);
    wtk_strbuf_delete(vp->speech_segement);
    wtk_strbuf_delete(vp->denoised_buf);
    wtk_free(vp);
}

static void vp_extractor_feed_(vp_extractor_t *vp, short *data, int len) {
    if (!vp->sep->vp) {
        wtk_vad2_feed(vp->vad, (char *)data, len * sizeof(short), 0);
    }
}

typedef struct {
    int id;
    wtk_queue_node_t q_n;
    wtk_queue_node_t hoard_n;
    wtk_stack_t *lip_buf;
    wtk_stack_t *raw_audio_buf;
    uint32_t end_frame_idx;
    wtk_vad2_t *vad;
    wtk_agc_t *agc;
    wtk_vad_cfg_t *vad_cfg;
    qtk_avspeech_separator_t *sep;
    int skip_nsamples;
    uint32_t raise_pos;
    unsigned raise_face_patch : 1;
    unsigned speaking : 1;
    unsigned vad_speaking : 1;
} tracklet_t;

typedef struct {
    wtk_queue_node_t node;
    wtk_queue_node_t hoard_n;
    uint8_t *I;
} image_msg_t;

typedef struct {
    wtk_queue_node_t node;
    wtk_queue_node_t hoard_n;
    qtk_avspeech_lip_result_t result;
    float landmark[10];
    wtk_heap_t *heap;
} lip_result_msg_t;

#ifdef QTK_USE_MPP

// 功能：MPP执行编码
MPP_RET test_mpp_run_yuv(uint8_t *image, MppApi *mpi, MppCtx ctx,
                         unsigned char **H264_buf, int *length,
                         qtk_avspeech_separator_t *sep) {
    MpiEncTestData *p = sep->encoder_params_ptr;
    MPP_RET ret;
    MppFrame frame = NULL;
    MppPacket packet = NULL;
    void *buf = mpp_buffer_get_ptr(p->frm_buf);
    // read_yuv_buffer((RK_U8*)buf, yuvImg, p->width, p->height);
    memcpy(buf, image, p->width * p->height * 3);
    // read_rgb_image((RK_U8*)buf, image, p->width, p->height, p->hor_stride,
    // p->ver_stride, p->fmt);
    ret = mpp_frame_init(&frame);
    if (ret) {
        mpp_err_f("mpp_frame_init failed\n");
        goto RET;
    }
    //
    mpp_frame_set_width(frame, p->width);
    mpp_frame_set_height(frame, p->height);
    mpp_frame_set_hor_stride(frame, p->hor_stride);
    mpp_frame_set_ver_stride(frame, p->ver_stride);
    mpp_frame_set_fmt(frame, p->fmt);
    mpp_frame_set_buffer(frame, p->frm_buf);
    mpp_frame_set_eos(frame, p->frm_eos);

    ret = mpi->encode_put_frame(ctx, frame);
    if (ret) {
        mpp_err("mpp encode put frame failed\n");
        goto RET;
    }
    ret = mpi->encode_get_packet(ctx, &packet);
    if (ret) {
        mpp_err("mpp encode get packet failed\n");
        goto RET;
    }

    if (packet) {
        void *ptr = mpp_packet_get_pos(packet);
        size_t len = mpp_packet_get_length(packet);
        p->pkt_eos = mpp_packet_get_eos(packet);
        //
        // H264_buf = new unsigned char[len];
        *H264_buf = (unsigned char *)malloc(len);

        memcpy(*H264_buf, ptr, len);

        *length = len;
        mpp_packet_deinit(&packet);
        p->stream_size += len;
        p->frame_count++;
        if (p->pkt_eos) {
            mpp_log("found last packet\n");
            mpp_assert(p->frm_eos);
        }
    }
RET:
    return ret;
}

// 功能：设置MPP编码器参数
// 说明：1-输入控制配置；2-码率控制配置；3-协议控制配置；4-SEI模式配置
MPP_RET test_mpp_setup(MpiEncTestData *p) {
    MPP_RET ret;
    MppApi *mpi;
    MppCtx ctx;
    MppEncCodecCfg *codec_cfg;
    MppEncPrepCfg *prep_cfg;
    MppEncRcCfg *rc_cfg;

    if (NULL == p) {
        return MPP_ERR_NULL_PTR;
    }

    mpi = p->mpi;
    ctx = p->ctx;
    codec_cfg = &p->codec_cfg;
    prep_cfg = &p->prep_cfg;
    rc_cfg = &p->rc_cfg;

    p->fps = 25;
    p->gop = 60;
    // p->bps = p->width * p->height / 5 * p->fps;
    p->bps = 4096 * 1024;

    // 1--输入控制配置
    prep_cfg->change = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                       MPP_ENC_PREP_CFG_CHANGE_ROTATION |
                       MPP_ENC_PREP_CFG_CHANGE_FORMAT;
    prep_cfg->width = p->width;
    prep_cfg->height = p->height;
    prep_cfg->hor_stride = p->hor_stride;
    prep_cfg->ver_stride = p->ver_stride;
    prep_cfg->format = p->fmt;
    prep_cfg->rotation = MPP_ENC_ROT_0;
    ret = mpi->control(ctx, MPP_ENC_SET_PREP_CFG, prep_cfg);
    if (ret) {
        mpp_err("mpi control enc set prep cfg failed ret %d\n", ret);
        goto RET;
    }

    // 2--码率控制配置
    rc_cfg->change = MPP_ENC_RC_CFG_CHANGE_ALL;
    rc_cfg->rc_mode = MPP_ENC_RC_MODE_VBR;
    rc_cfg->quality = MPP_ENC_RC_QUALITY_CQP;

    if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_CBR) {
        // constant bitrate has very small bps range of 1/16 bps
        rc_cfg->bps_target = p->bps;
        rc_cfg->bps_max = p->bps * 17 / 16;
        rc_cfg->bps_min = p->bps * 15 / 16;
    } else if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_VBR) {
        if (rc_cfg->quality == MPP_ENC_RC_QUALITY_CQP) {
            // constant QP does not have bps
            // rc_cfg->bps_target   = -1;
            // rc_cfg->bps_max      = -1;
            // rc_cfg->bps_min      = -1;
            rc_cfg->bps_target = p->bps;
            rc_cfg->bps_max = p->bps * 17 / 16;
            rc_cfg->bps_min = p->bps * 1 / 16;
        } else {
            // variable bitrate has large bps range
            rc_cfg->bps_target = p->bps;
            rc_cfg->bps_max = p->bps * 17 / 16;
            rc_cfg->bps_min = p->bps * 1 / 16;
        }
    }

    // fix input / output frame rate
    rc_cfg->fps_in_flex = 0;
    rc_cfg->fps_in_num = p->fps;
    rc_cfg->fps_in_denom = 1;
    rc_cfg->fps_out_flex = 0;
    rc_cfg->fps_out_num = p->fps;
    rc_cfg->fps_out_denom = 1;

    rc_cfg->gop = p->gop;
    rc_cfg->skip_cnt = 0;

    mpp_log("mpi_enc_test bps %d fps %d gop %d\n", rc_cfg->bps_target,
            rc_cfg->fps_out_num, rc_cfg->gop);
    ret = mpi->control(ctx, MPP_ENC_SET_RC_CFG, rc_cfg);
    if (ret) {
        mpp_err("mpi control enc set rc cfg failed ret %d\n", ret);
        goto RET;
    }

    // 3--协议控制配置
    codec_cfg->coding = p->type;
    codec_cfg->h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
                             MPP_ENC_H264_CFG_CHANGE_ENTROPY |
                             MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;

    // 66  - Baseline profile
    // 77  - Main profile
    // 100 - High profile
    codec_cfg->h264.profile = 100;
    /*
     * H.264 level_idc parameter
     * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
     * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
     * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
     * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
     * 50 / 51 / 52         - 4K@30fps
     */
    codec_cfg->h264.level = 40;
    codec_cfg->h264.entropy_coding_mode = 1;
    codec_cfg->h264.cabac_init_idc = 0;
    // codec_cfg->h264.qp_min = 0;
    // codec_cfg->h264.qp_max = 50;
    // codec_cfg->h264.transform8x8_mode = 0;
    ret = mpi->control(ctx, MPP_ENC_SET_CODEC_CFG, codec_cfg);
    if (ret) {
        mpp_err("mpi control enc set codec cfg failed ret %d\n", ret);
        goto RET;
    }
    // 4--SEI模式配置
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        goto RET;
    }

RET:
    return ret;
}

// 功能：MPP上下文初始化
// 说明：根据MpiEncTestCmd参数设置MpiEncTestData参数
MPP_RET test_ctx_init(MpiEncTestData **data, MpiEncTestCmd *cmd) {
    MpiEncTestData *p = NULL;
    MPP_RET ret = MPP_OK;
    if (!data || !cmd) {
        mpp_err_f("invalid input data %p cmd %p\n", data, cmd);
        return MPP_ERR_NULL_PTR;
    }
    p = mpp_calloc(MpiEncTestData, 1);
    if (!p) {
        mpp_err_f("create MpiEncTestData failed\n");
        ret = MPP_ERR_MALLOC;
        goto RET;
    }
    // get paramter from cmd
    p->width = cmd->width;
    p->height = cmd->height;
    p->hor_stride = MPP_ALIGN(cmd->width, 8) * 3;
    p->ver_stride = MPP_ALIGN(cmd->height, 2);
    p->fmt = cmd->format;
    p->type = cmd->type;
    p->num_frames = cmd->num_frames;
    // p->frame_size   = p->hor_stride * p->ver_stride * 3 / 2;
    p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64);
    p->packet_size = p->width * p->height;

RET:
    *data = p;
    return ret;
}

// 功能：初始化MPP编码器
static MpiEncTestData *test_mpp_run_yuv_init(MpiEncTestData *p, int width,
                                             int height, MppApi *mpi,
                                             MppCtx ctx) {
    MPP_RET ret;
    //
    MpiEncTestCmd cmd;
    cmd.width = width;
    cmd.height = height;
    cmd.type = MPP_VIDEO_CodingAVC;
    // cmd.format = MPP_FMT_YUV420P;
    cmd.format = MPP_FMT_RGB888;
    // cmd.format = MPP_FMT_BGR888;
    cmd.num_frames = 0;
    ret = test_ctx_init(&p, &cmd);
    if (ret) {
        mpp_err_f("test data init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }
    //
    mpp_log("p->frame_size = %d----------------\n", p->frame_size);
    ret = mpp_buffer_get(NULL, &p->frm_buf, p->frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
        goto MPP_TEST_OUT;
    }
    //
    mpp_log("mpi_enc_test encoder test start w %d h %d type %d\n", p->width,
            p->height, p->type);
    // encoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }
    //
    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }
    //
    ret = test_mpp_setup(p);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    mpi = p->mpi;
    ctx = p->ctx;

    //
    if (p->type == MPP_VIDEO_CodingHEVC) {
        MppPacket packet = NULL;
        ret = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
        if (ret) {
            mpp_err("mpi control enc get extra info failed\n");
        }
        // get and write sps/pps for H.264
        if (packet) {
            void *ptr = mpp_packet_get_pos(packet);
            size_t len = mpp_packet_get_length(packet);
            packet = NULL;
        }
    }
    return p;
MPP_TEST_OUT:
    return p;
}

#endif

static image_msg_t *image_msg_alloc_(qtk_avspeech_separator_t *sep) {
    image_msg_t *msg = wtk_malloc(sizeof(image_msg_t));
    msg->I = wtk_malloc(sep->cfg->height * sep->cfg->width * 3);
    return msg;
}

static int image_msg_free_(image_msg_t *msg) {
    wtk_free(msg->I);
    wtk_free(msg);
    return 0;
}

static image_msg_t *image_msg_new_(qtk_avspeech_separator_t *sep, uint8_t *I) {
    image_msg_t *msg = wtk_lockhoard_pop(&sep->image_msgs);
    memcpy(msg->I, I, sep->cfg->height * sep->cfg->width * 3);
    return msg;
}

static void image_msg_delete_(qtk_avspeech_separator_t *sep, image_msg_t *msg) {
    wtk_lockhoard_push(&sep->image_msgs, msg);
}

static lip_result_msg_t *lip_result_msg_alloc_(qtk_avspeech_separator_t *sep) {
    wtk_heap_t *heap = wtk_heap_new(4096);
    lip_result_msg_t *msg = wtk_malloc(sizeof(lip_result_msg_t));
    msg->heap = heap;
    return msg;
}

static int lip_result_msg_free_(lip_result_msg_t *msg) {
    wtk_heap_delete(msg->heap);
    wtk_free(msg);
    return 0;
}

static lip_result_msg_t *
lip_result_msg_new_(qtk_avspeech_separator_t *sep,
                    qtk_avspeech_lip_result_t *result) {
    lip_result_msg_t *msg = wtk_lockhoard_pop(&sep->lip_msgs);
    msg->result.state = result->state;
    msg->result.id = result->id;
    msg->result.frame_idx = result->frame_idx;
    if (result->state == QTK_AVSPEECH_LIP_DATA) {
        int lip_image_bytes =
            sizeof(uint8_t) * sep->lip->cfg->H * sep->lip->cfg->W;
        int face_patch_bytes =
            sizeof(uint8_t) * 3 * result->face_H * result->face_W;
        msg->result.lip_image = wtk_heap_malloc(msg->heap, lip_image_bytes);
        memcpy(msg->result.lip_image, result->lip_image, lip_image_bytes);
        msg->result.face_patch = wtk_heap_malloc(msg->heap, face_patch_bytes);
        memcpy(msg->result.face_patch, result->face_patch, face_patch_bytes);
        msg->result.face_H = result->face_H;
        msg->result.face_W = result->face_W;
        msg->result.roi = result->roi;
    }
    return msg;
}

static void lip_result_msg_delete_(qtk_avspeech_separator_t *sep,
                                   lip_result_msg_t *msg) {
    wtk_heap_reset(msg->heap);
    wtk_lockhoard_push(&sep->lip_msgs, msg);
}

static void notifier_audio_(qtk_avspeech_separator_t *sep, tracklet_t *tl,
                            short *wav, int len) {
    qtk_avspeech_separator_result_t result;
    result.type = QTK_AVSPEECH_SEPARATOR_AUDIO;
    wtk_strbuf_expand(sep->composed_audio, len * 2 * sizeof(short));
    result.audio.wav = (short *)sep->composed_audio->data;
    result.audio.len = len;


    memcpy(sep->composed_audio->data, wav, len * sizeof(short));
    wtk_stack_pop(tl->raw_audio_buf,
                  sep->composed_audio->data + len * sizeof(short),
                  len * sizeof(short));
    tl->raise_pos += len;
    sep->notifier(sep->upval, tl->id, tl->end_frame_idx, &result);
}

static void tracklet_pad_audio_(tracklet_t *tl) {
    short pad_audio[1024] = {0};
    int passed_nsample =
        (tl->end_frame_idx - tl->lip_buf->len / tl->sep->cfg->patch_bytes) *
        (tl->sep->cfg->sampling_rate / tl->sep->cfg->visual_voice.fps);
    if (passed_nsample > tl->raise_pos) {
        uint32_t pad_len = passed_nsample - tl->raise_pos;
        while (pad_len > 0) {
            int raise_len =
                min(pad_len, sizeof(pad_audio) / sizeof(pad_audio[0]));
            pad_len -= raise_len;
            notifier_audio_(tl->sep, tl, pad_audio, raise_len);
            tl->raise_pos += raise_len;
        }
    }
}

static void on_vad_(tracklet_t *tl, wtk_vad2_cmd_t cmd, short *data, int len) {
    switch (cmd) {
    case WTK_VAD2_START:
        tl->vad_speaking = 1;
        break;
    case WTK_VAD2_DATA:
        if (tl->vad_speaking) {
            notifier_audio_(tl->sep, tl, data, len);
        } else {
            wtk_stack_pop(tl->raw_audio_buf, NULL, len * 2);
        }
        break;
    case WTK_VAD2_END:
        tl->vad_speaking = 0;
        notifier_audio_(tl->sep, tl, NULL, 0);
        break;
    case WTK_VAD2_CANCEL:
        wtk_debug("should not be here\n");
        break;
    }
}

static void on_agc_(tracklet_t *tl, short *wav, int len) {
    if (tl->sep->cfg->use_vad) {
        wtk_vad2_feed(tl->vad, (char *)wav, len * sizeof(short), 0);
    } else {
        notifier_audio_(tl->sep, tl, wav, len);
    }
}

static tracklet_t *tracklet_new_(qtk_avspeech_separator_t *sep) {
    tracklet_t *tl = wtk_malloc(sizeof(tracklet_t));
    tl->lip_buf = wtk_stack_new(sep->cfg->video_segment_bytes * 2,
                                sep->cfg->video_segment_bytes * 32, 1);
    if (sep->cfg->use_vad) {
        tl->vad_cfg = wtk_vad_cfg_new(sep->cfg->vad_fn);
        tl->vad = wtk_vad2_new(tl->vad_cfg);
        wtk_vad2_set_notify(tl->vad, tl, (wtk_vad2_notify_f)on_vad_);
    }
    if (sep->cfg->use_agc) {
        tl->agc = wtk_agc_new(&sep->cfg->agc);
        wtk_agc_set_notify(tl->agc, tl, (wtk_agc_notify_f)on_agc_);
    }
    tl->end_frame_idx = 0;
    tl->sep = sep;
    tl->raw_audio_buf = wtk_stack_new(1024, 1024 * 1024 * 32, 1);
    return tl;
}

static int tracklet_delete_(tracklet_t *tl) {
    wtk_stack_delete(tl->lip_buf);
    if (tl->sep->cfg->use_agc) {
        wtk_agc_delete(tl->agc);
    }
    if (tl->sep->cfg->use_vad) {
        wtk_vad2_delete(tl->vad);
        wtk_vad_cfg_delete(tl->vad_cfg);
    }
    wtk_stack_delete(tl->raw_audio_buf);
    wtk_free(tl);
    return 0;
}

static tracklet_t *tracklet_enter_(qtk_avspeech_separator_t *sep, int id) {
    tracklet_t *tl;
    tl = wtk_hoard_pop(&sep->tracklet_hub);
    tl->id = id;
    tl->end_frame_idx = 0;
    tl->skip_nsamples = 0;
    tl->raise_face_patch = 0;
    tl->speaking = 0;
    tl->raise_pos = 0;
    wtk_queue_push(&sep->cur_tracklets, &tl->q_n);
    if (sep->cfg->use_vad) {
        wtk_vad2_start(tl->vad);
        tl->vad_speaking = 0;
    }
    if (sep->cfg->use_agc) {
        wtk_agc_start(tl->agc);
    }
    wtk_stack_reset(tl->raw_audio_buf);
    return tl;
}

static void tracklet_leave_(qtk_avspeech_separator_t *sep, tracklet_t *tl) {
    wtk_queue_remove(&sep->cur_tracklets, &tl->q_n);
    wtk_stack_reset(tl->lip_buf);
    if (sep->cfg->use_agc) {
        wtk_agc_feed(tl->agc, NULL, 0, 1);
        wtk_agc_reset(tl->agc);
    }
    if (sep->cfg->use_vad) {
        wtk_vad2_feed(tl->vad, NULL, 0, 1);
        wtk_vad2_reset(tl->vad);
    } else {
        notifier_audio_(tl->sep, tl, NULL, 0);
    }
    wtk_hoard_push(&sep->tracklet_hub, tl);
}

static void tracklet_progress_(qtk_avspeech_separator_t *sep, tracklet_t *tl,
                               qtk_avspeech_lip_result_t *result) {
    wtk_stack_push(tl->lip_buf, (char *)result->lip_image,
                   sep->cfg->patch_bytes);
    tl->end_frame_idx = result->frame_idx + 1;
    if (tl->raise_face_patch == 0) {
        qtk_avspeech_separator_result_t sep_result;
        sep_result.type = QTK_AVSPEECH_SEPARATOR_AVATOR;
        sep_result.avator.I = result->face_patch;
        sep_result.avator.width = result->face_W;
        sep_result.avator.height = result->face_H;
        tl->sep->notifier(tl->sep->upval, tl->id, tl->end_frame_idx,
                          &sep_result);
        tl->raise_face_patch = 1;
    }
}

static void tracklet_process_denoised_audio_(qtk_avspeech_separator_t *sep,
                                             tracklet_t *tl, short *audio,
                                             int len) {
    if (sep->cfg->use_agc) {
        wtk_agc_feed(tl->agc, audio, len, 0);
    } else {
        if (sep->cfg->use_vad) {
            wtk_vad2_feed(tl->vad, (char *)audio, len * 2, 0);
        } else {
            notifier_audio_(sep, tl, audio, len);
        }
    }
}

static void av_tracklet_forward_(qtk_avspeech_separator_t *sep,
                                 tracklet_t *tl) {
    int nsample = sep->cfg->audio_segment_bytes / sizeof(short);
    float normalizer = qtk_avspeech_visual_voice_get_audio_feature(
        sep->visual_voice, sep->audio_segment, sep->vv_feature);
    qtk_avspeech_visual_voice_feed(sep->visual_voice, sep->vv_feature,
                                   sep->video_segment);
    qtk_avspeech_visual_voice_synthesis_audio(
        sep->visual_voice, sep->audio_segment, sep->vv_feature, normalizer);
    short *audio = sep->audio_segment + tl->skip_nsamples;
    int len = nsample - tl->skip_nsamples;
    tracklet_process_denoised_audio_(sep, tl, audio, len);
    tl->skip_nsamples = sep->cfg->audio_segment_bytes *
                        (sep->cfg->sliding_factor - 1) /
                        (sizeof(short) * sep->cfg->sliding_factor);
}

static int av_sync_tracklet_(qtk_avspeech_separator_t *sep, tracklet_t *tl) {
    int is_end = 1;
    uint32_t tl_start_idx =
        tl->end_frame_idx - tl->lip_buf->len / sep->cfg->patch_bytes;
    uint32_t tl_segment_end_idx =
        tl_start_idx + sep->cfg->visual_voice.num_frames;
    wtk_lock_lock(&sep->audio_buf_guard);
    uint32_t audio_end_idx = sep->audio_poped_frames +
                             sep->audio_buf->len / sep->cfg->audio_frame_bytes;
    if (audio_end_idx >= tl_segment_end_idx) {
        uint32_t audio_skiped_frames = tl_start_idx - sep->audio_poped_frames;
        wtk_stack_read_at(
            sep->audio_buf, audio_skiped_frames * sep->cfg->audio_frame_bytes,
            (char *)sep->audio_segment, sep->cfg->audio_segment_bytes);
        audio_skiped_frames = tl_start_idx - sep->raw_audio_poped_frames;
        int valid_bytes =
            sep->cfg->audio_segment_bytes - tl->skip_nsamples * sizeof(short);
        char *data = wtk_malloc(valid_bytes); // TODO
        wtk_stack_read_at(sep->audio_buf_raw,
                          audio_skiped_frames * sep->cfg->audio_frame_bytes +
                              tl->skip_nsamples * sizeof(short),
                          data, valid_bytes);
        if (sep->cfg->use_vpdenoise) {
            vp_extractor_feed_(sep->vp_extractor, (short *)data,
                               valid_bytes / 2);
        }
        wtk_stack_push(tl->raw_audio_buf, data, valid_bytes);
        wtk_free(data);
        wtk_lock_unlock(&sep->audio_buf_guard);
        wtk_stack_read_at(tl->lip_buf, 0, (char *)sep->video_segment,
                          sep->cfg->video_segment_bytes);
        wtk_stack_pop(tl->lip_buf, NULL,
                      sep->cfg->video_segment_bytes / sep->cfg->sliding_factor);
        av_tracklet_forward_(sep, tl);
        is_end = 0;
    } else {
        wtk_lock_unlock(&sep->audio_buf_guard);
    }
    return is_end;
}

static void pop_stale_audio_buf_(qtk_avspeech_separator_t *sep,
                                 uint32_t min_video_frame_idx,
                                 wtk_stack_t *audio_buf,
                                 uint32_t *poped_frames) {
    if (min_video_frame_idx <= *poped_frames) {
        return;
    }
    uint32_t poped = min_video_frame_idx - *poped_frames;
    wtk_lock_lock(&sep->audio_buf_guard);
    uint32_t audio_reserved = audio_buf->len / sep->cfg->audio_frame_bytes;
    poped = min(poped, audio_reserved);
    wtk_stack_pop(audio_buf, NULL, poped * sep->cfg->audio_frame_bytes);
    wtk_lock_unlock(&sep->audio_buf_guard);
    *poped_frames += poped;
}

static void pop_stale_audio_(qtk_avspeech_separator_t *sep,
                             uint32_t min_video_frame_idx) {
    pop_stale_audio_buf_(sep, min_video_frame_idx, sep->audio_buf,
                         &sep->audio_poped_frames);
    if (sep->cfg->use_qform9) {
        pop_stale_audio_buf_(sep, min_video_frame_idx, sep->audio_buf_raw,
                             &sep->raw_audio_poped_frames);
    } else {
        sep->raw_audio_poped_frames = sep->audio_poped_frames;
    }
}

static void av_sync_(qtk_avspeech_separator_t *sep,
                     uint32_t min_video_frame_idx) {
    wtk_queue_node_t *node;
    int is_end;
    uint32_t min_video_frame_idx_in = min_video_frame_idx;
    do {
        is_end = 1;
        min_video_frame_idx = min_video_frame_idx_in;
        for (node = sep->cur_tracklets.pop; node; node = node->next) {
            tracklet_t *tl = data_offset2(node, tracklet_t, q_n);
            if (tl->lip_buf->len < sep->cfg->video_segment_bytes) {
                min_video_frame_idx =
                    min(min_video_frame_idx,
                        tl->end_frame_idx -
                            tl->lip_buf->len / sep->cfg->patch_bytes);
                continue;
            }
            if (av_sync_tracklet_(sep, tl) == 0) {
                is_end = 0;
            }
            min_video_frame_idx = min(
                min_video_frame_idx,
                tl->end_frame_idx - tl->lip_buf->len / sep->cfg->patch_bytes);
        }
        if (sep->cur_tracklets.length == 0) {
            min_video_frame_idx = sep->empty_end_frame_idx;
            if (sep->in_vp && min_video_frame_idx > sep->vp_frame_cursor) {
                min_video_frame_idx = sep->vp_frame_cursor;
            }
        }
        pop_stale_audio_(sep, min_video_frame_idx);
    } while (!is_end);
    if (sep->cfg->sync_audio) {
        for (node = sep->cur_tracklets.pop; node; node = node->next) {
            tracklet_t *tl = data_offset2(node, tracklet_t, q_n);
            tracklet_pad_audio_(tl);
        }
    }
}

static void vp_denoise_enter_(qtk_avspeech_separator_t *sep) {
    wtk_cmask_pse_start(sep->vp_extractor->pse);
    sep->vp_fake_tracklet = tracklet_enter_(sep, -1);
    sep->vp_frame_cursor = sep->empty_end_frame_idx;
}

static void vp_denoise_process_(qtk_avspeech_separator_t *sep) {
    uint32_t audio_end_idx = sep->audio_poped_frames +
                             sep->audio_buf->len / sep->cfg->audio_frame_bytes;
    audio_end_idx = min(audio_end_idx, sep->empty_end_frame_idx);
    tracklet_t *tl = sep->vp_fake_tracklet;
    if (audio_end_idx > sep->vp_frame_cursor) {
        short *wav_alloc = NULL;
        short *wav;
        uint32_t valid_frames = audio_end_idx - sep->vp_frame_cursor;
        uint32_t audio_skiped_frames =
            sep->vp_frame_cursor - sep->audio_poped_frames;
        if (sep->cfg->audio_frame_bytes * valid_frames <=
            sep->cfg->audio_segment_bytes) {
            wav = sep->audio_segment;
        } else {
            wav = wav_alloc =
                wtk_malloc(sep->cfg->audio_frame_bytes * valid_frames);
            wtk_debug("%d %d\n", valid_frames, sep->cfg->audio_frame_bytes);
        }
        wtk_stack_read_at(
            sep->audio_buf, audio_skiped_frames * sep->cfg->audio_frame_bytes,
            (char *)wav, sep->cfg->audio_frame_bytes * valid_frames);
        wtk_stack_push(tl->raw_audio_buf, (char *)wav,
                       valid_frames * sep->cfg->audio_frame_bytes);
        wtk_cmask_pse_feed(sep->vp_extractor->pse, wav,
                           valid_frames * sep->cfg->audio_frame_bytes / 2, 0);
        tracklet_process_denoised_audio_(
            sep, tl, (short *)sep->vp_extractor->denoised_buf->data,
            sep->vp_extractor->denoised_buf->pos / 2);
        wtk_strbuf_reset(sep->vp_extractor->denoised_buf);
        if (wav_alloc) {
            wtk_free(wav_alloc);
        }
        sep->vp_frame_cursor = audio_end_idx;
    }
}

static void vp_denoise_leave_(qtk_avspeech_separator_t *sep) {
    wtk_cmask_pse_reset(sep->vp_extractor->pse);
    tracklet_leave_(sep, sep->vp_fake_tracklet);
    sep->vp_fake_tracklet = NULL;
}

static int lip_result_dispatch_(qtk_avspeech_separator_t *sep,
                                qtk_avspeech_lip_result_t *result) {
    int found = 0;
    wtk_queue_node_t *node;
    tracklet_t *tl;
    qtk_avspeech_separator_result_t sep_result;
    if (result->state == QTK_AVSPEECH_LIP_START) {
        tracklet_enter_(sep, result->id);
        return 0;
    }
    if (result->state == QTK_AVSPEECH_LIP_EMPTY) {
        sep->empty_end_frame_idx = result->frame_idx + 1;
        if (sep->vp) {
            if (sep->in_vp) {
                vp_denoise_process_(sep);
            } else {
                vp_denoise_enter_(sep);
                sep->in_vp = 1;
            }
        }
        return 0;
    }
    for (node = sep->cur_tracklets.pop; node; node = node->next) {
        tl = data_offset2(node, tracklet_t, q_n);
        if (tl->id == result->id) {
            found = 1;
            break;
        }
    }
    if (!found) {
        wtk_debug("shoud not be here, wild tracklet found [%d]\n", result->id);
        return -1;
    }

    if (result->state == QTK_AVSPEECH_LIP_END) {
        tracklet_leave_(sep, tl);
        sep_result.type = QTK_AVSPEECH_SEPARATOR_NOPERSON;
        // sep_result.type = QTK_AVSPEECH_SEPARATOR_FACE_ROI;
        // sep_result.face_roi.x1 = 0;
        // sep_result.face_roi.x2 = 0;
        // sep_result.face_roi.y1 = 0;
        // sep_result.face_roi.y2 = 0;
        sep->notifier(tl->sep->upval, tl->id, tl->end_frame_idx, &sep_result);
        return 0;
    }

    if (sep->vp && sep->in_vp) {
        vp_denoise_leave_(sep);
        sep->in_vp = 0;
    }

    sep_result.type = QTK_AVSPEECH_SEPARATOR_FACE_ROI;
    sep_result.face_roi = result->roi;
    sep->notifier(tl->sep->upval, tl->id, tl->end_frame_idx, &sep_result);
    tracklet_progress_(sep, tl, result);
    return 0;
}

static int on_lip_(qtk_avspeech_separator_t *sep,
                   qtk_avspeech_lip_result_t *result) {
    lip_result_msg_t *msg = lip_result_msg_new_(sep, result);
    wtk_blockqueue_push(&sep->sep_input, &msg->node);
    return 0;
}

static int lip_process_(qtk_avspeech_separator_t *sep, wtk_thread_t *t) {
    wtk_queue_node_t *node;
    image_msg_t *msg;
    static int cnt = 0;

    while (1) {
        if (++cnt % 25 == 0) {
            //            wtk_debug("lip_process sep->lip_input.length: %d\n",
            //                      sep->lip_input.length);
            cnt = 0;
        }

        node = wtk_blockqueue_pop(&sep->lip_input, -1, NULL);

        if (node == NULL) {
            wtk_debug("lip_process_ node is NULL! lip_process "
                      "sep->lip_input.length: %d\n",
                      sep->lip_input.length);
            break;
        }
        msg = data_offset2(node, image_msg_t, node);

        qtk_avspeech_lip_feed(sep->lip, msg->I, sep->cfg->height,
                              sep->cfg->width);
        image_msg_delete_(sep, msg);
    }

    return 0;
}

static int sep_process_(qtk_avspeech_separator_t *sep, wtk_thread_t *t) {
    wtk_queue_node_t *node;
    lip_result_msg_t *msg;
    static int cnt = 0;

    while (1) {
        uint32_t min_video_frame_idx = 0xFFFFFFFF;
        if (++cnt % 25 == 0) {
            //            wtk_debug("sep_process sep->sep_input.length: %d\n",
            //                      sep->sep_input.length);
            cnt = 0;
        }

        node = wtk_blockqueue_pop(&sep->sep_input, -1, NULL);
        if (node == NULL) {
            wtk_debug("sep_process_ node is NULL! sep_process "
                      "sep->sep_input.length: %d\n",
                      sep->sep_input.length);
            break;
        }

        msg = data_offset2(node, lip_result_msg_t, node);
        lip_result_dispatch_(sep, &msg->result);
        lip_result_msg_delete_(sep, msg);
        av_sync_(sep, min_video_frame_idx);
    }
    return 0;
}

static void raise_sep_audio_(qtk_avspeech_separator_t *sep, short *wav,
                             int len) {
    wtk_lock_lock(&sep->audio_buf_guard);
    wtk_stack_push(sep->audio_buf, (char *)wav, len * sizeof(short));
    wtk_lock_unlock(&sep->audio_buf_guard);
}

static void on_qform9_(qtk_avspeech_separator_t *sep, short *wav, int len,
                       int is_end) {
    raise_sep_audio_(sep, wav, len);
}

qtk_avspeech_separator_t *
qtk_avspeech_separator_new(qtk_avspeech_separator_cfg_t *cfg, void *upval,
                           qtk_avspeech_separator_notifier_t notifier) {

    int64_t shape[5];
    qtk_avspeech_separator_t *sep =
        wtk_malloc(sizeof(qtk_avspeech_separator_t));
    sep->cfg = cfg;
    sep->lip = qtk_avspeech_lip_new(&cfg->lip, sep,
                                    (qtk_avspeech_lip_notifier_t)on_lip_);
    sep->audio_segment = wtk_malloc(cfg->audio_segment_bytes);
    sep->video_segment = wtk_malloc(cfg->video_segment_bytes);
    sep->audio_buf = wtk_stack_new(cfg->audio_segment_bytes * 2,
                                   cfg->audio_segment_bytes * 32, 1);
    sep->audio_poped_frames = 0;
    sep->raw_audio_poped_frames = 0;
    sep->upval = upval;
    sep->notifier = notifier;
    sep->empty_end_frame_idx = 0;
    wtk_hoard_init(&sep->tracklet_hub, offsetof(tracklet_t, hoard_n),
                   cfg->tracklet_max_free, (wtk_new_handler_t)tracklet_new_,
                   (wtk_delete_handler_t)tracklet_delete_, sep);
    wtk_queue_init(&sep->cur_tracklets);
    wtk_lock_init(&sep->audio_buf_guard);
    wtk_lockhoard_init(&sep->lip_msgs, offsetof(lip_result_msg_t, hoard_n),
                       cfg->lip_result_max_free,
                       (wtk_new_handler_t)lip_result_msg_alloc_,
                       (wtk_delete_handler_t)lip_result_msg_free_, sep);
    wtk_lockhoard_init(&sep->image_msgs, offsetof(image_msg_t, hoard_n),
                       cfg->image_max_free, (wtk_new_handler_t)image_msg_alloc_,
                       (wtk_delete_handler_t)image_msg_free_, sep);

    wtk_blockqueue_init(&sep->lip_input);
    wtk_blockqueue_init(&sep->sep_input);

    if (cfg->use_qform9) {
        if (cfg->qform9.stft2.channel > MAX_QFORM9_CHN) {
            wtk_debug("should not be here\n");
            goto err;
        }
        sep->qform9 = wtk_qform9_new(&cfg->qform9);
        wtk_qform9_set_notify(sep->qform9, sep,
                              (wtk_qform9_notify_f)on_qform9_);
        wtk_qform9_start(sep->qform9, 90, 0);
        sep->audio_buf_raw = wtk_stack_new(cfg->audio_segment_bytes * 2,
                                           cfg->audio_segment_bytes * 32, 1);
    } else {
        sep->audio_buf_raw = sep->audio_buf;
    }

    if (cfg->use_vpdenoise) {
        sep->dnsmos = qtk_nnrt_new(&cfg->dnsmos);
        sep->dnsmos_segment = wtk_malloc(sizeof(float) * cfg->dnsmos_nsample);
        shape[0] = 1;
        shape[1] = cfg->dnsmos_nsample;
        sep->dnsmos_input =
            qtk_nnrt_value_create_external(sep->dnsmos, QTK_NNRT_VALUE_ELEM_F32,
                                           shape, 2, sep->dnsmos_segment);
    }

    sep->composed_audio = wtk_strbuf_new(1024, 1);
    sep->visual_voice = qtk_avspeech_visual_voice_new(&cfg->visual_voice);
    {
        int nsample = cfg->visual_voice.audio_segment_duration * 16000;
        int K = cfg->visual_voice.stft.n_fft / 2 + 1;
        int T = nsample / cfg->visual_voice.stft.hop_length + 1;
        sep->vv_feature = wtk_malloc(sizeof(wtk_complex_t) * T * K);
    }
    sep->vp = NULL;
    sep->in_vp = 0;
    sep->vp_fake_tracklet = NULL;
    if (sep->cfg->use_vpdenoise) {
        sep->vp_extractor = vp_extractor_new_(sep);
    }

    wtk_thread_init(&sep->lip_thread, (thread_route_handler)lip_process_, sep);
    wtk_thread_set_name(&sep->lip_thread, "lip");
    wtk_thread_init(&sep->sep_thread, (thread_route_handler)sep_process_, sep);
    wtk_thread_set_name(&sep->sep_thread, "sep");
    wtk_thread_start(&sep->lip_thread);
    wtk_thread_start(&sep->sep_thread);

#ifdef QTK_USE_MPP
    if (sep->cfg->use_mpp) {
        sep->encoder_params_ptr =
            test_mpp_run_yuv_init(sep->encoder_params_ptr, sep->cfg->width,
                                  sep->cfg->height, sep->mpi, sep->ctx);
        sep->mpi = sep->encoder_params_ptr->mpi;
        sep->ctx = sep->encoder_params_ptr->ctx;

        sep->fp_output = fopen(
            "/sdcard/Android/data/com.qdreamer.hvs/files/output.h264", "wb");
    }
#endif
    return sep;
err:
    return NULL;
}

void qtk_avspeech_separator_delete(qtk_avspeech_separator_t *sep) {
#ifdef QTK_USE_MPP
    if (sep->cfg->use_mpp) {
        if (sep->fp_output) {
            fclose(sep->fp_output);
        }
        if (sep->ctx) {
            mpp_destroy(sep->ctx);
            sep->ctx = NULL;
        }
    }

#endif
    wtk_queue_node_t *node, *next;
    tracklet_t *tl;
    wtk_blockqueue_wake(&sep->lip_input);
    wtk_thread_join(&sep->lip_thread);
    qtk_avspeech_lip_delete(sep->lip);
    wtk_blockqueue_wake(&sep->sep_input);
    wtk_thread_join(&sep->sep_thread);
    wtk_thread_clean(&sep->lip_thread);
    wtk_thread_clean(&sep->sep_thread);

    for (node = sep->cur_tracklets.pop; node; node = next) {
        next = node->next;
        tl = data_offset2(node, tracklet_t, q_n);
        tracklet_leave_(sep, tl);
    }

    if (sep->cfg->use_qform9) {
        wtk_qform9_delete(sep->qform9);
        wtk_stack_delete(sep->audio_buf_raw);
    }
    if (sep->cfg->use_vpdenoise) {
        qtk_nnrt_value_release(sep->dnsmos, sep->dnsmos_input);
        wtk_free(sep->dnsmos_segment);
        qtk_nnrt_delete(sep->dnsmos);
    }
    wtk_hoard_clean(&sep->tracklet_hub);
    wtk_free(sep->video_segment);
    wtk_free(sep->audio_segment);
    wtk_stack_delete(sep->audio_buf);
    wtk_lock_clean(&sep->audio_buf_guard);
    wtk_lockhoard_clean(&sep->lip_msgs);
    wtk_lockhoard_clean(&sep->image_msgs);
    wtk_strbuf_delete(sep->composed_audio);
    qtk_avspeech_visual_voice_delete(sep->visual_voice);
    wtk_free(sep->vv_feature);
    if (sep->vp) {
        wtk_free(sep->vp);
    }
    if (sep->cfg->use_vpdenoise) {
        vp_extractor_delete_(sep->vp_extractor);
    }
    wtk_free(sep);
}

int qtk_avspeech_separator_feed_image(qtk_avspeech_separator_t *sep,
                                      uint8_t *image) {
    image_msg_t *msg = image_msg_new_(sep, image);

    if (msg == NULL) {
        wtk_debug("msg is NULL\n");
        return -1;
    }
    static int cnt = 0;
    if (++cnt % 25 == 0) {
        //     wtk_debug("feed image:sep->lip_input.length: %d\n",
        //               sep->lip_input.length);
        cnt = 0;
        // printf("lip_process sep->lip_input.length: %d\n",
        // sep->lip_input.length);
    }
#ifdef QTK_USE_MPP
    if (sep->cfg->use_mpp) {
        unsigned char *H264_buf = NULL;
        int H264_buf_length = 0;
        test_mpp_run_yuv(image, sep->mpi, sep->ctx, &H264_buf, &H264_buf_length,
                         sep);
        fwrite(H264_buf, 1, H264_buf_length, sep->fp_output);
        if (H264_buf) {
            free(H264_buf);
        }
    }
#endif

    wtk_blockqueue_push(&sep->lip_input, &msg->node);
    return 0;
}

int qtk_avspeech_separator_feed_audio(qtk_avspeech_separator_t *sep, short *wav,
                                      int len) {
    static int cnt = 0;
    if (++cnt % 25 == 0) {
        //        wtk_debug("feed audio:sep->audio_buf->len: %d\n",
        //        sep->audio_buf->len);
        cnt = 0;
    }

    if (sep->cfg->use_qform9) {
        int i, j;
        short audio_buf[32000];
        short *data[MAX_QFORM9_CHN];
        int channel = sep->cfg->qform9.stft2.channel;
        if (len * channel > sizeof(audio_buf) / sizeof(audio_buf[0])) {
            wtk_debug("should not be here\n");
            return -1;
        }
        for (i = 0; i < channel; i++) {
            data[i] = audio_buf + i * len;
        }
        for (i = 0; i < len; i++) {
            for (j = 0; j < channel; j++) {
                data[j][i] = *wav++;
            }
        }
        wtk_lock_lock(&sep->audio_buf_guard);
        wtk_stack_push(sep->audio_buf_raw, (char *)data[0],
                       len * sizeof(short));
        wtk_lock_unlock(&sep->audio_buf_guard);
        wtk_qform9_feed(sep->qform9, data, len, 0);
    } else {
        raise_sep_audio_(sep, wav, len);
    }
    return 0;
}

int qtk_avspeech_separator_reset(qtk_avspeech_separator_t *sep) {
    wtk_queue_node_t *node, *next;
    tracklet_t *tl;
    wtk_blockqueue_wake(&sep->lip_input);
    wtk_thread_join(&sep->lip_thread);
    wtk_blockqueue_wake(&sep->sep_input);
    wtk_thread_join(&sep->sep_thread);

    for (node = sep->cur_tracklets.pop; node; node = next) {
        next = node->next;
        tl = data_offset2(node, tracklet_t, q_n);
        tracklet_leave_(sep, tl);
    }

    if (sep->cfg->use_qform9) {
        wtk_qform9_reset(sep->qform9);
    }
    wtk_stack_reset(sep->audio_buf);
    sep->audio_poped_frames = 0;
    sep->raw_audio_poped_frames = 0;
    wtk_lockhoard_clean(&sep->lip_msgs);
    wtk_lockhoard_clean(&sep->image_msgs);
    wtk_lockhoard_init(&sep->lip_msgs, offsetof(lip_result_msg_t, hoard_n),
                       sep->cfg->lip_result_max_free,
                       (wtk_new_handler_t)lip_result_msg_alloc_,
                       (wtk_delete_handler_t)lip_result_msg_free_, sep);
    wtk_lockhoard_init(&sep->image_msgs, offsetof(image_msg_t, hoard_n),
                       sep->cfg->image_max_free,
                       (wtk_new_handler_t)image_msg_alloc_,
                       (wtk_delete_handler_t)image_msg_free_, sep);

    sep->empty_end_frame_idx = 0;
    qtk_avspeech_lip_reset(sep->lip);
    wtk_thread_start(&sep->lip_thread);
    wtk_thread_start(&sep->sep_thread);
    return 0;
}
