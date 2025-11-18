#include "wtk/bfio/ahs/estimate/qtk_gain_estimate.h"
#include "wtk/bfio/ahs/estimate/qtk_linear_conv.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"

void print_usage() {
    printf("ahs usage:\n");
    printf("\t-i input rir.txt  file\n");
    printf("\n\n");
}

static int out_len = 0;

static void gain_test_file(qtk_gain_estimate_t *g_est, char *ifn) {
    FILE *fp = fopen(ifn, "r");
    int len = 0;
    int allo_len = 40000;
    float *rir = (float *)malloc(sizeof(float) * allo_len);
    if (rir == NULL) {
        perror("Memory allocation failed");
        fclose(fp);
        return -1; // 或者其他错误处理
    }
    while (len < allo_len && fscanf(fp, "%f", rir + len) == 1) {
        ++len;
    }

    int idx = qtk_linear_conv_idx_find(rir, len);
    printf("idx:%d\n", idx);
    float min_gain = qtk_gain_estimate_rir_v2(g_est, rir, len, idx, 16);
    printf("rt60:%f\n", g_est->rt60);
    printf("min_gain:%f\n", min_gain);

    wtk_free(rir);
    fclose(fp);
}

int main(int argc,char **argv)
{
    qtk_gain_estimate_t *g_est = NULL;
    wtk_arg_t *arg;
    char *cfg_fn = NULL;
    char *bin_fn = NULL;
    char *scp = NULL;
    char *output = NULL;
    char *rir = NULL;

    arg = wtk_arg_new(argc, argv);
    if (!arg) {
        goto end;
    }
    wtk_arg_get_str_s(arg, "i", &rir);
    // if((!cfg_fn && !bin_fn)||(!scp && !rir))
    // {
    // 	print_usage();
    // 	goto end;
    // }

    g_est = qtk_gain_estimate_new();
    if (rir) {
        int i;

        for (i = 0; i < 1; ++i) {
            gain_test_file(g_est, rir);
        }
    }
end:
    if (g_est) {
        qtk_gain_estimate_delete(g_est);
    }
    if (arg) {
        wtk_arg_delete(arg);
    }
    return 0;
}
