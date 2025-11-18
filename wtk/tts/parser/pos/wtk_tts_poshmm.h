#ifndef WTK_TTS_PARSER_POS_WTK_TTS_POSHMM
#define WTK_TTS_PARSER_POS_WTK_TTS_POSHMM
#include "wtk/core/math/wtk_math.h" 
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strpool.h"
#include "wtk/core/math/wtk_sparsem.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_poshmm wtk_tts_poshmm_t;

typedef struct
{
	hash_str_node_t node;
	int idx;
}wtk_tts_hashidx_t;


struct wtk_tts_poshmm
{
	wtk_strpool_t *pool;
	int nwrd;
	int N;	//number of status(pos);Q={1,2,...,i,...,N}, 词性数
	int M;	//number of observation symbols;V={1,2,...,j,...,M}, 单词数
	wtk_matrix_t *A;
	wtk_vector_t *pi;/* pi[1..N] pi[i] is the initial state distribution. */
	//wtk_matrix_t *mat;//initialize
	wtk_sparsem_t *mat;
	wtk_str_hash_t *hash;	//word index mapping;
	int oov;
};


wtk_tts_poshmm_t* wtk_tts_poshmm_new(wtk_strpool_t *pool,int nwrd);
void wtk_tts_poshmm_delete(wtk_tts_poshmm_t *h);
int wtk_tts_poshmm_bytes(wtk_tts_poshmm_t *h);
int wtk_tts_poshmm_load(wtk_tts_poshmm_t *h,wtk_source_t *src);
int wtk_tts_poshmm_load_voc(wtk_tts_poshmm_t *h,wtk_source_t *src);

int wtk_tts_poshmm_get_idx(wtk_tts_poshmm_t *h,char *data,int bytes);


#ifdef __cplusplus
};
#endif
#endif
