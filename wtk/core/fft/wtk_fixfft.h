#ifndef WTK_CORE_FFT_WTK_FIXFFT
#define WTK_CORE_FFT_WTK_FIXFFT
#include "wtk/core/wtk_type.h" 
#ifdef __cplusplus
extern "C" {
#endif
#define QCONST16(x,bits) ((wtk_fix16_t)(.5+(x)*(((wtk_fix32_t)1)<<(bits))))
#define QCONST32(x,bits) ((wtk_fix32_t)(.5+(x)*(((wtk_fix32_t)1)<<(bits))))

#define NEG16(x) (-(x))
#define NEG32(x) (-(x))
#define EXTRACT16(x) ((wtk_fix16_t)(x))
#define EXTEND32(x) ((wtk_fix32_t)(x))
#define SHR16(a,shift) ((a) >> (shift))
#define SHL16(a,shift) ((a) << (shift))
#define SHR32(a,shift) ((a) >> (shift))
#define SHL32(a,shift) ((a) << (shift))
#define PSHR16(a,shift) (SHR16((a)+((1<<((shift))>>1)),shift))
#define PSHR32(a,shift) (SHR32((a)+((EXTEND32(1)<<((shift))>>1)),shift))
#define VSHR32(a, shift) (((shift)>0) ? SHR32(a, shift) : SHL32(a, -(shift)))
#define SATURATE16(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))
#define SATURATE32(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))

#define SATURATE32PSHR(x,shift,a) (((x)>=(SHL32(a,shift))) ? (a) : \
                                   (x)<=-(SHL32(a,shift)) ? -(a) : \
                                   (PSHR32(x, shift)))

#define SHR(a,shift) ((a) >> (shift))
#define SHL(a,shift) ((wtk_fix32_t)(a) << (shift))
#define PSHR(a,shift) (SHR((a)+((EXTEND32(1)<<((shift))>>1)),shift))
#define SATURATE(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))


#define ADD16(a,b) ((wtk_fix16_t)((wtk_fix16_t)(a)+(wtk_fix16_t)(b)))
#define SUB16(a,b) ((wtk_fix16_t)(a)-(wtk_fix16_t)(b))
#define ADD32(a,b) ((wtk_fix32_t)(a)+(wtk_fix32_t)(b))
#define SUB32(a,b) ((wtk_fix32_t)(a)-(wtk_fix32_t)(b))


/* result fits in 16 bits */
#define MULT16_16_16(a,b)     ((((wtk_fix16_t)(a))*((wtk_fix16_t)(b))))

/* (wtk_fix32_t)(wtk_fix16_t) gives TI compiler a hint that it's 16x16->32 multiply */
#define MULT16_16(a,b)     (((wtk_fix32_t)(wtk_fix16_t)(a))*((wtk_fix32_t)(wtk_fix16_t)(b)))
#define MULT16_32(a,b)     (((wtk_fix32_t)(a))*((wtk_fix32_t)(b)))

#define MAC16_16(c,a,b) (ADD32((c),MULT16_16((a),(b))))
#define MULT16_32_Q12(a,b) ADD32(MULT16_16((a),SHR((b),12)), SHR(MULT16_16((a),((b)&0x00000fff)),12))
#define MULT16_32_Q13(a,b) ADD32(MULT16_16((a),SHR((b),13)), SHR(MULT16_16((a),((b)&0x00001fff)),13))
#define MULT16_32_Q14(a,b) ADD32(MULT16_16((a),SHR((b),14)), SHR(MULT16_16((a),((b)&0x00003fff)),14))

#define MULT16_32_Q11(a,b) ADD32(MULT16_16((a),SHR((b),11)), SHR(MULT16_16((a),((b)&0x000007ff)),11))
#define MAC16_32_Q11(c,a,b) ADD32(c,ADD32(MULT16_16((a),SHR((b),11)), SHR(MULT16_16((a),((b)&0x000007ff)),11)))

#define MULT16_32_P15(a,b) ADD32(MULT16_16((a),SHR((b),15)), PSHR(MULT16_16((a),((b)&0x00007fff)),15))
#define MULT16_32_Q15(a,b) ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15))
#define MAC16_32_Q15(c,a,b) ADD32(c,ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15)))

#define MULT32_32_Q15(a,b) ((a)*((a)>>15)+(((a)*((a)&0x00007fff))>>15))



#define MAC16_16_Q11(c,a,b)     (ADD32((c),SHR(MULT16_16((a),(b)),11)))
#define MAC16_16_Q13(c,a,b)     (ADD32((c),SHR(MULT16_16((a),(b)),13)))
#define MAC16_16_P13(c,a,b)     (ADD32((c),SHR(ADD32(4096,MULT16_16((a),(b))),13)))

#define MULT16_16_Q11_32(a,b) (SHR(MULT16_16((a),(b)),11))
#define MULT16_16_Q13(a,b) (SHR(MULT16_16((a),(b)),13))
#define MULT16_16_Q14(a,b) (SHR(MULT16_16((a),(b)),14))
#define MULT16_16_Q15(a,b) (SHR(MULT16_16((a),(b)),15))

#define MULT16_16_P13(a,b) (SHR(ADD32(4096,MULT16_16((a),(b))),13))
#define MULT16_16_P14(a,b) (SHR(ADD32(8192,MULT16_16((a),(b))),14))
#define MULT16_16_P15(a,b) (SHR(ADD32(16384,MULT16_16((a),(b))),15))

#define MUL_16_32_R15(a,bh,bl) ADD32(MULT16_16((a),(bh)), SHR(MULT16_16((a),(bl)),15))

#define DIV32_16(a,b) ((wtk_fix16_t)(((wtk_fix32_t)(a))/((wtk_fix16_t)(b))))
#define PDIV32_16(a,b) ((wtk_fix16_t)(((wtk_fix32_t)(a)+((wtk_fix16_t)(b)>>1))/((wtk_fix16_t)(b))))
#define DIV32(a,b) (((wtk_fix32_t)(a))/((wtk_fix32_t)(b)))
#define PDIV32(a,b) (((wtk_fix32_t)(a)+((wtk_fix16_t)(b)>>1))/((wtk_fix32_t)(b)))

#define MIN16(a,b) ((a) < (b) ? (a) : (b))   /**< Maximum 16-bit value.   */

#define L1 32767
#define L2 -7651
#define L3 8277
#define L4 -626

typedef struct wtk_fixfft wtk_fixfft_t;
typedef short wtk_fix16_t;
typedef int wtk_fix32_t;
typedef long long wtk_fix64_t;
typedef struct wtk_fixfft_state wtk_fixfft_state_t;

typedef struct
{
	wtk_fix16_t r;
	wtk_fix16_t i;
}wtk_fixcpx_t;

typedef struct
{
    int nfft;
    int inverse;
    int factors[64];
    wtk_fixcpx_t *twiddles;
}wtk_fixfft_substate_t;


struct wtk_fixfft_state
{
	wtk_fixfft_substate_t *substate;
	wtk_fixcpx_t *tmpbuf;
	wtk_fixcpx_t *super_twiddles;
	int nfft;
};

struct wtk_fixfft
{
	int N;
	wtk_fixfft_state_t *forward;
	wtk_fixfft_state_t *backward;
};

int wtk_fixfft_bytes(wtk_fixfft_t *f);
wtk_fixfft_t* wtk_fixfft_new(int n);
void wtk_fixfft_delete(wtk_fixfft_t *fft);
void wtk_fixfft_fft(wtk_fixfft_t *fft,short *in,short *out);
void wtk_fixfft_ifft(wtk_fixfft_t *fft,short *in,short *out);

short wtk_fix_cos(short x);
short wtk_fix_sqrt(int x);


void wtk_fixfft_print_fft(short *f,int n);


#ifdef __cplusplus
};
#endif
#endif
