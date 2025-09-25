/* 
 * File:   ais_hts_engine.h
 * Author: hlw74
 *
 * Created on December 16, 2009, 2:05 PM
 */

#ifndef _AIS_HTS_ENGINE_H
#define	_AIS_HTS_ENGINE_H
#ifdef	__cplusplus
extern "C" {
#endif
#include "cst_hts.h"

    
#define STR_LEN 1024
#define AIS_OK             0
#define AIS_WARNING 1
#define AIS_ERR_N1   -1
#define AIS_ERR_N2   -2
#define AIS_ERR_N3   -3
#define AIS_ERR_N4   -4
#define AIS_CFG_ENTRY 128    

typedef struct cst_voice_struct ais_voice;
typedef struct cst_utterance_struct ais_utterance;
    
ais_voice *init_engine();
int release_engine(ais_voice  *vcs);

int syn_speech_to_buff(ais_voice  *u, char *text, FILE *out);
hts_lab * syn_speech_to_hts_lab(ais_voice  *vcs, char *text);
char *norm_text(char *intxt);
void print_htslab2(hts_lab *hl, FILE *fp);
void set_user_lexicon(ais_voice  *vcs,char *path);
void free_user_lexicon(ais_voice *vcs);

#ifdef	__cplusplus
}
#endif

#endif	/* _AIS_HTS_ENGINE_H */

