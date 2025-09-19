#include "qtk/nn/vm/qtk_nn_vm.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_riff.h"
#include "qtk/serde/qtk_serde_np.h"
int treader(void *upval, char *d, int l){
    int ret;
    FILE *fp = (FILE*)upval;
    ret = fread(d,1,l,fp);
    return ret;
}

static void data_read(char *fn,float **data){

    FILE *fp;
    fp = fopen(fn,"rb");
    fread(data[0],sizeof(float),2 * 257,fp);
    fread(data[1],sizeof(float),129,fp);
    fread(data[2],sizeof(float),65,fp);
    fread(data[3],sizeof(float),33,fp);
    fread(data[4],sizeof(float),17,fp);
    fread(data[5],sizeof(float),90,fp);
    fread(data[6],sizeof(float),90,fp);
    fread(data[7],sizeof(float),90,fp);
    fclose(fp);
}
//void *qtk_serde_np_load(qtk_serde_np_t *np, qtk_io_reader r, void *upval)
int main(int argc, char **argv) {
    char *cfg_fn = NULL;
    wtk_arg_t *arg = NULL;
    qtk_serde_np_t np;
    qtk_serde_np_init(&np);
    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_debug("%s\n",cfg_fn);
    qtk_nn_vm_t nv;
    int nin = 10;
    FILE *fp = fopen(cfg_fn, "rb");
    float *input[10];
    void *val;
    qtk_nn_vm_load(&nv, treader, fp);
    qtk_nn_vm_get_input(&nv, &nin, (void**)input);

    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/x.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[0],val,sizeof(float)*39 * 80);
    qtk_serde_np_reset(&np);
    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/encoder_cache_key_0.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[1],val,sizeof(float)*32 * 96);
    qtk_serde_np_reset(&np);
    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/encoder_cache_nonlin_attn_0.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[2],val,sizeof(float)*32 * 48);
    qtk_serde_np_reset(&np);
    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/encoder_cache_val1_0.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[3],val,sizeof(float)*32 * 36);
    qtk_serde_np_reset(&np);
    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/encoder_cache_val2_0.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[4],val,sizeof(float)*32 * 36);
    qtk_serde_np_reset(&np);
    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/encoder_cache_conv1_0.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[5],val,sizeof(float)*64 * 15);
    qtk_serde_np_reset(&np);
    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/encoder_cache_conv2_0.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[6],val,sizeof(float)*64 * 15);
    qtk_serde_np_reset(&np);
    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/embed_cache.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[7],val,sizeof(float)*16 * 6 * 19);
    qtk_serde_np_reset(&np);
    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/processed_mask_cache_1.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[8],val,sizeof(uint8_t)*64 * 1);
    qtk_serde_np_reset(&np);
    fclose(fp);
    fp = fopen("/home/madch/work/qtk/qtk-kj/xb-zip/classifier_cache_0.npy", "rb");
    val = qtk_serde_np_load(&np, treader, fp);
    memcpy(input[9],val,sizeof(float)*96 * 7);
    qtk_serde_np_reset(&np);
    fclose(fp);   
    //data_read("/tmp/1", (float**)input);
    qtk_nn_vm_run(&nv);

    qtk_nn_vm_get_output(&nv, NULL, NULL);
    qtk_nn_vm_clean(&nv);
    if (arg) {
        wtk_arg_delete(arg);
    }
    return 0;
}