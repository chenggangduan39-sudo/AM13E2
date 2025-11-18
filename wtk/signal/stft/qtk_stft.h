#ifndef F01A321B_D0AF_A9C5_C9D8_E7A06C986625
#define F01A321B_D0AF_A9C5_C9D8_E7A06C986625

typedef enum {
    QTK_STFT_FORWARD,
    QTK_STFT_BACKWARD,
} qtk_stft_direction_t;

typedef struct {
    float *data;
    int len;
} qtk_stft_synthesis_sample_t;

typedef int (*qtk_stft_notifier_t)(void *upval, void *data,
                                   qtk_stft_direction_t direction);

#endif /* F01A321B_D0AF_A9C5_C9D8_E7A06C986625 */
