#include "wtk_mer_thread.h"

int wtk_mer_get_cpu_core_num()
{
    int i;
    i = (int)sysconf(_SC_NPROCESSORS_ONLN);
    return i;
}
int wtk_mer_thread_wav_process(pthread_t *tid, int *m/* 当前线程号 */, wtk_matf_t **f0_mf2, wtk_matf_t **bap_mf2, wtk_matf_t **mgc_mf2)
{/* 多线程wav */
    int ret=-1;
    int n = m==NULL?0:*m;
    static wtk_heap_t *heap=NULL;
    static pthread_mutex_t mutex;
    static pthread_t **tid_arr;
    static wtk_matf_t **f0_mf_arr;
    static wtk_matf_t **bap_mf_arr;
    static wtk_matf_t **mgc_mf_arr;
    static int size=10;
    static int is_mutex_init=0;
    static int total;
    static int count;
    int f0_len_arr[size]
      , ap_len_arr[size]
      , sp_len_arr[size];

    if (is_mutex_init == 0) {
        if (pthread_mutex_init(&mutex, NULL) != 0){
            // 互斥锁初始化失败
            wtk_debug("初始化互斥锁失败\n");
            wtk_exit(1);
        }
        is_mutex_init = 1;
    }
    if (pthread_mutex_lock(&mutex) != 0){
        wtk_debug("加互斥锁失败\n");
        wtk_exit(1);
    }
    if (heap == NULL) {
        if (n>=10)
        {wtk_debug("线程数量超出设置范围 n:%d > size:%d \n", n, size);wtk_exit(1);}
        heap = wtk_heap_new(4096);
        tid_arr = wtk_heap_malloc(heap, sizeof(pthread_t*)*size);
        f0_mf_arr = wtk_heap_malloc(heap, sizeof(void*)*size);
        bap_mf_arr = wtk_heap_malloc(heap, sizeof(void*)*size);
        mgc_mf_arr = wtk_heap_malloc(heap, sizeof(void*)*size);
    }

    if (tid == NULL)
    {/* 处理数据 */
        if (total == 0) {ret=0;goto end;}
        pthread_t t = pthread_self();
        wtk_matf_t
            *f0_mf = *f0_mf2,
            *bap_mf = *bap_mf2,
            *mgc_mf = *mgc_mf2;
        int i;
        count++;
        for (i=0; i<total; i++)
        {
            if(*tid_arr[i] == t) {
                f0_mf_arr[i] = f0_mf;
                bap_mf_arr[i] = bap_mf;
                mgc_mf_arr[i] = mgc_mf;
            }
        }
        if (count==total) {
            /* 最后一个线程 */
            int j=0;
            wtk_matf_t *f0_dst, *ap_dst, *sp_dst;
            float *f0_dp, *ap_dp, *sp_dp;
            int f0_len=0, sp_len=0, ap_len=0;
            for (j=0; j<total; ++j)
            {
                f0_len_arr[j] = f0_mf_arr[j]->row*f0_mf_arr[j]->col;
                f0_len += f0_len_arr[j];

                ap_len_arr[j] = bap_mf_arr[j]->row*bap_mf_arr[j]->col;
                ap_len += ap_len_arr[j];

                sp_len_arr[j] = mgc_mf_arr[j]->row*mgc_mf_arr[j]->col;
                sp_len += sp_len_arr[j];
                // wtk_debug("f0_len: %d\n", f0_len);
                // wtk_debug("ap_len: %d\n", ap_len);
                // wtk_debug("sp_len: %d\n", sp_len);
            }
            f0_dst = wtk_matf_new(1, f0_len);
            f0_dp = f0_dst->p;
            ap_dst = wtk_matf_new(1, ap_len);
            ap_dp = ap_dst->p;
            sp_dst = wtk_matf_new(1, sp_len);
            sp_dp = sp_dst->p;
            for(j=0; j<total; ++j)
            {
                memcpy(f0_dp, f0_mf_arr[j]->p, sizeof(*f0_dp)*f0_len_arr[j]);
                f0_dp += f0_len_arr[j];
                memcpy(ap_dp, bap_mf_arr[j]->p, sizeof(*ap_dp)*ap_len_arr[j]);
                ap_dp += ap_len_arr[j];
                memcpy(sp_dp, mgc_mf_arr[j]->p, sizeof(*sp_dp)*sp_len_arr[j]);
                sp_dp += sp_len_arr[j];
                wtk_matf_delete(f0_mf_arr[j]);
                wtk_matf_delete(bap_mf_arr[j]);
                wtk_matf_delete(mgc_mf_arr[j]);
            }
            *f0_mf2 = f0_dst;
            *bap_mf2 = ap_dst;
            *mgc_mf2 = sp_dst;
            
            count=total=ret=0;
            wtk_heap_delete(heap);
            heap=NULL;
        }
    } else
    {/* 初始化 */
        total++;
        tid_arr[n] = tid;
    }
end:
    pthread_mutex_unlock(&mutex);
    return ret;
}

int wtk_mer_thread_act_process(pthread_t *tid, int *m/* 当前线程号 */, wtk_mer_wav_t **hash2)
{/* 多线程声音分解 */
    int ret=-1;
    int n = m==NULL?0:*m;
    static wtk_heap_t *heap=NULL;
    static pthread_mutex_t mutex;
    static pthread_t **tid_arr;
    static wtk_mer_wav_t **hash_arr;
    static wtk_matf_t **lf0_mf_arr;
    static wtk_matf_t **bap_mf_arr;
    static wtk_matf_t **mgc_mf_arr;
    static int size=10;
    static int is_mutex_init=0;
    static int total;
    static int count;
    int f0_len_arr[size]
      , ap_len_arr[size]
      , sp_len_arr[size];

    if (is_mutex_init == 0) {
        if (pthread_mutex_init(&mutex, NULL) != 0){
            // 互斥锁初始化失败
            wtk_debug("初始化互斥锁失败\n");
            wtk_exit(1);
        }
        is_mutex_init = 1;
    }
    if (pthread_mutex_lock(&mutex) != 0){
        wtk_debug("加互斥锁失败\n");
        wtk_exit(1);
    }
    if (heap == NULL) {
        if (n>=10)
        {wtk_debug("线程数量超出设置范围 %d \n", size);wtk_exit(1);}
        heap = wtk_heap_new(4096);
        tid_arr = wtk_heap_malloc(heap, sizeof(pthread_t*)*size);
        lf0_mf_arr = wtk_heap_malloc(heap, sizeof(wtk_matdf_t*)*size);
        bap_mf_arr = wtk_heap_malloc(heap, sizeof(wtk_matdf_t*)*size);
        mgc_mf_arr = wtk_heap_malloc(heap, sizeof(wtk_matdf_t*)*size);
        hash_arr = wtk_heap_malloc(heap, sizeof(wtk_mer_wav_t*));
    }

    if (tid == NULL)
    {/* 处理数据 */
        if (total == 0) {ret=0;goto end;}
        pthread_t t = pthread_self();
        wtk_mer_wav_t *hash = *hash2;
        wtk_matf_t 
            *lf0_mf = hash->lf0,
            *bap_mf = hash->bap,
            *mgc_mf = hash->mgc;
        int i;
        count++;
        for (i=0; i<total; i++)
        {
            if(*tid_arr[i] == t) {
                lf0_mf_arr[i] = lf0_mf;
                bap_mf_arr[i] = bap_mf;
                mgc_mf_arr[i] = mgc_mf;
                hash_arr[i] = hash;
            }
        }
        if (count==total) {
            /* 最后一个线程 */
            int j=0;
            wtk_heap_t *hash_heap = wtk_heap_new(4096);
            wtk_mer_wav_t *hash_dst = wtk_mer_wav_new(hash_heap);
            wtk_matf_t *f0_dst, *ap_dst, *sp_dst;
            float *f0_dp, *ap_dp, *sp_dp;
            int f0_len=0, sp_len=0, ap_len=0, f0_row=0, f0_col=0, ap_row=0, ap_col=0, sp_row=0, sp_col=0;
            for (j=0; j<total; ++j)
            {
                f0_len_arr[j] = lf0_mf_arr[j]->row*lf0_mf_arr[j]->col;
                f0_len += f0_len_arr[j];
                f0_row += lf0_mf_arr[j]->row;
                f0_col = lf0_mf_arr[j]->col;

                ap_len_arr[j] = bap_mf_arr[j]->row*bap_mf_arr[j]->col;
                ap_len += ap_len_arr[j];
                ap_row += bap_mf_arr[j]->row;
                ap_col = bap_mf_arr[j]->col;

                sp_len_arr[j] = mgc_mf_arr[j]->row*mgc_mf_arr[j]->col;
                sp_len += sp_len_arr[j];
                sp_row += mgc_mf_arr[j]->row;
                sp_col = mgc_mf_arr[j]->col;
                // wtk_debug("f0_len: %d\n", f0_len);
                // wtk_debug("ap_len: %d\n", ap_len);
                // wtk_debug("sp_len: %d\n", sp_len);
            }
            f0_dst = wtk_matf_heap_new(hash_heap, f0_row, f0_col);
            f0_dp = f0_dst->p;
            ap_dst = wtk_matf_heap_new(hash_heap, ap_row, ap_col);
            ap_dp = ap_dst->p;
            sp_dst = wtk_matf_heap_new(hash_heap, sp_row, sp_col);
            sp_dp = sp_dst->p;
            for(j=0; j<total; ++j)
            {
                memcpy(f0_dp, lf0_mf_arr[j]->p, sizeof(float)*f0_len_arr[j]);
                f0_dp += f0_len_arr[j];
                memcpy(ap_dp, bap_mf_arr[j]->p, sizeof(float)*ap_len_arr[j]);
                ap_dp += ap_len_arr[j];
                memcpy(sp_dp, mgc_mf_arr[j]->p, sizeof(float)*sp_len_arr[j]);
                sp_dp += sp_len_arr[j];
                wtk_mer_wav_delete(hash_arr[j]);
            }
            hash_dst->lf0 = f0_dst;
            hash_dst->bap = ap_dst;
            hash_dst->mgc = sp_dst;
            *hash2 = hash_dst;
            count=total=ret=0;
            wtk_heap_delete(heap);
            heap=NULL;
        }
    } else
    {/* 初始化 */
        total++;
        tid_arr[n] = tid;
    }
end:
    pthread_mutex_unlock(&mutex);
    return ret;
}
