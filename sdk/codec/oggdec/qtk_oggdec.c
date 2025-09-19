#include "qtk_oggdec.h"
#include "wtk/core/wtk_alloc.h"

static void *process_header(ogg_packet *op, spx_int32_t enh_enabled,
                            spx_int32_t *frame_size, int *granule_frame_size,
                            spx_int32_t *rate, int *nframes, int forceMode,
                            int *channels, SpeexStereoState *stereo,
                            int *extra_headers) {
    void *st;
    const SpeexMode *mode;
    SpeexHeader *header;
    int modeID;
    SpeexCallback callback;

    header = speex_packet_to_header((char *)op->packet, op->bytes);
    if (!header) {
        fprintf(stderr, "Cannot read header\n");
        return NULL;
    }
    if (header->mode >= SPEEX_NB_MODES || header->mode < 0) {
        fprintf(
            stderr,
            "Mode number %d does not (yet/any longer) exist in this version\n",
            header->mode);
        free(header);
        return NULL;
    }

    modeID = header->mode;
    if (forceMode != -1)
        modeID = forceMode;

    mode = speex_lib_get_mode(modeID);

    if (header->speex_version_id > 1) {
        fprintf(stderr,
                "This file was encoded with Speex bit-stream version %d, which "
                "I don't know how to decode\n",
                header->speex_version_id);
        free(header);
        return NULL;
    }

    if (mode->bitstream_version < header->mode_bitstream_version) {
        fprintf(stderr, "The file was encoded with a newer version of Speex. "
                        "You need to upgrade in order to play it.\n");
        free(header);
        return NULL;
    }
    if (mode->bitstream_version > header->mode_bitstream_version) {
        fprintf(stderr,
                "The file was encoded with an older version of Speex. You "
                "would need to downgrade the version in order to play it.\n");
        free(header);
        return NULL;
    }

    st = speex_decoder_init(mode);
    if (!st) {
        fprintf(stderr, "Decoder initialization failed.\n");
        free(header);
        return NULL;
    }
    speex_decoder_ctl(st, SPEEX_SET_ENH, &enh_enabled);
    speex_decoder_ctl(st, SPEEX_GET_FRAME_SIZE, frame_size);
    *granule_frame_size = *frame_size;

    if (!*rate)
        *rate = header->rate;
    /* Adjust rate if --force-* options are used */
    if (forceMode != -1) {
        if (header->mode < forceMode) {
            *rate <<= (forceMode - header->mode);
            *granule_frame_size >>= (forceMode - header->mode);
        }
        if (header->mode > forceMode) {
            *rate >>= (header->mode - forceMode);
            *granule_frame_size <<= (header->mode - forceMode);
        }
    }

    speex_decoder_ctl(st, SPEEX_SET_SAMPLING_RATE, rate);

    *nframes = header->frames_per_packet;

    if (*channels == -1)
        *channels = header->nb_channels;

    if (!(*channels == 1)) {
        *channels = 2;
        callback.callback_id = SPEEX_INBAND_STEREO;
        callback.func = speex_std_stereo_request_handler;
        callback.data = stereo;
        speex_decoder_ctl(st, SPEEX_SET_HANDLER, &callback);
    }
    /*
       if (!quiet)
       {
       fprintf(stderr, "Decoding %d Hz audio using %s mode", *rate,
       mode->modeName);

       if (*channels == 1)
       fprintf(stderr, " (mono");
       else
       fprintf(stderr, " (stereo");

       if (header->vbr)
       fprintf(stderr, ", VBR)\n");
       else
       fprintf(stderr, ")\n");
       }
       */
    *extra_headers = header->extra_headers;
    free(header);
    return st;
}

qtk_oggdec_t *qtk_oggdec_new() {
    qtk_oggdec_t *dec;

    dec = (qtk_oggdec_t *)wtk_calloc(1, sizeof(qtk_oggdec_t));
    return dec;
}
int qtk_oggdec_delete(qtk_oggdec_t *dec) {
    wtk_free(dec);
    return 0;
}
void qtk_oggdec_start(qtk_oggdec_t *dec, qtk_oggdec_write_f write, void *hook) {
    SpeexStereoState stereo = SPEEX_STEREO_STATE_INIT;

    dec->write_f = write;
    dec->write_hook = hook;
    dec->stereo = stereo;
    dec->forceMode = -1;
    dec->enh_enabled = 1;
    dec->loss_percent = -1;
    ogg_sync_init(&(dec->oy));
    speex_bits_init(&(dec->bits));
    dec->page_granule = 0;
    dec->last_granule = 0;
    dec->frame_size = 0;
    dec->granule_frame_size = 0;
    dec->nframes = 2;
    dec->rate = 0;
    dec->packet_count = 0;
    dec->channels = -1;
    dec->extra_headers = 0;
    dec->eos = 0;
    dec->stream_init = 0;
    dec->st = 0;
    dec->speex_serialno = -1;
    return;
}
void qtk_oggdec_stop(qtk_oggdec_t *dec) {
    if (dec->st) {
        speex_decoder_destroy(dec->st);
        dec->st = 0;
    }
    speex_bits_destroy(&(dec->bits));
    if (dec->stream_init) {
        ogg_stream_clear(&(dec->os));
    }
    ogg_sync_clear(&(dec->oy));
}
int qtk_oggdec_feed(qtk_oggdec_t *dec, char *data, int len) {
    short output[MAX_FRAME_SIZE];
    short out[MAX_FRAME_SIZE];
    ogg_sync_state *oy = &(dec->oy);
    ogg_page *og = &(dec->og);
    ogg_stream_state *os = &(dec->os);
    ogg_packet *op = &(dec->op);
    SpeexBits *bits = &(dec->bits);
    int fsize = 200;
    char *s, *e, *p;
    int step, left;
    int packet_no;
    int ret = 0;
    int page_nb_packets;
    int skip_samples = 0;
    int i, j;

    s = data;
    e = s + len;
    while (s < e) {
        left = e - s;
        step = left < fsize ? left : fsize;
        /*Get the ogg buffer for writing*/
        p = ogg_sync_buffer(oy, step);
        memcpy(p, s, step);
        s += step;
        ogg_sync_wrote(oy, step);
        /*Loop for all complete pages we got (most likely only one)*/
        while (ogg_sync_pageout(oy, og) == 1) {
            if (dec->stream_init == 0) {
                ogg_stream_init(os, ogg_page_serialno(og));
                dec->stream_init = 1;
            }
            if (ogg_page_serialno(og) != os->serialno) {
                /* so all streams are read. */
                ogg_stream_reset_serialno(os, ogg_page_serialno(og));
            }
            /*Add page to the bitstream*/
            ogg_stream_pagein(os, og);
            dec->page_granule = ogg_page_granulepos(og);
            page_nb_packets = ogg_page_packets(og);
            if (dec->page_granule > 0 && dec->frame_size) {
                /* FIXME: shift the granule values if --force-* is specified */
                skip_samples =
                    dec->frame_size *
                    (page_nb_packets * dec->granule_frame_size * dec->nframes -
                     (dec->page_granule - dec->last_granule)) /
                    dec->granule_frame_size;
                if (ogg_page_eos(og)) {
                    skip_samples = -skip_samples;
                }
                /*else if (!ogg_page_bos(&og))
                  skip_samples = 0;*/
            } else {
                skip_samples = 0;
            }
            // wtk_debug("skip_samples=%d\n",skip_samples);
            /*printf ("page granulepos: %d %d %d\n", skip_samples,
             * page_nb_packets, (int)page_granule);*/
            dec->last_granule = dec->page_granule;
            /*Extract all available packets*/
            packet_no = 0;
            // wtk_debug("eos: %d\n",d->eos);
            while (!dec->eos && ogg_stream_packetout(os, op) == 1) {
                if (op->bytes >= 5 && !memcmp(op->packet, "Speex", 5)) {
                    dec->speex_serialno = os->serialno;
                }
                // wtk_debug("%d\n",speex_serialno);
                if (dec->speex_serialno == -1 ||
                    os->serialno != dec->speex_serialno) {
                    break;
                }
                /*If first packet, process as Speex header*/
                if (dec->packet_count == 0) {
                    dec->st = process_header(
                        op, dec->enh_enabled, &dec->frame_size,
                        &dec->granule_frame_size, &dec->rate, &dec->nframes,
                        dec->forceMode, &dec->channels, &dec->stereo,
                        &dec->extra_headers);
                    if (!dec->st) {
                        ret = -1;
                        goto end;
                    }
                    speex_decoder_ctl(dec->st, SPEEX_GET_LOOKAHEAD,
                                      &dec->lookahead);
                    if (!dec->nframes) {
                        dec->nframes = 1;
                    }
                } else if (dec->packet_count == 1) {
                    // print comment;
                } else if (dec->packet_count <= 1 + dec->extra_headers) {
                    /* Ignore extra headers */
                } else {
                    int lost = 0;

                    packet_no++;
                    // wtk_debug("pktno=%d\n",packet_no);
                    if (dec->loss_percent > 0 &&
                        100 * ((float)rand()) / RAND_MAX < dec->loss_percent)
                        lost = 1;

                    /*End of stream condition*/
                    if (op->e_o_s && os->serialno == dec->speex_serialno) {
                        /* don't care for anything except speex eos */
                        dec->eos = 1;
                    }
                    /*Copy Ogg packet to Speex bitstream*/
                    speex_bits_read_from(bits, (char *)op->packet, op->bytes);
                    for (j = 0; j != dec->nframes; j++) {
                        int ret;
                        /*Decode frame*/
                        if (!lost)
                            ret = speex_decode_int(dec->st, bits, output);
                        else
                            ret = speex_decode_int(dec->st, NULL, output);

                        /*for (i=0;i<frame_size*channels;i++)
                          printf ("%d\n", (int)output[i]);*/

                        if (ret == -1)
                            break;
                        if (ret == -2) {
                            fprintf(stderr,
                                    "Decoding error: corrupted stream?\n");
                            break;
                        }
                        if (speex_bits_remaining(bits) < 0) {
                            fprintf(stderr,
                                    "Decoding overflow: corrupted stream?\n");
                            break;
                        }
                        if (dec->channels == 2)
                            speex_decode_stereo_int(output, dec->frame_size,
                                                    &dec->stereo);

                        /*Convert to short and save to output file*/
                        for (i = 0; i < dec->frame_size * dec->channels; i++) {
                            out[i] = (output[i]);
                            // out[i] = le_short(output[i]);
                        }
                        {
                            int frame_offset = 0;
                            int new_frame_size = dec->frame_size;
                            /*printf ("packet %d %d\n", packet_no,
                             * skip_samples);*/
                            /*fprintf (stderr, "packet %d %d %d\n", packet_no,
                             * skip_samples, lookahead);*/
                            if (packet_no == 1 && j == 0 && skip_samples > 0) {
                                /*printf ("chopping first packet\n");*/
                                new_frame_size -= skip_samples + dec->lookahead;
                                frame_offset = skip_samples + dec->lookahead;
                            }
                            if (packet_no == page_nb_packets &&
                                skip_samples < 0) {
                                int packet_length =
                                    dec->nframes * dec->frame_size +
                                    skip_samples + dec->lookahead;
                                new_frame_size =
                                    packet_length - j * dec->frame_size;
                                if (new_frame_size < 0)
                                    new_frame_size = 0;
                                if (new_frame_size > dec->frame_size)
                                    new_frame_size = dec->frame_size;
                                /*printf ("chopping end: %d %d %d\n",
                                 * new_frame_size, packet_length, packet_no);*/
                            }
                            // wtk_debug("nfs=%d
                            // lad=%d\n",new_frame_size,d->lookahead);
                            if (new_frame_size > 0) {
                                dec->write_f(dec->write_hook,
                                             (char *)(out + frame_offset *
                                                                dec->channels),
                                             sizeof(short) * new_frame_size *
                                                 dec->channels);
                            }
                        }
                    }
                }
                dec->packet_count++;
            }
        }
    }
end:
    return ret;
}
