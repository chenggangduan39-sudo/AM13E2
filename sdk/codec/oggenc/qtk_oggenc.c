#include "qtk_oggenc.h"

spx_uint16_t _le_uint16(spx_uint16_t i) {
    static short d = 0x1122;
    return *((char *)&d) == 0x22 ? i : (i << 8 & 0xff00) | (i >> 8 & 0x00ff);
}

spx_uint32_t _le_uint32(spx_uint32_t i) {
    static short d = 0x1122;
    return *((char *)&d) == 0x22
               ? i
               : (i << 24 & 0xff000000) | (i << 8 & 0x00ff0000) |
                     (i >> 8 & 0x0000ff00) | (i >> 24 & 0x000000ff);
}

qtk_oggenc_t *qtk_oggenc_new(qtk_oggenc_cfg_t *cfg) {
    qtk_oggenc_t *enc;

    enc = (qtk_oggenc_t *)wtk_calloc(1, sizeof(qtk_oggenc_t));
    enc->cfg = cfg;
    enc->spx_mode = NULL;

    enc->spx_rate = 8000;
    enc->spx_channels = 1;
    enc->spx_frames_per_packet = 1;
    enc->spx_vbr = 0; // 0使quality生效, 1：quality不生效
    enc->spx_complexity = 2;
    enc->spx_quality = 8;
    enc->og_pages_per_second = 8;
    enc->write = NULL;
    enc->write_ths = NULL;
    ogg_stream_init(&enc->og_stream_state, rand());
    speex_bits_init(&enc->spx_bits);
    return enc;
}

void qtk_oggenc_delete(qtk_oggenc_t *enc) {
    speex_bits_destroy(&enc->spx_bits);
    ogg_stream_clear(&enc->og_stream_state);
    if (enc->spx_state) {
        speex_encoder_destroy(enc->spx_state);
        enc->spx_state = NULL;
    }
    wtk_free(enc);
}

void qtk_oggenc_reset(qtk_oggenc_t *enc) {
    ogg_stream_reset(&enc->og_stream_state);
    speex_bits_reset(&enc->spx_bits);

    if (enc->spx_state) {
        speex_encoder_destroy(enc->spx_state);
        enc->spx_state = NULL;
    }

    enc->spx_frame_id = 0;
    enc->spx_frame_buf_pos = 0;
    enc->spx_frame_id_pageout = 0;
}

void qtk_oggenc_set_write(qtk_oggenc_t *enc, void *ths,
                          qtk_oggenc_write_f write) {
    enc->write = write;
    enc->write_ths = ths;
}

int qtk_oggenc_write(qtk_oggenc_t *enc, ogg_page *p) {
    if (enc->write) {
        enc->write(enc->write_ths, (char *)p->header, p->header_len);
        enc->write(enc->write_ths, (char *)p->body, p->body_len);
    }
    // return cb(user_data, p->body, p->body_len, p->header, p->header_len);
    return 0;
}

int qtk_oggenc_write_speex_header(qtk_oggenc_t *enc) {

    int packet_size;
    int ret = 0;

    speex_init_header(&enc->spx_header, enc->spx_rate, enc->spx_channels,
                      enc->spx_mode);
    enc->spx_header.frames_per_packet = enc->spx_frames_per_packet;
    enc->spx_header.vbr = enc->spx_vbr;

    enc->og_packet.packet =
        (unsigned char *)speex_header_to_packet(&enc->spx_header, &packet_size);
    enc->og_packet.bytes = packet_size;
    enc->og_packet.b_o_s = 1;
    enc->og_packet.e_o_s = 0;
    enc->og_packet.granulepos = 0;
    enc->og_packet.packetno = 0;

    ogg_stream_packetin(&enc->og_stream_state, &enc->og_packet);
    free(enc->og_packet.packet);

    while (ogg_stream_flush(&enc->og_stream_state, &enc->og_page)) {
        ret = qtk_oggenc_write(enc, &enc->og_page);
        if (0 != ret) {
            break;
        }
    }

    return ret;
}

int qtk_oggenc_write_speex_comments_header(qtk_oggenc_t *enc) {
    int ret = 0;
    /*
       vorbis style comment,
       http://www.xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-810005 1)
       vendor_length: uint32, lsb first(little endian) 2) vendor_string: 3)
       user_comment_list_length: uint32, 0
       */
    spx_uint32_t _len, _le_len;

    speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, (void *)&enc->spx_version);
    /* snprintf(enc->spx_vendor, sizeof(enc->spx_vendor), "Encoded with Speex
     * %s", enc->spx_version); */
    sprintf(enc->spx_vendor, "Encoded with Speex %s", enc->spx_version);

    _len = strlen(enc->spx_vendor);
    _le_len = _le_uint32(_len);

    enc->spx_comments = calloc(1, 4 + _len + 4);
    memcpy(enc->spx_comments, &_le_len, 4);
    memcpy(enc->spx_comments + 4, enc->spx_vendor, _len);

    enc->og_packet.packet = (unsigned char *)enc->spx_comments;
    enc->og_packet.bytes = 4 + _len + 4;
    enc->og_packet.b_o_s = 0;
    enc->og_packet.e_o_s = 0;
    enc->og_packet.granulepos = 0;
    enc->og_packet.packetno = 1;

    ogg_stream_packetin(&enc->og_stream_state, &enc->og_packet);
    free(enc->og_packet.packet);

    while (ogg_stream_flush(&enc->og_stream_state, &enc->og_page)) {
        ret = qtk_oggenc_write(enc, &enc->og_page);
        if (0 != ret) {
            break;
        }
    }
    return ret;
}

int qtk_oggenc_write_speex_frame(qtk_oggenc_t *enc, int eof) {

    int i, bytes;
    int ret = 0;

    /* convert to little endian */
    for (i = 0; i < enc->spx_frame_buf_pos; i++)
        enc->spx_frame_buf[i] = _le_uint16(enc->spx_frame_buf[i]);

    /* complete a speex frame */
    for (i = enc->spx_frame_buf_pos; i < enc->spx_frame_size; i++)
        enc->spx_frame_buf[i] = 0;

    speex_bits_reset(&enc->spx_bits);
    speex_encode_int(enc->spx_state, enc->spx_frame_buf, &enc->spx_bits);

    speex_bits_insert_terminator(&enc->spx_bits);
    bytes = speex_bits_write(&enc->spx_bits, enc->spx_bits_buf,
                             sizeof(enc->spx_bits_buf));

    enc->og_packet.packet = (unsigned char *)enc->spx_bits_buf;
    enc->og_packet.bytes = bytes;
    enc->og_packet.b_o_s = 0;
    enc->og_packet.e_o_s = eof;

    /* packetno starting from 2, one speex frame per packet
       granulepos is the number of the last sample encoded in that packet */
    enc->og_packet.packetno = enc->spx_frame_id + 2;
    enc->og_packet.granulepos =
        (enc->spx_frame_id + 1) * enc->spx_frame_size - enc->spx_lookahead;

    ogg_stream_packetin(&enc->og_stream_state, &enc->og_packet);

    enc->spx_frame_id++;

    /* normal pageout */
    while (ogg_stream_pageout(&enc->og_stream_state, &enc->og_page)) {
        ret = qtk_oggenc_write(enc, &enc->og_page);
        if (0 != ret) {
            break;
        }
        enc->spx_frame_id_pageout = enc->spx_frame_id;
    }

    /*
     * force N pages per second, speex_frames = 1000 / 20 / og_pages_per_second
     */
    while (enc->spx_frame_id != enc->spx_frame_id_pageout &&
           (enc->spx_frame_id - enc->spx_frame_id_pageout) %
                   (int)(50 / enc->og_pages_per_second) ==
               0 &&
           ogg_stream_flush(&enc->og_stream_state, &enc->og_page)) {
        ret = qtk_oggenc_write(enc, &enc->og_page);
        if (0 != ret) {
            break;
        }
        enc->spx_frame_id_pageout = enc->spx_frame_id;
    }

    /* if eof, flush the remain pages */
    while (eof && ogg_stream_flush(&enc->og_stream_state, &enc->og_page)) {
        ret = qtk_oggenc_write(enc, &enc->og_page);
        if (0 != ret) {
            break;
        }
        enc->spx_frame_id_pageout = enc->spx_frame_id;
    }

    /* printf("speex_frame: %d, %d, %d\n", enc->spx_frame_id,
     * enc->spx_frame_buf_pos, eof); */
    enc->spx_frame_buf_pos = 0;

    return ret;
}

int qtk_oggenc_start(qtk_oggenc_t *enc, int rate, int channels, int bits) {
    float vbr_quality = 0.0;

    if ((rate != 8000 && rate != 16000) || channels != 1 || bits != 16) {
        return -1;
    }

    enc->spx_rate = rate;
    enc->spx_channels = channels;
    enc->spx_bits_per_sample = bits;
    enc->spx_quality = enc->cfg->spx_quality; // 0 ~ 10,默认8
    enc->spx_complexity = enc->cfg->spx_complexity;
    enc->spx_vbr = enc->cfg->spx_vbr;

    enc->spx_frame_id = 0;
    enc->spx_frame_buf_pos = 0;
    enc->spx_frame_id_pageout = 0;

    /* init speex encoder */
    if (enc->spx_rate == 8000)
        enc->spx_mode = speex_lib_get_mode(SPEEX_MODEID_NB);
    else
        enc->spx_mode = speex_lib_get_mode(SPEEX_MODEID_WB);

    enc->spx_state = speex_encoder_init(enc->spx_mode);

    speex_encoder_ctl(enc->spx_state, SPEEX_GET_FRAME_SIZE,
                      &enc->spx_frame_size);
    speex_encoder_ctl(enc->spx_state, SPEEX_GET_LOOKAHEAD, &enc->spx_lookahead);

    speex_encoder_ctl(enc->spx_state, SPEEX_SET_SAMPLING_RATE,
                      (void *)&enc->spx_rate);
    speex_encoder_ctl(enc->spx_state, SPEEX_SET_COMPLEXITY,
                      (void *)&enc->spx_complexity);
    if (enc->spx_vbr) {
        speex_encoder_ctl(enc->spx_state, SPEEX_SET_VBR, (void *)&enc->spx_vbr);
        vbr_quality = (float)enc->spx_quality;
        speex_encoder_ctl(enc->spx_state, SPEEX_SET_VBR_QUALITY,
                          (void *)&vbr_quality);
    } else {
        speex_encoder_ctl(enc->spx_state, SPEEX_SET_QUALITY,
                          (void *)&enc->spx_quality);
    }
    /* write speex headers */
    qtk_oggenc_write_speex_header(enc);
    qtk_oggenc_write_speex_comments_header(enc);
    return 0;
}

int qtk_oggenc_encode(qtk_oggenc_t *enc, const void *data, int size, int eof) {
    short *pack_data;
    int pack_data_pos;
    int cpsamples;

    /* main encoding loop, one frame per iteration, and write speex frames */
    pack_data = (short *)data;
    pack_data_pos = 0;
    size /= 2;
    for (;;) {
        cpsamples = enc->spx_frame_size - enc->spx_frame_buf_pos;
        cpsamples = cpsamples <= (size - pack_data_pos)
                        ? cpsamples
                        : (size - pack_data_pos);
        memcpy(enc->spx_frame_buf + enc->spx_frame_buf_pos,
               pack_data + pack_data_pos, cpsamples * 2);
        enc->spx_frame_buf_pos += cpsamples;
        pack_data_pos += cpsamples;

        if (enc->spx_frame_buf_pos < enc->spx_frame_size)
            break;

        qtk_oggenc_write_speex_frame(enc, 0);
    }
    if (eof) {
        qtk_oggenc_write_speex_frame(enc, 1);
    }
    return 0;
}
