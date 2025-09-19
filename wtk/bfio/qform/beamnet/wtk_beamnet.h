#ifndef WTK_BFIO_QFORM_BEAMNET_WTK_BEAMNET_H
#define WTK_BFIO_QFORM_BEAMNET_WTK_BEAMNET_H
#include "wtk/bfio/qform/beamnet/wtk_beamnet_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_beamnet wtk_beamnet_t;
typedef enum wtk_beamnet_state wtk_beamnet_state_e;
typedef enum wtk_beamnet_event_type wtk_beamnet_event_type_e;
typedef void(*wtk_beamnet_notify_f)(void *ths,short *output,int len);

struct wtk_beamnet {
    wtk_beamnet_cfg_t *cfg;

	wtk_strbuf_t **mic;
	wtk_strbuf_t **sp;
    float theta;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
	float *synthesis_mem;

	wtk_complex_t **fft;
	wtk_complex_t **fft_sp;
	wtk_complex_t *fftx;

	float *out;

	void *ths;
	wtk_beamnet_notify_f notify;

#ifdef ONNX_DEC
    qtk_onnxruntime_t *seperator;
    qtk_onnxruntime_t *beamformer;
	OrtValue **sep_caches;
	OrtValue **beam_caches;
    int *seperator_out_len;
    int *beamformer_out_len;
#endif

    float **feature;
    float *x;
    float *mix_spec;
    float *time_delay;
    float *mic_aptd;
    float *echo_aptd;
    wtk_complex_t **mic_covar;
    wtk_complex_t **ideal_phase_covar;
    wtk_complex_t **ideal_phase_shift;
    wtk_complex_t **freq_covar;
    wtk_complex_t *freq_covar_sum;
    float *LPS;
    float *echo_LPS;
    float **IPDs;
    float *FDDF;
    wtk_complex_t *mean_mat;
    wtk_complex_t **input_norm;

    float *speech_est_real;
    float *speech_est_imag;
    float *noise_est_real;
    float *noise_est_imag;
    float *speech_mask;
    float *noise_mask;
    float *src_est_real;
    float *src_est_imag;
};

wtk_beamnet_t *wtk_beamnet_new(wtk_beamnet_cfg_t *cfg);
void wtk_beamnet_delete(wtk_beamnet_t *beamnet);
void wtk_beamnet_start(wtk_beamnet_t *beamnet, float theta, float phi);
void wtk_beamnet_reset(wtk_beamnet_t *beamnet);
void wtk_beamnet_feed(wtk_beamnet_t *beamnet, short *data, int len, int is_end);
void wtk_beamnet_set_notify(wtk_beamnet_t *beamnet,void *ths,wtk_beamnet_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
