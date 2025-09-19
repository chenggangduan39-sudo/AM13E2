#ifndef WTK_WORLD_SYNTH_H_
#define WTK_WORLD_SYNTH_H_
typedef struct wtk_rfft wtk_rfft_t;

// int wtk_world_synth_main(wtk_rfft_t *rf, float *win, int fft_size, int fs, float frame_period, float *f0, int f0_len, float *sp, int sp_len, float *ap, int ap_len, char *fn_out, void *outio);

#ifdef __cplusplus
extern "C" {
#endif

int wtk_world_synth(wtk_rfft_t *rf, float *win, int fft_size, int fs, float frame_period, float *f0, int f0_length, float *sp, int sp_len, float *ap, int ap_len, char *fn_out, void *outio);

// int wtk_world_synth(wtk_rfft_t *rf, float *win, int fft_size, int fs, float frame_period, float *f0, int f0_len, float *sp, int sp_len, float *ap, int ap_len, char *fn_out, void *outio)
// {
//     return wtk_world_synth_main(rf, win, fft_size, fs, frame_period, f0, f0_len, sp, sp_len, ap, ap_len, fn_out, outio);
// };
#ifdef __cplusplus
}
#endif
#endif