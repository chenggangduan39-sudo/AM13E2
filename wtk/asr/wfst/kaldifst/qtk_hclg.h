#ifndef QTK_HCLG_H_
#define QTK_HCLG_H_
#include "qtk_hclg_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_hclg qtk_hclg_t;
typedef struct qtk_hclg_header qtk_hclg_header_t;

struct qtk_hclg_header
{
	  char* fsttype;     // E.g. "vector".
	  char* arctype;     // E.g. "standard".
	  unsigned int version;      // Type version number.
	  unsigned int flags;        // File format bits.
	  unsigned long properties;  // FST property bits.
	  unsigned long start;        // Start state.
	  unsigned long numstates;    // # of states.
	  unsigned long numarcs;      // # of arcs.
};

struct qtk_hclg
{
	qtk_hclg_cfg_t* cfg;
	qtk_hclg_header_t* header;
	FILE* hclg_fp;
};

int qtk_hclg_new(qtk_hclg_cfg_t* cfg);
int qtk_hclg_reset(qtk_hclg_t* hclg);
void qtk_hclg_delete(qtk_hclg_t* hclg);
int qtk_hclg_parse_header(qtk_hclg_t* hclg);

#ifdef __cplusplus
};
#endif
#endif

