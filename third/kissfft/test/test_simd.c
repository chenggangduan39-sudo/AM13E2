#include <qdm_kiss_fftnd.h>

static void test1(void)
{
    int is_inverse = 1;
    int n[2] = {256,256};
    size_t nbytes = sizeof(qdm_kiss_fft_cpx)*n[0]*n[1];

    qdm_kiss_fft_cpx * inbuf = _mm_malloc(nbytes,16);
    qdm_kiss_fft_cpx * outbuf = _mm_malloc(nbytes,16);
    memset(inbuf,0,nbytes);
    memset(outbuf,0,nbytes);

    qdm_kiss_fftnd_cfg cfg = qdm_kiss_fftnd_alloc(n,2,is_inverse,0,0);
    qdm_kiss_fftnd(cfg,inbuf,outbuf);
    qdm_kiss_fft_free(cfg);
    _mm_free(inbuf);
    _mm_free(outbuf);
}

int main(void)
{
    test1();
    return 0;
}
