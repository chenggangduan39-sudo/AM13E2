#include "wtk_wfstr.h"
#include "wtk_wfstr_prune.h"
#include "wtk/os/wtk_proc.h"
#include <assert.h>
#include <math.h>
//int wtk_wfstr_process_tok(wtk_wfstr_t *r,wtk_fst_tokset_t *tokset,wtk_fst_trans_t *trans);
//int wtk_wfstr_process_tok(wtk_wfstr_t *r,wtk_fst_tokset_t *tokset,wtk_fst_trans_t *to_trans,wtk_fst_inst_t *tokset_inst);
int wtk_wfstr_process_tok(wtk_wfstr_t *r,wtk_fst_tok_t *tokset,
		wtk_fst_trans_t *to_trans);
void wtk_wfstr_merge_tokset(wtk_wfstr_t *rec,wtk_fst_tok_t *res,
		wtk_fst_tok_t *cmp,wtk_fst_tok_t *src);
wtk_fst_tok_t* wtk_wfstr_new_tok(wtk_wfstr_t *r);
void wtk_fst_join_new_inst_q(wtk_wfstr_t *r);
void wtk_wfstr_print_tokset_path(wtk_wfstr_t *r,wtk_fst_tok_t *tokset);
int wtk_wfstr_check_path_label(wtk_wfst_path_t *pth);
void wtk_wfstr_merge_path(wtk_wfstr_t* rec,wtk_wfst_path_t *tok,wtk_wfst_path_t *src);
void wtk_wfstr_check_path(wtk_wfstr_t *rec,wtk_wfst_path_t *pth);
int wtk_wfstr_path_is_full_in(wtk_wfst_path_t *parent,wtk_wfst_path_t *child);
wtk_wfst_path_t* wtk_wfstr_dup_path(wtk_wfstr_t *r,wtk_wfst_path_t *pth);
wtk_wfst_path_t* wtk_wfstr_dup_path2(wtk_wfstr_t *r,wtk_wfst_path_t *pth);
void wtk_wfstr_reset_net3_end(wtk_wfstr_t *r,wtk_fst_net3_t *net);
void wtk_wfstr_collect_path(wtk_wfstr_t *r);
void wtk_wfstr_push_tokset(wtk_wfstr_t *r,wtk_fst_tok_t *tokset);

/*
void wtk_fst_tok_set_null(wtk_fst_tok_t *tok)
{
	tok->path=NULL;
	tok->like=tok->raw_like=tok->ac_like=tok->ac_lookahead_score=tok->lm_like=LZERO;
}*/

#define wtk_fst_tok_set_null(tok) \
		{(tok)->path=NULL;(tok)->align=NULL;	\
		(tok)->like=(tok)->raw_like=(tok)->ac_like=(tok)->ac_lookahead_score=(tok)->lm_like=LZERO;}

wtk_fst_tok_t fst_null_tok={
		NULL,
#ifdef USE_ALIGN
		NULL,
#endif
		LZERO,		//score
#ifdef USE_EXT_LIKE
		LZERO,		//raw_like
#endif
		LZERO,		//ac_like;
		LZERO,		//ac_lookahead_score
		LZERO,		//lm_score
};



int wtk_fst_path_depth(wtk_wfst_path_t *pth)
{
	int i=0;

	while(pth)
	{
		++i;
		pth=pth->prev;
	}
	return i;
}

void* wtk_wfstr_memcpy2(void *dst, const void *src, size_t len)
{
    if (len <= 63) {
		register unsigned char *dd = (unsigned char*)dst + len;
		register const unsigned char *ss = (const unsigned char*)src + len;
    	if(sizeof(double)==8)
    	{
			switch (len)
			{
			case 64:    *((double*)(dd - 64)) = *((double*)(ss - 64));
			case 56:    *((double*)(dd - 56)) = *((double*)(ss - 56));
			case 48:    *((double*)(dd - 48)) = *((double*)(ss - 48));
			case 40:    *((double*)(dd - 40)) = *((double*)(ss - 40));
			case 32:    *((double*)(dd - 32)) = *((double*)(ss - 32));
			case 24:    *((double*)(dd - 24)) = *((double*)(ss - 24));
			case 16:    *((double*)(dd - 16)) = *((double*)(ss - 16));
			case  8:    *((double*)(dd - 8)) = *((double*)(ss - 8));
						break;
			case 63:    *((double*)(dd - 63)) = *((double*)(ss - 63));
			case 55:    *((double*)(dd - 55)) = *((double*)(ss - 55));
			case 47:    *((double*)(dd - 47)) = *((double*)(ss - 47));
			case 39:    *((double*)(dd - 39)) = *((double*)(ss - 39));
			case 31:    *((double*)(dd - 31)) = *((double*)(ss - 31));
			case 23:    *((double*)(dd - 23)) = *((double*)(ss - 23));
			case 15:    *((double*)(dd - 15)) = *((double*)(ss - 15));
			case  7:    *((int*)(dd - 7)) = *((int*)(ss - 7));
						*((int*)(dd - 4)) = *((int*)(ss - 4));
						break;
			case 62:    *((double*)(dd - 62)) = *((double*)(ss - 62));
			case 54:    *((double*)(dd - 54)) = *((double*)(ss - 54));
			case 46:    *((double*)(dd - 46)) = *((double*)(ss - 46));
			case 38:    *((double*)(dd - 38)) = *((double*)(ss - 38));
			case 30:    *((double*)(dd - 30)) = *((double*)(ss - 30));
			case 22:    *((double*)(dd - 22)) = *((double*)(ss - 22));
			case 14:    *((double*)(dd - 14)) = *((double*)(ss - 14));
			case  6:    *((int*)(dd - 6)) = *((int*)(ss - 6));
						*((short*)(dd - 2)) = *((short*)(ss - 2));
						break;
			case 61:    *((double*)(dd - 61)) = *((double*)(ss - 61));
			case 53:    *((double*)(dd - 53)) = *((double*)(ss - 53));
			case 45:    *((double*)(dd - 45)) = *((double*)(ss - 45));
			case 37:    *((double*)(dd - 37)) = *((double*)(ss - 37));
			case 29:    *((double*)(dd - 29)) = *((double*)(ss - 29));
			case 21:    *((double*)(dd - 21)) = *((double*)(ss - 21));
			case 13:    *((double*)(dd - 13)) = *((double*)(ss - 13));
			case  5:    *((int*)(dd - 5)) = *((int*)(ss - 5));
						dd[-1] = ss[-1];
						break;
			case 60:    *((double*)(dd - 60)) = *((double*)(ss - 60));
			case 52:    *((double*)(dd - 52)) = *((double*)(ss - 52));
			case 44:    *((double*)(dd - 44)) = *((double*)(ss - 44));
			case 36:    *((double*)(dd - 36)) = *((double*)(ss - 36));
			case 28:    *((double*)(dd - 28)) = *((double*)(ss - 28));
			case 20:    *((double*)(dd - 20)) = *((double*)(ss - 20));
			case 12:    *((double*)(dd - 12)) = *((double*)(ss - 12));
			case  4:    *((int*)(dd - 4)) = *((double*)(ss - 4));
						break;
			case 59:    *((double*)(dd - 59)) = *((double*)(ss - 59));
			case 51:    *((double*)(dd - 51)) = *((double*)(ss - 51));
			case 43:    *((double*)(dd - 43)) = *((double*)(ss - 43));
			case 35:    *((double*)(dd - 35)) = *((double*)(ss - 35));
			case 27:    *((double*)(dd - 27)) = *((double*)(ss - 27));
			case 19:    *((double*)(dd - 19)) = *((double*)(ss - 19));
			case 11:    *((double*)(dd - 11)) = *((double*)(ss - 11));
			case  3:    *((short*)(dd - 3)) = *((int*)(ss - 3));
						dd[-1] = ss[-1];
						break;
			case 58:    *((double*)(dd - 58)) = *((double*)(ss - 58));
			case 50:    *((double*)(dd - 50)) = *((double*)(ss - 50));
			case 42:    *((double*)(dd - 42)) = *((double*)(ss - 42));
			case 34:    *((double*)(dd - 34)) = *((double*)(ss - 34));
			case 26:    *((double*)(dd - 26)) = *((double*)(ss - 26));
			case 18:    *((double*)(dd - 18)) = *((double*)(ss - 18));
			case 10:    *((double*)(dd - 10)) = *((double*)(ss - 10));
			case  2:    *((short*)(dd - 2)) = *((short*)(ss - 2));
						break;
			case 57:    *((double*)(dd - 57)) = *((double*)(ss - 57));
			case 49:    *((double*)(dd - 49)) = *((double*)(ss - 49));
			case 41:    *((double*)(dd - 41)) = *((double*)(ss - 41));
			case 33:    *((double*)(dd - 33)) = *((double*)(ss - 33));
			case 25:    *((double*)(dd - 25)) = *((double*)(ss - 25));
			case 17:    *((double*)(dd - 17)) = *((double*)(ss - 17));
			case  9:    *((double*)(dd - 9)) = *((double*)(ss - 9));
			case  1:    dd[-1] = ss[-1];
						break;
			case 0:
			default: break;
			}
			return dd;
    	}else
    	{
			switch (len)
			{
			case 68:    *((int*)(dd - 68)) = *((int*)(ss - 68));
			case 64:    *((int*)(dd - 64)) = *((int*)(ss - 64));
			case 60:    *((int*)(dd - 60)) = *((int*)(ss - 60));
			case 56:    *((int*)(dd - 56)) = *((int*)(ss - 56));
			case 52:    *((int*)(dd - 52)) = *((int*)(ss - 52));
			case 48:    *((int*)(dd - 48)) = *((int*)(ss - 48));
			case 44:    *((int*)(dd - 44)) = *((int*)(ss - 44));
			case 40:    *((int*)(dd - 40)) = *((int*)(ss - 40));
			case 36:    *((int*)(dd - 36)) = *((int*)(ss - 36));
			case 32:    *((int*)(dd - 32)) = *((int*)(ss - 32));
			case 28:    *((int*)(dd - 28)) = *((int*)(ss - 28));
			case 24:    *((int*)(dd - 24)) = *((int*)(ss - 24));
			case 20:    *((int*)(dd - 20)) = *((int*)(ss - 20));
			case 16:    *((int*)(dd - 16)) = *((int*)(ss - 16));
			case 12:    *((int*)(dd - 12)) = *((int*)(ss - 12));
			case  8:    *((int*)(dd - 8)) = *((int*)(ss - 8));
			case  4:    *((int*)(dd - 4)) = *((int*)(ss - 4));
						break;
			case 67:    *((int*)(dd - 67)) = *((int*)(ss - 67));
			case 63:    *((int*)(dd - 63)) = *((int*)(ss - 63));
			case 59:    *((int*)(dd - 59)) = *((int*)(ss - 59));
			case 55:    *((int*)(dd - 55)) = *((int*)(ss - 55));
			case 51:    *((int*)(dd - 51)) = *((int*)(ss - 51));
			case 47:    *((int*)(dd - 47)) = *((int*)(ss - 47));
			case 43:    *((int*)(dd - 43)) = *((int*)(ss - 43));
			case 39:    *((int*)(dd - 39)) = *((int*)(ss - 39));
			case 35:    *((int*)(dd - 35)) = *((int*)(ss - 35));
			case 31:    *((int*)(dd - 31)) = *((int*)(ss - 31));
			case 27:    *((int*)(dd - 27)) = *((int*)(ss - 27));
			case 23:    *((int*)(dd - 23)) = *((int*)(ss - 23));
			case 19:    *((int*)(dd - 19)) = *((int*)(ss - 19));
			case 15:    *((int*)(dd - 15)) = *((int*)(ss - 15));
			case 11:    *((int*)(dd - 11)) = *((int*)(ss - 11));
			case  7:    *((int*)(dd - 7)) = *((int*)(ss - 7));
						*((int*)(dd - 4)) = *((int*)(ss - 4));
						break;
			case  3:    *((short*)(dd - 3)) = *((short*)(ss - 3));
						dd[-1] = ss[-1];
						break;
			case 66:    *((int*)(dd - 66)) = *((int*)(ss - 66));
			case 62:    *((int*)(dd - 62)) = *((int*)(ss - 62));
			case 58:    *((int*)(dd - 58)) = *((int*)(ss - 58));
			case 54:    *((int*)(dd - 54)) = *((int*)(ss - 54));
			case 50:    *((int*)(dd - 50)) = *((int*)(ss - 50));
			case 46:    *((int*)(dd - 46)) = *((int*)(ss - 46));
			case 42:    *((int*)(dd - 42)) = *((int*)(ss - 42));
			case 38:    *((int*)(dd - 38)) = *((int*)(ss - 38));
			case 34:    *((int*)(dd - 34)) = *((int*)(ss - 34));
			case 30:    *((int*)(dd - 30)) = *((int*)(ss - 30));
			case 26:    *((int*)(dd - 26)) = *((int*)(ss - 26));
			case 22:    *((int*)(dd - 22)) = *((int*)(ss - 22));
			case 18:    *((int*)(dd - 18)) = *((int*)(ss - 18));
			case 14:    *((int*)(dd - 14)) = *((int*)(ss - 14));
			case 10:    *((int*)(dd - 10)) = *((int*)(ss - 10));
			case  6:    *((int*)(dd - 6)) = *((int*)(ss - 6));
			case  2:    *((short*)(dd - 2)) = *((short*)(ss - 2));
						break;
			case 65:    *((int*)(dd - 65)) = *((int*)(ss - 65));
			case 61:    *((int*)(dd - 61)) = *((int*)(ss - 61));
			case 57:    *((int*)(dd - 57)) = *((int*)(ss - 57));
			case 53:    *((int*)(dd - 53)) = *((int*)(ss - 53));
			case 49:    *((int*)(dd - 49)) = *((int*)(ss - 49));
			case 45:    *((int*)(dd - 45)) = *((int*)(ss - 45));
			case 41:    *((int*)(dd - 41)) = *((int*)(ss - 41));
			case 37:    *((int*)(dd - 37)) = *((int*)(ss - 37));
			case 33:    *((int*)(dd - 33)) = *((int*)(ss - 33));
			case 29:    *((int*)(dd - 29)) = *((int*)(ss - 29));
			case 25:    *((int*)(dd - 25)) = *((int*)(ss - 25));
			case 21:    *((int*)(dd - 21)) = *((int*)(ss - 21));
			case 17:    *((int*)(dd - 17)) = *((int*)(ss - 17));
			case 13:    *((int*)(dd - 13)) = *((int*)(ss - 13));
			case  9:    *((int*)(dd - 9)) = *((int*)(ss - 9));
			case  5:    *((int*)(dd - 5)) = *((int*)(ss - 5));
			case  1:    dd[-1] = ss[-1];
						break;
			case 0:
			default: break;
			}
			return dd;
    	}
    }   else {
        return memcpy(dst, src, len);
    }
}

void* wtk_wfstr_memcpy(void *dst, const void *src, size_t len)
{
    if (len <= 63) {
        register unsigned char *dd = (unsigned char*)dst + len;
        register const unsigned char *ss = (const unsigned char*)src + len;
        switch (len)
        {
        case 68:    *((int*)(dd - 68)) = *((int*)(ss - 68));
        case 64:    *((int*)(dd - 64)) = *((int*)(ss - 64));
        case 60:    *((int*)(dd - 60)) = *((int*)(ss - 60));
        case 56:    *((int*)(dd - 56)) = *((int*)(ss - 56));
        case 52:    *((int*)(dd - 52)) = *((int*)(ss - 52));
        case 48:    *((int*)(dd - 48)) = *((int*)(ss - 48));
        case 44:    *((int*)(dd - 44)) = *((int*)(ss - 44));
        case 40:    *((int*)(dd - 40)) = *((int*)(ss - 40));
        case 36:    *((int*)(dd - 36)) = *((int*)(ss - 36));
        case 32:    *((int*)(dd - 32)) = *((int*)(ss - 32));
        case 28:    *((int*)(dd - 28)) = *((int*)(ss - 28));
        case 24:    *((int*)(dd - 24)) = *((int*)(ss - 24));
        case 20:    *((int*)(dd - 20)) = *((int*)(ss - 20));
        case 16:    *((int*)(dd - 16)) = *((int*)(ss - 16));
        case 12:    *((int*)(dd - 12)) = *((int*)(ss - 12));
        case  8:    *((int*)(dd - 8)) = *((int*)(ss - 8));
        case  4:    *((int*)(dd - 4)) = *((int*)(ss - 4));
                    break;
        case 67:    *((int*)(dd - 67)) = *((int*)(ss - 67));
        case 63:    *((int*)(dd - 63)) = *((int*)(ss - 63));
        case 59:    *((int*)(dd - 59)) = *((int*)(ss - 59));
        case 55:    *((int*)(dd - 55)) = *((int*)(ss - 55));
        case 51:    *((int*)(dd - 51)) = *((int*)(ss - 51));
        case 47:    *((int*)(dd - 47)) = *((int*)(ss - 47));
        case 43:    *((int*)(dd - 43)) = *((int*)(ss - 43));
        case 39:    *((int*)(dd - 39)) = *((int*)(ss - 39));
        case 35:    *((int*)(dd - 35)) = *((int*)(ss - 35));
        case 31:    *((int*)(dd - 31)) = *((int*)(ss - 31));
        case 27:    *((int*)(dd - 27)) = *((int*)(ss - 27));
        case 23:    *((int*)(dd - 23)) = *((int*)(ss - 23));
        case 19:    *((int*)(dd - 19)) = *((int*)(ss - 19));
        case 15:    *((int*)(dd - 15)) = *((int*)(ss - 15));
        case 11:    *((int*)(dd - 11)) = *((int*)(ss - 11));
        case  7:    *((int*)(dd - 7)) = *((int*)(ss - 7));
                    *((int*)(dd - 4)) = *((int*)(ss - 4));
                    break;
        case  3:    *((short*)(dd - 3)) = *((short*)(ss - 3));
                    dd[-1] = ss[-1];
                    break;
        case 66:    *((int*)(dd - 66)) = *((int*)(ss - 66));
        case 62:    *((int*)(dd - 62)) = *((int*)(ss - 62));
        case 58:    *((int*)(dd - 58)) = *((int*)(ss - 58));
        case 54:    *((int*)(dd - 54)) = *((int*)(ss - 54));
        case 50:    *((int*)(dd - 50)) = *((int*)(ss - 50));
        case 46:    *((int*)(dd - 46)) = *((int*)(ss - 46));
        case 42:    *((int*)(dd - 42)) = *((int*)(ss - 42));
        case 38:    *((int*)(dd - 38)) = *((int*)(ss - 38));
        case 34:    *((int*)(dd - 34)) = *((int*)(ss - 34));
        case 30:    *((int*)(dd - 30)) = *((int*)(ss - 30));
        case 26:    *((int*)(dd - 26)) = *((int*)(ss - 26));
        case 22:    *((int*)(dd - 22)) = *((int*)(ss - 22));
        case 18:    *((int*)(dd - 18)) = *((int*)(ss - 18));
        case 14:    *((int*)(dd - 14)) = *((int*)(ss - 14));
        case 10:    *((int*)(dd - 10)) = *((int*)(ss - 10));
        case  6:    *((int*)(dd - 6)) = *((int*)(ss - 6));
        case  2:    *((short*)(dd - 2)) = *((short*)(ss - 2));
                    break;
        case 65:    *((int*)(dd - 65)) = *((int*)(ss - 65));
        case 61:    *((int*)(dd - 61)) = *((int*)(ss - 61));
        case 57:    *((int*)(dd - 57)) = *((int*)(ss - 57));
        case 53:    *((int*)(dd - 53)) = *((int*)(ss - 53));
        case 49:    *((int*)(dd - 49)) = *((int*)(ss - 49));
        case 45:    *((int*)(dd - 45)) = *((int*)(ss - 45));
        case 41:    *((int*)(dd - 41)) = *((int*)(ss - 41));
        case 37:    *((int*)(dd - 37)) = *((int*)(ss - 37));
        case 33:    *((int*)(dd - 33)) = *((int*)(ss - 33));
        case 29:    *((int*)(dd - 29)) = *((int*)(ss - 29));
        case 25:    *((int*)(dd - 25)) = *((int*)(ss - 25));
        case 21:    *((int*)(dd - 21)) = *((int*)(ss - 21));
        case 17:    *((int*)(dd - 17)) = *((int*)(ss - 17));
        case 13:    *((int*)(dd - 13)) = *((int*)(ss - 13));
        case  9:    *((int*)(dd - 9)) = *((int*)(ss - 9));
        case  5:    *((int*)(dd - 5)) = *((int*)(ss - 5));
        case  1:    dd[-1] = ss[-1];
                    break;
        case 0:
        default: break;
        }
        return dd;
    }   else {
        return memcpy(dst, src, len);
    }
}


/*
*/
/**
 *   The types LogFloat and LogDouble are used for representing
 *real numbers on a log scale.  LZERO is used for log(0) in log
 *arithmetic, any log real value <= LSMALL is considered to be
 *zero.
 * Return sum x + y on log scale, sum < LSMALL is floored to LZERO;
 */
double wtk_wfstr_log_add(wtk_wfstr_t *rec, double x, double y)
{
	double temp, diff, z;

	//log(sum(e^x,e^y))
	//log(e^x+e^y)=x+log(1+e^(y-x))
	if (x < y)
	{
		temp = x;
		x = y;
		y = temp;
	}
	diff = y - x;
	if (diff < rec->cfg->min_log_exp)
	{
		return (x < LSMALL) ? LZERO : x;
	}
	else
	{
		z = exp(diff);
		return x + log(1.0 + z);
	}
}


float wtk_wfstr_calc_dia_prob(wtk_wfstr_t *rec,wtk_mixpdf_t *pdf,wtk_fixi_t *fix)
{
	float prob;
	register int v;
	register int *pi,*pe;
	register int *pmean;
	register int *pvar;
	register int mean;

	v=0;
	pi=fix->p-1;
	pe=pi+fix->col;
	pmean=(int*)(pdf->mean);
	pvar=(int*)(pdf->variance);
	while((pi+4)<=pe)
	{
		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));

		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));

		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));

		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));
	}
	while(pi<pe)
	{
		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));
	}
	prob=pdf->fGconst+v/rec->fix_scale;
	//prob=pdf->fGconst+v*rec->fix_scale;//v/rec->fix_scale;//(fix->scale*rec->hmmset->cfg->var_scale*fix->scale);
	//wtk_debug("prob=%f v=%d scale=%f/%f\n",prob,v,fix->scale,rec->hmmset->cfg->var_scale);
	//wtk_debug("prob=%f gconst=%f v=%d\n",prob,pdf->fGconst,v);
//	if(prob>=0)
//	{
//		wtk_debug("prob=%f gconst=%f v=%d\n",prob,pdf->fGconst,v);
//		exit(0);
//	}
	return prob;
}

float wtk_wfstr_calc_mix_prob(wtk_wfstr_t *rec, wtk_vector_t *obs,
		wtk_mixpdf_t *mix)
{
	wtk_precomp_t *pre;

	if (mix->index <= rec->hmmset->num_phy_mix && mix->index > 0)
	{
		pre = rec->mPre + mix->index - 1;
		if (pre->id != rec->frame)
		{
			pre->id = rec->frame;
			if(rec->hmmset->cfg->use_fix)
			{
				pre->outp=wtk_wfstr_calc_dia_prob(rec,mix,rec->hlda.fix);
			}else
			{
				pre->outp=wtk_mixpdf_calc_dia_prob(mix, obs);
			}
			//wtk_debug("outp=%f\n",pre->outp);
			//exit(0);
		}
		return pre->outp;
	}
	else
	{
		if(rec->hmmset->cfg->use_fix)
		{
			return wtk_wfstr_calc_dia_prob(rec,mix,rec->hlda.fix);
		}else
		{
			return wtk_mixpdf_calc_dia_prob(mix, obs);
		}
	}
}

void wtk_wfstr_set_dnn_handler(wtk_wfstr_t *rec,void *ths,wtk_wfstr_dnn_get_value_f f)
{
	rec->dnn_ths=ths;
	rec->dnn_get=f;
}

float wtk_wfstr_calc_prob(wtk_wfstr_t *rec, wtk_vector_t *obs, wtk_state_t* state)
{
	wtk_dnn_state_t *s=state->dnn;
	wtk_precomp_t *pre;
	wtk_stream_t *stream;
	wtk_mixture_t *mix;
	int streams, i, m;
	float outp = 0;
	float mixprob, streamprob;

	if(!rec->sPre)
	{
		return obs[s->index]+s->gconst;
	}
	//wtk_debug("%d,%d\n",rec->nsp,state->index);
	if (state->index > 0)// && state->index <= rec->nsp)
	{
		pre = rec->sPre + state->index - 1;
	}
	else
	{
		pre = 0;
	}
	if (!pre || pre->id != rec->cache_frame)
	{
		if(rec->cfg->use_dnn)
		{
			if(rec->dnn_get)
			{
				//wtk_debug("id=%d frame=%d\n",pre->id,rec->frame);
				outp=rec->dnn_get(rec->dnn_ths,rec->f,s->index)+s->gconst;
				//wtk_debug("outp=%f,gconf=%f\n",outp,s->gconst);
			}else
			{
				outp=obs[s->index]+s->gconst;
				//wtk_debug("frame=%d %f/%f/%f\n",rec->frame,obs[s->index],s->gconst,outp);
			}
			//wtk_debug("[%d]=%f\n",s->index,outp);
		}else
		{
			streams = rec->hmmset->stream_width[0];
			for (i = 1, stream = state->pStream; i <= streams; ++i, ++stream)
			{
				streamprob = LZERO;
				for (m = 1, mix = stream->pmixture; m <= stream->nMixture;
						++m, ++mix)
				{
					//weight=log(mix->fWeight);
					//weight = mix->fWeight;
					mixprob = wtk_wfstr_calc_mix_prob(rec, obs, mix->pdf);
					//wtk_debug("mixpro=%f\n",mixprob);
					//log(c*p)=log(c)+log(p)=weight+mixprob
					streamprob = wtk_wfstr_log_add(rec, streamprob, mix->fWeight + mixprob);
				}
				if (streams == 1)
				{
					outp = streamprob;
				}
				else
				{
					outp += state->pfStreamWeight[i] * streamprob;
				}
			}
			//wtk_debug("outp=%f\n",outp);
			//exit(0);
			//wtk_debug("%.*s: %f\n",state->name->len,state->name->data,outp);
		}
		if (pre)
		{
			pre->id = rec->cache_frame;
			pre->outp = outp;
		}
	}
	else
	{
		outp = pre->outp;
	}
	//printf("%d:%f\n",++j,outp);//exit(0);
	return outp;
}


void wtk_wfstr_init_hmm(wtk_wfstr_t *rec)
{
	wtk_hmmset_t *hl = rec->hmmset;

	rec->nsp = hl->num_states;
	rec->nmp=hl->num_phy_mix;

	if(rec->cfg->use_spre)
	{
		rec->sPre = (wtk_precomp_t*) wtk_calloc(rec->nsp,sizeof(wtk_precomp_t));
		rec->mPre =
			(wtk_precomp_t*) wtk_calloc(rec->nmp,sizeof(wtk_precomp_t));
	}else
	{
		rec->sPre=NULL;
		rec->mPre=NULL;
	}
}


wtk_wfstr_t* wtk_wfstr_new(wtk_wfstrec_cfg_t *cfg,wtk_fst_net_cfg_t *net_cfg,wtk_hmmset_t *hmm)
{
	wtk_wfstr_t *r;
	int max_hmm_state;
	int i;

	r=(wtk_wfstr_t*)wtk_malloc(sizeof(wtk_wfstr_t));
	r->cfg=cfg;
	r->net_cfg=net_cfg;
	r->hmmset=hmm;
	r->net=0;
	r->ebnf_net=NULL;
	r->lat_rescore=NULL;
	r->dnn_get=NULL;
	r->dnn_ths=NULL;
	r->sil_id=net_cfg->sym_in->sil_id;
	/*
	if(cfg->use_fixprune)
	{
		r->fixprune=wtk_fixprune_new(&(cfg->fixprune));
	}else
	{
		r->fixprune=NULL;
	}*/
	if(cfg->use_prune)
	{
		//r->histogram=wtk_histogram_new(&(cfg->histogram));
		r->prune=wtk_prune_new(&(cfg->prune));
	}else
	{
		r->prune=NULL;
	}
	if(cfg->use_phn_prune)
	{
		r->phn_prune=wtk_prune_new(&(cfg->phn_prune));
	}else
	{
		r->phn_prune=NULL;
	}
	if(cfg->use_wrd_prune)
	{
		r->wrd_prune=wtk_prune_new(&(cfg->wrd_prune));
	}else
	{
		r->wrd_prune=NULL;
	}
	if(cfg->use_expand_prune)
	{
		r->expand_prune=wtk_prune_new(&(cfg->expand_prune));
	}else
	{
		r->expand_prune=NULL;
	}
	r->buf=wtk_strbuf_new(256,1);
	if(cfg->inst_use_heap)
	{
		r->inst_pool=NULL;
	}else
	{
		r->inst_pool=wtk_vpool2_new(sizeof(wtk_fst_inst_t),cfg->inst_cache);
	}
	max_hmm_state=hmm->max_hmm_state;
	//r->tokset_pool=wtk_vpool_new(sizeof(wtk_fst_tokset_t)*max_hmm_state,cfg->tok_cache);
	if(cfg->use_lat)
	{
		//r->use_ntok=0;
		r->lat_net=wtk_fst_net3_new(&(cfg->lat_net),net_cfg);
		//r->inst_pool=wtk_vpool_new(sizeof(wtk_fst_inst_t)+sizeof(wtk_fst_lat_inst_t),cfg->inst_cache);
	}else
	{
		r->lat_net=0;
		//r->use_ntok=cfg->ntok>1;
	}

	r->tokset_pool=wtk_vpool2_new(sizeof(wtk_fst_tok_t)*max_hmm_state,cfg->tok_cache);
	r->pth_pool=wtk_vpool2_new(sizeof(wtk_wfst_path_t),cfg->pth_cache);
#ifdef USE_ALIGN
    if(cfg->state) {
        r->align_pool = wtk_vpool_new(sizeof(wtk_wfst_align_t), cfg->align_cache);
    } else {
        r->align_pool = NULL;
    }
#endif

    r->tokset_buf_bytes = sizeof(wtk_fst_tok_t) * (max_hmm_state - 1);
    r->step_inst1_tokset = NULL;
    r->null_tokset_buf_bytes = sizeof(wtk_fst_tok_t) * max_hmm_state;
    r->null_tokset_buf =
        (wtk_fst_tok_t *)wtk_calloc(max_hmm_state, sizeof(wtk_fst_tok_t));
    for (i = 0; i < max_hmm_state; ++i) {
        r->null_tokset_buf[i] = fst_null_tok;
    }
        r->heap=wtk_heap_new(4096);
	if(r->hmmset->cfg && r->hmmset->cfg->use_fix)
	{
		//r->fix_scale=1.0/(r->hmmset->cfg->mean_scale*r->hmmset->cfg->mean_scale*r->hmmset->cfg->var_scale);
		r->fix_scale=(r->hmmset->cfg->mean_scale*r->hmmset->cfg->mean_scale*r->hmmset->cfg->var_scale);
	}
	if(r->cfg->hlda_fn)
	{
		if(r->hmmset->cfg &&r->hmmset->cfg->use_fix)
		{
			r->hlda.fix=wtk_fixi_new(1,wtk_matrix_rows(r->cfg->hlda_matrix));
		}else
		{
			r->hlda.vector=wtk_vector_new(wtk_matrix_rows(r->cfg->hlda_matrix));
		}
	}else
	{
		if(r->hmmset->cfg && r->hmmset->cfg->use_fix)
		{
			r->hlda.fix=wtk_fixi_new(1,r->hmmset->vec_size);
		}else
		{
			r->hlda.vector=NULL;
		}
	}
//	r->notify=NULL;
//	r->notify_ths=NULL;
	wtk_wfstr_init_hmm(r);
	wtk_wfstr_reset(r);
	return r;
}

void wtk_wfstr_delete(wtk_wfstr_t *r) {
        if (r->cfg->hlda_fn) {
                if(r->hmmset->cfg && r->hmmset->cfg->use_fix)
		{
			if(r->hlda.fix)
			{
				wtk_fixi_delete(r->hlda.fix);
			}
		}else
		{
			wtk_vector_delete(r->hlda.vector);
		}
        } else {
                if(r->hmmset->cfg && r->hmmset->cfg->use_fix && r->hlda.fix)
		{
			wtk_fixi_delete(r->hlda.fix);
		}
        }
        wtk_strbuf_delete(r->buf);
	if(r->prune)
	{
		wtk_prune_delete(r->prune);
	}
	if(r->phn_prune)
	{
		wtk_prune_delete(r->phn_prune);
	}
	if(r->wrd_prune)
	{
		wtk_prune_delete(r->wrd_prune);
	}
	if(r->expand_prune)
	{
		wtk_prune_delete(r->expand_prune);
	}
	if (r->mPre)
	{
		wtk_free(r->mPre);
	}
	if (r->sPre)
	{
		wtk_free(r->sPre);
        }
        // wtk_free(r->nul_tok_buf);

        wtk_free(r->null_tokset_buf);

	if(r->inst_pool)
	{
		wtk_vpool2_delete(r->inst_pool);
	}
	wtk_vpool2_delete(r->tokset_pool);
	wtk_vpool2_delete(r->pth_pool);
#ifdef USE_ALIGN
    if(r->align_pool) {
        wtk_vpool_delete(r->align_pool);
    }
#endif
	if(r->cfg->use_lat)
	{
		wtk_fst_net3_delete(r->lat_net);
	}
	wtk_heap_delete(r->heap);
	wtk_free(r);
}

void wtk_fst_tok_init(wtk_fst_tok_t *tok)
{
	tok->like=0.0;
#ifdef USE_EXT_LIKE
	tok->raw_like=0.0;
#endif
	tok->ac_like=0;
	tok->ac_lookahead_score=0.0;
	tok->lm_like=0.0;
	//tok->ac_score=0.0;
	tok->path=0;
#ifdef USE_ALIGN
	tok->align=0;
#endif

#ifdef	X_DNN_CONF
	tok->score_avg = 0.0f;
	tok->score_in_state = 0.0f;
	tok->frames_in_state = 0;
	tok->n_states = 0;
#endif
}

void wtk_wfstr_reset(wtk_wfstr_t *r)
{
	wtk_fst_tok_t *tok;

	r->inst1_valid_phns=0;
	r->inst1_valid_wrds=0;
	r->max_pth=NULL;
	r->last_pth=NULL;
	r->end_hint_frame=0;
	r->nphn=0;
	r->nwrd=0;
	r->nsil=0;
	r->end_hint=0;
	r->emit_beam=r->cfg->emit_beam;
	r->cache_frame=0;
	r->lat_prune_time=0;
	if(r->cfg->use_lat)
	{
		wtk_fst_net3_reset(r->lat_net);
		r->use_rescore=1;
	}else
	{
		r->use_rescore=0;
	}
	wtk_queue_init(&(r->inst_q));
	wtk_queue_init(&(r->new_inst_q));
#ifdef  COLLECT_PATH
	wtk_queue_init(&(r->path_no_q));
	wtk_queue_init(&(r->path_yes_q));
#endif
	if(r->prune)
	{
		wtk_prune_reset(r->prune);
	}
	if(r->phn_prune)
	{
		wtk_prune_reset(r->phn_prune);
	}
	if(r->wrd_prune)
	{
		wtk_prune_reset(r->wrd_prune);
	}
	if(r->expand_prune)
	{
		r->best_expand_score=LZERO;
		wtk_prune_reset(r->expand_prune);
	}
	if(r->inst_pool)
	{
		wtk_vpool2_reset(r->inst_pool);
	}
	if(r->step_inst1_tokset)
	{
		wtk_wfstr_push_tokset(r,r->step_inst1_tokset+1);
		r->step_inst1_tokset=NULL;
	}
	wtk_vpool2_reset(r->tokset_pool);
	r->step_inst1_tokset=(wtk_fst_tok_t*)wtk_vpool2_pop(r->tokset_pool);
	memset(r->step_inst1_tokset,0,r->tokset_pool->bytes);
	--r->step_inst1_tokset;
	wtk_vpool2_reset(r->pth_pool);
#ifdef USE_ALIGN
    if(r->align_pool) {
        wtk_vpool_reset(r->align_pool);
    }
#endif
	//r->inst_max_use=0;
	wtk_heap_reset(r->heap);

	tok=&(r->init_tokset);
	wtk_fst_tok_init(tok);

	r->best_final_tokset=fst_null_tok;
	r->best_ebnf_final_tokset=fst_null_tok;

	r->gen_max_tok=fst_null_tok;
	r->exit_max_tok=fst_null_tok;
	r->gen_max_trans=NULL;
	r->is_forceout=0;

	r->best_exit_score=LZERO;
	r->best_emit_score=LZERO;

	r->best_start_score=LZERO;
	r->best_end_score=LZERO;
	//LSMALL
#ifdef USE_LZERO
	r->cur_end_thresh=LZERO;
	r->cur_wrd_thresh=LZERO;
	r->cur_start_thresh=LZERO;
	r->cur_emit_thresh=LZERO;
	r->cur_ntok_thresh=LZERO;
#else
	r->cur_end_thresh=LSMALL;
	r->cur_wrd_thresh=LSMALL;
	r->cur_start_thresh=LSMALL;
	r->cur_emit_thresh=LSMALL;
	//r->cur_ntok_thresh=LSMALL;
	r->cur_lat_thresh=LSMALL;
#endif
	r->normalize_score=0.0;
	r->frame=0;

	if(r->cfg->use_spre)
	{
		memset(r->sPre, 0, r->nsp*sizeof(wtk_precomp_t));
		memset(r->mPre, 0, r->nmp*sizeof(wtk_precomp_t));
	}
}

int wtk_wfstr_start(wtk_wfstr_t *r,wtk_fst_net_t *net)
{
	return wtk_wfstr_start2(r,net,NULL);
}

int wtk_wfstr_start2(wtk_wfstr_t *r,wtk_fst_net_t *comm_net,wtk_fst_net_t *ebnf_net)
{
	//if(!r->net_cfg)
	{
		r->net_cfg=comm_net->cfg;
	}
	r->conf=0;
	r->net=comm_net;
	if(r->cfg->use_lat)
	{
		r->lat_net->net_cfg=comm_net->cfg;
	}
	r->ebnf_net=ebnf_net;
	//wtk_debug("%f/%f %f/%f\n",comm_net->cfg->lmscale,ebnf_net->cfg->lmscale,r->cfg->ac_scale,r->cfg->custom_ac_scale);
	wtk_wfstr_process_tok(r,&(r->init_tokset),0);
	wtk_fst_join_new_inst_q(r);
	return 0;
}



void wtk_fst_join_new_inst_q(wtk_wfstr_t *r)
{
	if(r->new_inst_q.length<=0){return;}
	//wtk_debug("================> link new_inst=%d inst=%d\n",r->new_inst_q.length,r->inst_q.length);
	wtk_queue_link(&(r->new_inst_q),&(r->inst_q));
	//wtk_fst_reorder_inst(r,s,e);
	r->inst_q=r->new_inst_q;
	wtk_queue_init(&(r->new_inst_q));
}

wtk_fst_tok_t* wtk_wfstr_pop_tokset(wtk_wfstr_t *r)
{
	wtk_fst_tok_t *tokset;

	tokset=(wtk_fst_tok_t*)wtk_vpool2_pop(r->tokset_pool);
	memcpy(tokset,r->null_tokset_buf,r->null_tokset_buf_bytes);
	//wtk_debug("pop %p\n",tokset);
	return tokset;
}

void wtk_wfstr_push_tokset(wtk_wfstr_t *r,wtk_fst_tok_t *tokset)
{
	//wtk_debug("push %p\n",tokset);
	wtk_vpool2_push(r->tokset_pool,tokset);
}

wtk_fst_tok_t* wtk_wfstr_pop_step1_bak_tokset(wtk_wfstr_t *r)
{
	wtk_fst_tok_t *t;

	//wtk_debug("pop %p\n",r->step_inst1_tokset);
	if(r->step_inst1_tokset)
	{
		t=r->step_inst1_tokset;
		r->step_inst1_tokset=NULL;
		return t;
	}
	t=(wtk_fst_tok_t*)wtk_vpool2_pop(r->tokset_pool);
	t-=1;
	return t;
}

void wtk_wfstr_push_step1_bak_tokset(wtk_wfstr_t *r,wtk_fst_tok_t *t)
{
	//wtk_debug("push %p/%p\n",t,r->step_inst1_tokset);
	if(r->step_inst1_tokset)
	{
		wtk_vpool2_push(r->tokset_pool,t+1);
	}else
	{
		r->step_inst1_tokset=t;
	}
}


#ifdef USE_SORT_INST
void wtk_wfstr_move_to_recent(wtk_wfstr_t *rec,wtk_fst_inst_t *inst)
{
	//wtk_debug("move to recent inst=%p\n",inst);
	if(inst==rec->cur_inst)
	{
		rec->cur_inst=wtk_queue_node_data(inst->inst_n.next,wtk_fst_inst_t,inst_n);
	}
	wtk_queue_touch_node(&(rec->inst_q),&(inst->inst_n));
	inst->ooo=1;
}

void wtk_wfstr_reorder_inst(wtk_wfstr_t *rec,wtk_fst_trans_t *trans)
{
	wtk_fst_inst_t *inst;
	wtk_fst_trans_t *t;
	wtk_hmm_t *hmm;
	wtk_fst_state_t *tostate;

	inst=trans->inst;
	if(!inst || !inst->ooo){return;}
	inst->ooo=0;
	tostate=trans->to_state;
	for(t=tostate->v.trans;t;t=t->next)
	{
		inst=t->inst;
		if(!inst){continue;}
		hmm=inst->hmm;
		if(hmm->transP[hmm->num_state][1]>LZERO)
		{
			wtk_wfstr_move_to_recent(rec,inst);
		}
	}
	for(t=tostate->v.trans;t;t=t->next)
	{
		inst=t->inst;
		if(!inst){continue;}
		//hmm=inst->hmm;
		//if(hmm->transP[hmm->num_state][1]>LZERO && inst->ooo)
		if(inst->ooo)
		{
			wtk_wfstr_reorder_inst(rec,inst->trans);
		}
	}
}
#endif


wtk_fst_inst_t* wtk_wfstr_pop_inst(wtk_wfstr_t *rec,wtk_fst_trans_t *trans)
{
	wtk_fst_inst_t *inst;
	wtk_hmm_t *hmm;
	int idx_hmm;

//	if(rec->frame==31)
//	{
//		wtk_debug("trans[%d]=%p %d:%d\n",rec->frame,trans,trans->in_id,trans->out_id);
//	}
	//wtk_debug("trans[%d]=%p %d:%d\n",rec->frame,trans,trans->in_id,trans->out_id);
	if(trans->to_state->type==WTK_FST_TOUCH_STATE && (rec->net_cfg->use_rbin|| !rec->net->v.bin))
	{
		//wtk_debug("found bug %d %d\n",trans->to_state->ntrans,trans->to_state->id);
		return NULL;
	}
	idx_hmm=trans->in_id-1;
	if(idx_hmm>rec->hmmset->hmm_array->nslot)
	{
		//wtk_debug("id=%d/%d not found\n",idx_hmm,rec->hmmset->hmm_array->nslot);
		return NULL;
	}
	hmm=((wtk_hmm_t**)rec->hmmset->hmm_array->slot)[idx_hmm];
	if(!hmm->transP)
	{
		wtk_debug("id=%d/%d not found\n",idx_hmm,rec->hmmset->hmm_array->nslot);
		exit(0);
		return NULL;
	}
	//wtk_debug("inst=%p v[%d]=%f,%f\n",inst,prev_thresh->indx,prev_thresh->like,prev_thresh->thresh);
	//exit(0);
	if(rec->inst_pool)
	{
		inst=(wtk_fst_inst_t*)wtk_vpool2_pop(rec->inst_pool);
	}else
	{
		inst=(wtk_fst_inst_t*)wtk_heap_malloc(rec->heap,sizeof(wtk_fst_inst_t));
	}
#ifdef USE_BIT_MAP
	inst->bit_map=0;
#endif
	inst->hmm=hmm;
	//wtk_debug("%d=%p\n",idx_hmm,inst->hmm);
	//exit(0);
	/*
	if(!inst->hmm)
	{
		wtk_debug("hmm=%p idx=%d id=%d n=%d\n",inst->hmm,idx_hmm,trans->in_id
				,rec->hmmset->hmm_array->nslot);
		exit(0);
	}*/
	//wtk_debug("v[%d]=%.*s\n",trans->in_id,inst->hmm->name->len,inst->hmm->name->data);
	inst->states=wtk_wfstr_pop_tokset(rec);
	inst->states-=1;
	inst->n_active_hyps=0;
	inst->trans=trans;
	//inst->exit_like=LZERO;
	trans->hook.inst=inst;
	//wtk_wfstr_check_depth_thresh(rec,inst);
	//wtk_debug("v[%d/%d]: inst=%p/%p idx=%d\n",rec->frame,i,inst,inst->depth_wrd_thresh,inst->depth_wrd_thresh->index);
	if(trans->to_state->type==WTK_FST_TOUCH_STATE)
	{
		wtk_fst_net_load_state(rec->net,trans->to_state);
	}
#ifdef USE_SORT_INST
	if(rec->cfg->use_sort)
	{
		inst->ooo=1;
		wtk_queue_push(&(rec->inst_q),&(inst->inst_n));
		wtk_wfstr_reorder_inst(rec,trans);
	}else
#endif
	{
		wtk_queue_push_front(&(rec->new_inst_q),&(inst->inst_n));
	}
	//wtk_queue_push(&(rec->new_inst_q),&(inst->inst_n));
	//reorder for TEE
	//wtk_debug("new inst=%p\n",inst);
	return inst;
}

void wtk_wfstr_push_inst(wtk_wfstr_t *rec,wtk_fst_inst_t *inst)
{
	wtk_wfstr_push_tokset(rec,inst->states+1);
	if(rec->inst_pool)
	{
		wtk_vpool2_push(rec->inst_pool,inst);
	}
}


void wtk_wfstr_reset_net3_end(wtk_wfstr_t *r,wtk_fst_net3_t *net)
{
	wtk_fst_node_t *n=net->end;
	wtk_fst_lat_inst_t *inst;
	wtk_queue_node_t *qn;
	wtk_fst_node_t *n1;

	if(!n->state)
	{
		return;
	}
	inst=(wtk_fst_lat_inst_t*)n->state->hook;
	if(!inst)
	{
		return;
	}
	while(1)
	{
		qn=wtk_queue2_pop(&(inst->list_q));
		if(!qn){break;}
		n1=data_offset2(qn,wtk_fst_node_t,inst_n);
		wtk_fst_net_remove_node_from_prev(net,n1);
		wtk_fst_net3_push_node(net,n1);
	}
}


int wtk_wfst_path_has_out_id(wtk_wfst_path_t *pth,unsigned int out_id,int frame)
{
	if(pth->trans->out_id==out_id && pth->frame==frame)
	{
		return 1;
	}
	if(pth->prev)
	{
		return wtk_wfst_path_has_out_id(pth->prev,out_id,frame);
	}
	return 0;
}

int wtk_wfst_path_has_out_id2(wtk_wfst_path_t *pth,unsigned int out_id)
{
	if(pth->trans->out_id==out_id)
	{
		return 1;
	}
	if(pth->prev)
	{
		return wtk_wfst_path_has_out_id2(pth->prev,out_id);
	}
	return 0;
}

int wtk_wfst_path_last_wrd(wtk_wfst_path_t *pth)
{
	while(pth)
	{
		if(pth->trans->out_id>0)
		{
			return pth->trans->out_id;
		}
		pth=pth->prev;
	}
	return 0;
}

int wtk_fst_path_prev_wrd_id(wtk_wfst_path_t *pth)
{
	if(pth->trans->out_id!=0)
	{
		return pth->trans->out_id;
	}
	if(pth->prev)
	{
		return wtk_fst_path_prev_wrd_id(pth->prev);
	}
	return -1;
}

int wtk_wfstr_attach_state(wtk_wfstr_t *r,wtk_fst_tok_t *tokset,
		wtk_fst_trans_t *to_trans,wtk_fst_state_t *state)
{
	wtk_fst_lat_inst_t *from_inst;
	wtk_fst_node_t *node,*from_node;
	wtk_fst_net3_t *lat_net=r->lat_net;
	wtk_fst_arc_t *arc;
	//float like;
	float ac_like,lm_like;
	float arc_like;
	wtk_wfst_path_t *prev=NULL;

	if(tokset->path && tokset->path->hook)
	{
		return 0;
	}
	if(!to_trans)
	{
		state->hook=from_inst=wtk_fst_net3_pop_lat_inst(lat_net);
		node=wtk_fst_net3_pop_node(lat_net,r->frame,0,state);
		//arc=wtk_fst_net3_pop_arc(lat_net,0,node,NULL,0.0,0.0,0.0,r->frame);
		lat_net->start=node;
		return 0;
	}
	if(r->cfg->lat_beam>0 && tokset->like<r->cur_lat_thresh && r->frame>0)
	{
		return 0;
	}
	//wtk_debug("attach %d/%d\n",to_trans->in_id,to_trans->out_id);
	if(tokset->path->prev)
	{
		if(!tokset->path->prev->hook)
		{
			/*
			if(!r->cfg->use_lat_prev_wrd)
			{
				return 0;
			}*/
#ifdef USE_MY_T
			from_node=wtk_fst_get_path_net_node(r,tokset->tok.path->prev,&prev,1);
			/*
			if(r->frame>514 && wtk_fst_path_has_out_id(tokset->tok.path,30165,416))
			{
				wtk_wfstr_print_path(r,tokset->tok.path);
				wtk_fst_net3_print_prev_arc(r->lat_net,from_node,100);
				wtk_debug("from_node=%p\n",from_node);
				exit(0);
			}*/
			if(!from_node)
			{
				//30165/416
				return;
			}
#else
			wtk_queue_node_t *qn;
			wtk_wfst_path_t *tp;

			from_node=NULL;
			tp=tokset->path->prev;//->prev;
			//wtk_debug("out=%d:%d\n",tokset->tok.path->prev->trans->in_id,tokset->tok.path->prev->trans->out_id);
			//wtk_debug("tp=%p\n",tp);
			while(tp && !tp->hook)
			{
				if(tp->trans->out_id>0)
				{
					from_inst=wtk_fst_net3_get_trans_inst(lat_net,tp->trans);
					if(from_inst && from_inst->list_q.pop)
					{
						for(qn=from_inst->list_q.pop;qn;qn=qn->next)
						{
							node=data_offset2(qn,wtk_fst_node_t,inst_n);
							if(node->frame==tp->frame)
							{
								from_node=node;
								break;
							}else if(node->frame<tp->frame)
							{
								break;
							}
						}
						if(from_node)
						{
							prev=tp;
							break;
						}else
						{
							return 0;
						}
					}else
					{
						return 0;
					}
				}
				tp=tp->prev;
			}
			if(!from_node)
			{
				if(tp && tp->hook)
				{
					prev=tp;
					//arc=(wtk_fst_arc_t*)(tp->hook);
					//from_node=arc->to_node;
					//from_node=(wtk_fst_node_t*)(tp->hook);
					from_node=wtk_fst_path_get_lat_node(tp);
				}else
				{
					return 0;
				}
			}
#endif
		}else
		{
			//arc=(wtk_fst_arc_t*)(tokset->tok.path->prev->hook);
			//from_node=arc->to_node;
			//from_node=(wtk_fst_node_t*)(tokset->tok.path->prev->hook);
			from_node=wtk_fst_path_get_lat_node(tokset->path->prev);
			prev=tokset->path->prev;
		}
	}else
	{
		prev=tokset->path->prev;
		from_node=lat_net->start;
	}
	//wtk_debug("v[%d/%d]: f=%d, n=%p\n",r->frame,ix,from_node->frame,from_node);
	node=wtk_fst_net3_get_trans_node(lat_net,to_trans,r->frame);
	//like=tokset->tok.raw_like;
	if(tokset->path && prev)// && tokset->tok.path->prev)
	{
		ac_like=tokset->ac_like-prev->ac_like;
		lm_like=tokset->lm_like-prev->lm_like;
#ifdef USE_EXT_LIKE
		arc_like=tokset->raw_like-prev->raw_like;
#else
		arc_like=ac_like+lm_like;
#endif
	}else
	{
		ac_like=tokset->ac_like;
		lm_like=tokset->lm_like;
#ifdef USE_EXT_LIKE
		arc_like=tokset->raw_like;
#else
		arc_like=ac_like+lm_like;
#endif
	}
	arc=wtk_fst_net3_pop_arc(lat_net,from_node,node,to_trans,arc_like,ac_like,lm_like,r->frame);
	wtk_fst_arc_link_path(arc,tokset->path);
	if(state->type==WTK_FST_FINAL_STATE)
	{
		//wtk_debug("attach state v[%d]=%p\n",r->frame,node);
		if(lat_net->end)// && r->lat_net->end!=node)
		{
			wtk_wfstr_reset_net3_end(r,lat_net);
		}
		lat_net->end=node;
		lat_net->end_state=node->state;
	}
	return 0;
}


wtk_wfst_path_t* wtk_wfstr_add_path(wtk_wfstr_t *r,wtk_fst_tok_t *tokset,
		wtk_fst_trans_t *trans)
{
	wtk_wfst_path_t *pth;


	pth=(wtk_wfst_path_t*)wtk_vpool2_pop(r->pth_pool);
#ifdef USE_ALIGN
	pth->usage=0;
#endif
#ifdef  COLLECT_PATH
	wtk_queue_push_front(&(r->path_no_q),&(pth->q_n));
	pth->queue=&(r->path_no_q);
#endif
	//pth->ref_count=0;
	pth->hook=NULL;
	pth->trans=trans;
	pth->frame=r->frame;
	//pth->nxt_chain=0;
	//pth->label=to_trans->out_id;
	//pth->like=tokset->tok.like;
	pth->ac_like=tokset->ac_like;
	pth->lm_like=tokset->lm_like;
#ifdef USE_EXT_LIKE
	pth->raw_like=tokset->raw_like;
#endif
	//pth->like=tokset->like;
	pth->prev=tokset->path;

	tokset->path=pth;
	if(r->cfg->use_end_hint)
	{
		if(trans->out_id>0)
		{
			++r->nwrd;
		}else
		{
			++r->nphn;
			if(trans->in_id==r->sil_id)
			{
				++r->nsil;
			}
		}
	}
#ifdef USE_ALIGN
    if(r->align_pool)
    {
        pth->align = tokset->align;
        tokset->align = 0;
    }
#endif
//	if(trans->out_id==43372)
//	{
//		wtk_wfstr_print_path(r,pth);
//	}
	//wtk_wfstr_print_path(r,pth);
	return pth;
}

int wtk_fst_path_last_wrd_dur(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	int frame=pth->frame;
	wtk_wfst_path_t *prev;
	int snt_end_id=r->net_cfg->snt_end_id;

	prev=pth->prev;
	while(prev && (prev->trans->out_id!=snt_end_id))// (prev->trans->out_id==0 || prev->trans->out_id==snt_end_id))
	{
		prev=prev->prev;
	}
//	if(prev)
//	{
//		prev=prev->prev;
//	}
	if(prev)
	{
		//wtk_debug("frame=%d prev=%d %d\n",frame,prev->frame,frame-prev->frame);
		return frame-prev->frame;
	}else
	{
		return frame;
	}
}

int wtk_wfstr_process_tok_next(wtk_wfstr_t *r,wtk_fst_tok_t *tokset,
		wtk_fst_trans_t *to_trans,wtk_fst_state_t *state)
{
	wtk_wfstrec_cfg_t *cfg=r->cfg;
	wtk_fst_net_t *net;//=r->net;
	int eps_id;//=net->cfg->eps_id;
	wtk_fst_tok_t *dup_tok,*tok;
	wtk_fst_inst_t *inst;
	wtk_fst_tok_t *ttokset;
	wtk_fst_tok_t dup_tokset;
	wtk_fst_trans_t *trans;
	wtk_hmm_t *hmm;
	//wtk_fst_path_t *pth;
	double score,ts;
	int i;
	float pure_lm;

	if(state->custom)
	{
		net=r->ebnf_net?r->ebnf_net:r->net;
	}else
	{
		net=r->net;
	}
	eps_id=net->cfg->eps_id;
	dup_tok=&(dup_tokset);
	tok=(tokset);
	if(!state->custom && r->use_rescore)
	{
		if(r->cfg->use_lat)
		{
			wtk_wfstr_attach_state(r,tokset,to_trans,state);
		}
	}
	//wtk_debug("step state=%p\n",state);
	//wtk_debug("trans=%d\n",state->ntrans);
#ifndef USE_ARRAY
	for(i=0,trans=state->v.trans;i<state->ntrans;++i,++trans)
	{
		//wtk_debug("[%d/%d]\n",trans->in_id,trans->out_id);
#else
	for(trans=state->v.trans;trans;trans=trans->next)
	{
#endif
#ifdef	ATTACH_NWLP
		score=tok->like+trans->weight+trans->to_state->nwlmp;
#else
		score=(double)tok->like+trans->weight;//+trans->to_state->nwlmp;
		//wtk_debug("v[%d]=%f\n",r->frame,score);
		//wtk_debug("%f/%f id=%d/%d\n",tok->like,trans->weight,trans->in_id,trans->out_id);
//		if(state->custom)
//		{
//			static int ki=0;
//			++ki;
//			wtk_debug("v[%d/%d]=%f/%f/%f trans=%d/%d\n",state->custom,r->frame,trans->weight,score,tok->like,trans->in_id,trans->out_id);
//			if(state->custom)
//			{
//				//exit(0);
//			}
//		}
#endif
		//wtk_debug("v[%d]=%d/%d %d=>%d\n",r->frame,trans->in_id,trans->out_id,state->id,trans->to_state->id);
		if(trans->in_id==eps_id)
		{
			//wtk_debug("v[%d]=%f/%f\n",r->frame,score,r->cur_end_thresh);
			if(score>r->cur_end_thresh)
			{
				dup_tokset=*tokset;
				dup_tok->like=score;
#ifdef USE_EXT_LIKE
				dup_tok->raw_like+=trans->weight;
#endif
				if(trans->out_id>0)
				{
					pure_lm=trans->weight-net->cfg->wordpen;
				}else
				{
					pure_lm=trans->weight;
				}
				dup_tok->lm_like+=pure_lm;
				wtk_wfstr_process_tok(r,&dup_tokset,trans);//,entry_tokset);
			}
		}else
		{
			if(cfg->use_in_wrd_beam &&  trans->out_id>0 && score<r->cur_end_thresh)
			{
				continue;
			}
			//wtk_fst_net_print_trans(net,trans);
			//wtk_debug("inst=%p %d:%d\n",trans->hook.inst,trans->in_id,trans->out_id);
			if(!trans->hook.inst)
			{
				inst=wtk_wfstr_pop_inst(r,trans);
				if(r->expand_prune && score>r->best_expand_score)
				{
					r->best_expand_score=score;
				}
			}else
			{
				inst=(wtk_fst_inst_t*)trans->hook.inst;
			}
			if(!inst)
			{
				//never be here;
				continue;
			}
			ttokset=inst->states+1;
			if(r->cfg->add_wrd_sep && trans->out_id>0)
			{
				wtk_wfstr_add_path(r,tokset,NULL);
			}
			if(trans->out_id>0)
			{
				pure_lm=trans->weight-net->cfg->wordpen;
			}else
			{
				pure_lm=trans->weight;
			}
			if(score>ttokset->like)
			{
				if(ttokset->like<=LZERO)
				{
					//wtk_debug("bit_map=%d\n",inst->bit_map);
#ifdef	USE_BIT_MAP
					inst->bit_map+=2;
#endif
					//wtk_wfstr_check_bit_map(inst);
					++inst->n_active_hyps;
				}
				//wtk_fst_tokset_cpy(ttokset,tokset); for ntok;
				*ttokset=*tokset;
				ttokset->like=score;
#ifdef USE_EXT_LIKE
				ttokset->raw_like+=trans->weight;
#endif
				ttokset->lm_like+=pure_lm;
				if(score>r->best_emit_score)
				{
					r->best_emit_score=score;
					//wtk_debug("best_emit=%f\n",r->best_emit_score);
				}
				if(!r->cfg->use_single_best && score>r->best_start_score)
				{
					r->best_start_score=score;
				}
//				if(b && 0)
//				{
//					//ttokset->tok.raw_like+=xlike*r->rescore.rel->cfg->lm_inc_scale;
//					//ttokset->tok.raw_like=xlike*r->rescore.rel->cfg->lm_inc_scale+trans->weight;
////					wtk_debug("xlike=%f raw=%f ac=%f lm=%f\n",xlike,ttokset->tok.raw_like,ttokset->tok.ac_like,
////							ttokset->tok.lm_like);
//					wtk_debug("ac=%f lm=%f raw=%f\n",ttokset->tok.ac_like,ttokset->tok.lm_like,ttokset->tok.raw_like);
//					ttokset->tok.raw_like=xlike*r->rescore.rel->cfg->lm_inc_scale+ttokset->tok.raw_like;//+ttokset->tok.lm_like;
//				}
			}
			//wtk_wfstr_check_depth_thresh(r,inst);
			//if(0)
			{
				hmm=inst->hmm;
				//hmm=wtk_wfstr_get_inst_hmm(r,inst);
				ts=hmm->transP[hmm->num_state][1];
				if(ts>LZERO)
				{
					score+=ts;
					if((trans->out_id!=eps_id && score>r->cur_wrd_thresh)
					||(trans->out_id==eps_id && score>r->cur_end_thresh))
					{
						dup_tokset=*ttokset;
						dup_tok->like=score;
#ifdef USE_EXT_LIKE
						dup_tok->raw_like+=trans->weight+ts;
#endif
						dup_tok->ac_like+=ts;
						dup_tok->lm_like+=pure_lm;
						wtk_wfstr_process_tok(r,&dup_tokset,trans);//inst->states+r->hmmset->max_hmm_state-1);
					}
				}
			}
		}
	}
	return 0;
}

int wtk_wfstr_process_tok(wtk_wfstr_t *r,wtk_fst_tok_t *tokset,
		wtk_fst_trans_t *to_trans)
{
	wtk_fst_tok_t *best;
	wtk_fst_net_t *net;
	int eps_id;
	wtk_fst_state_t *state;
	//wtk_fst_path_t *pth;
	double weight;
	int use_rescore;

	if(to_trans)
	{
		//wtk_debug("in=%d/%d\n",to_trans->in_id,to_trans->out_id);
		//wtk_debug("to_trans %d/%d out=%d in=%d\n",to_trans->to_state->id,to_trans->to_state->ntrans,to_trans->out_id,
		//		r->net_cfg->pstate_in[to_trans->to_state->id]);
		state=to_trans->to_state;
		if(state->custom)//r->ebnf_net && state->custom)
		{
			net=r->ebnf_net?r->ebnf_net:r->net;
			use_rescore=0;
		}else
		{
			net=r->net;
			use_rescore=r->use_rescore;
		}
		//wtk_debug("ebnf=%p net=%p use_rescore=%d\n",r->ebnf_net,r->net,use_rescore);
		//exit(0);
		eps_id=net->cfg->eps_id;
		if(state->type==WTK_FST_TOUCH_STATE)
		{
			//wtk_debug("load %d\n",state->id);
//			if(r->net_cfg->use_rbin)
//			{
//				//wtk_debug("found bug\n");
//				return 0;
//			}
			wtk_fst_net_load_state(r->net,state);
		}
		// 1. Add a new path to |tok| if at word boundary
		//wtk_debug("use_eps_pth=%d lat=%d\n",r->cfg->use_eps_pth,r->use_lat);
		if((use_rescore) || (to_trans->out_id  != eps_id) || r->cfg->use_eps_pth)
		{
			wtk_wfstr_add_path(r,tokset,to_trans);
		}else if(r->cfg->model || r->cfg->state)
		{
			if(to_trans->out_id!=eps_id || to_trans->in_id!=eps_id)
			{
				wtk_wfstr_add_path(r,tokset,to_trans);
			}
		}
		// 2. Update |bestFinalToken| if trans reaches the final state
		if(state->type==WTK_FST_FINAL_STATE)
		{
			//wtk_debug("v[%d] reach end\n",r->frame);
			if(r->ebnf_net && state->custom)
			{
				best=&(r->best_ebnf_final_tokset);
			}else
			{
				best=&(r->best_final_tokset);
			}
			//best=&(r->best_final_tokset);
			weight=state->v.weight;
			if(tokset->like+weight>best->like)
			{
				//wtk_debug("v[%d] is final r=%p pth=%p\n",r->frame,r,tokset->tok.path);
				//wtk_wfstr_print_path(r,tokset->tok.path);
				if(use_rescore)
				{
					if(r->cfg->use_lat)
					{
						wtk_wfstr_attach_state(r,tokset,to_trans,state);
					}
				}
				*best=*tokset;
				best->like+=weight;
#ifdef USE_EXT_LIKE
				best->raw_like+=weight;
#endif
				best->lm_like+=weight;
			}
			return 0;
		}
		//wtk_debug("in=%d out=%d to=%d\n",to_trans->in_id,to_trans->out_id,to_trans->to_state->id);
		wtk_wfstr_process_tok_next(r,tokset,to_trans,state);
	}else
	{
		state=wtk_fst_net_get_start_state(r->net);
		if(state->type==WTK_FST_TOUCH_STATE)
		{
			wtk_fst_net_load_state(r->net,state);
		}
		wtk_wfstr_process_tok_next(r,tokset,to_trans,state);
		if(r->ebnf_net)
		{
			state=wtk_fst_net_get_start_state(r->ebnf_net);
			if(state->type==WTK_FST_TOUCH_STATE)
			{
				wtk_fst_net_load_state(r->ebnf_net,state);
			}
			wtk_wfstr_process_tok_next(r,tokset,to_trans,state);
		}
	}
	return 0;
}

wtk_queue_node_t* wtk_wfstr_return_inst(wtk_wfstr_t *r,wtk_queue_t *q,
		wtk_queue_node_t *n)
{
	wtk_queue_node_t *nxt;
	wtk_fst_inst_t *inst;
	//wtk_fst_state_t *state;

#ifdef USE_SORT_INST
	nxt=r->cur_inst->inst_n.next;
#else
	nxt=n->next;
#endif
	wtk_queue_remove(q,n);
	inst=data_offset2(n,wtk_fst_inst_t,inst_n);
	inst->trans->hook.inst=NULL;
	wtk_wfstr_push_inst(r,inst);
	return nxt;
}

wtk_wfst_path_t* wtk_wfstr_recent_pron_path(wtk_wfstr_t *rec,wtk_wfst_path_t *path)
{
	while(path && path->trans->out_id==rec->net->cfg->eps_id)
	{
		path=path->prev;
	}
	return path;
}


typedef float wtk_wfstr_float_t;

int wtk_wfstr_check_bit_map(wtk_fst_inst_t *inst)
{
	wtk_fst_tok_t *cur;
	int j;
	int bit_map=0;

	for(cur=inst->states+1,j=1;j<=inst->hmm->num_state;++j,++cur)
	{
		if(cur->like>LSMALL)
		{
			//wtk_debug("j=%d\n",j);
			bit_map+=1<<j;
		}
	}
#ifdef	USE_BIT_MAP
	if(bit_map!=inst->bit_map)
	{
		wtk_debug("found has=%#x/ use=%#x\n",bit_map,inst->bit_map);
		exit(0);
	}
#endif
	return bit_map;
}

#ifdef	X_DNN_CONF
void wtk_wfstr_update_dnn(wtk_wfstr_t *r,wtk_fst_tok_t *res,wtk_fst_tok_t *dest,wtk_fst_tok_t *tgt,wtk_hmm_t *hmm,int j,int i,float prob)
{
	wtk_dnn_state_t* s;
	wtk_state_t *sh;
	float f;

	sh = (wtk_state_t*)hmm->pState[j];
	/* skip sil */
	if(hmm->sil)//0 == wtk_string_cmp_s(hmm->name, "sil"))
	{
		dest->score_avg = tgt->score_avg;
		dest->score_in_state = 0.0f;
		dest->frames_in_state = r->frame;
		dest->n_states = tgt->n_states;
	}else
	{
		s = sh->dnn;
		f=prob-s->gconst;
		/* not self loop */
		if(i!=j)//dest != tgt)
		{
			if(r->frame != tgt->frames_in_state)
			{
				dest->score_avg += (tgt->score_in_state + f) / (r->frame - tgt->frames_in_state);
				++dest->n_states;
				//wtk_debug("plus=%d\n",dest->n_states);
			}
			dest->score_in_state = 0.0f;
			dest->frames_in_state = r->frame;
		}
		else
		{
			dest->score_avg = tgt->score_avg;
			dest->score_in_state = tgt->score_in_state +f;// r->obs[s->index];
			dest->frames_in_state = tgt->frames_in_state;
			dest->n_states = tgt->n_states;
		}
	}

	// state confidence, tok propagate to next state
	res->score_avg = dest->score_avg;
	res->score_in_state = dest->score_in_state;
	res->frames_in_state = dest->frames_in_state;
	res->n_states = dest->n_states;

}
#endif

#ifdef USE_ALIGN
wtk_wfst_align_t* wtk_wfstr_new_align(wtk_wfstr_t *rec, int state,
                                       double like, int frame, wtk_fst_trans_t *trans, wtk_wfst_align_t *prev)
{
    wtk_wfst_align_t *align;

    //align = (wtk_fst_align_t*) wtk_vpool_pop(rec->align_pool);
    align = (wtk_wfst_align_t*)wtk_heap_malloc(rec->heap, sizeof(wtk_wfst_align_t));
    align->state = state;
    align->like = like;
    align->frame = frame;
    align->trans = trans;
    align->prev = prev;
    return align;
}
#endif

wtk_wfstr_float_t  wtk_wfstr_step_hmm1_emit_state2_x(wtk_wfstr_t *r,wtk_fst_inst_t *inst)
{
	wtk_hmm_t *hmm;
	register wtk_fst_tok_t *res,*cur;
	wtk_fst_tok_t *bk_tok,*tgt=NULL;
	wtk_matrix_t *trans;
	short **pp,*p;
	int i,endi,j,n,x;
	float *pf;
	//double score,f;
	float score,f;
	wtk_wfstr_float_t ac_lookahead_alpha=r->cfg->ac_lookahead_alpha;
	wtk_wfstr_float_t max_score,lookahead_score,transf;
	int nactive=0;
	int use_hist=r->cfg->use_prune;

#ifdef	USE_BIT_MAP
	int bit_map=0;

	bit_map=inst->bit_map;
	//wtk_wfstr_check_bit_map(inst);
	if(bit_map==0)
	{
		return LZERO;
	}
	inst->bit_map=0;
#endif
	hmm=inst->hmm;
	n=hmm->num_state;
	pp=r->hmmset->seIndexes[hmm->tIdx]+2;
	bk_tok=r->step_inst1_tokset;
	trans=hmm->transP;
	max_score=LZERO;
	for(j=2,res=bk_tok+2;j<n;++j,++res,++pp)
	{
		p=*pp;
		i=p[0];endi=p[1];
#ifdef	USE_BIT_MAP
		x=((bit_map>>i)<<(32-endi));
		if(x==0)
		{
			*res=fst_null_tok;
			continue;
		}
#endif
		cur=inst->states+i;
		pf=trans[j];
		if(cur->like>LSMALL)
		{
			score=cur->like+pf[i];
		}else
		{
			score=LZERO;
		}
		tgt=cur;
		x=i;
		//wtk_debug("score=%f/%f/%f\n",cur->tok.like,pf[i],score);
		//wtk_debug("v[%d] score=%f/%f norm=%f thresh=%f\n",j,cur->tok.like,pf[i],r->normalize_score,r->cur_emit_thresh)
		for(++cur,++i;i<=endi;++i,++cur)
		{
			//wtk_debug("%f/%f\n",*(xf),pf[i]);
			//wtk_debug("v[%d] score=%f/%f \n",j,cur->tok.like,pf[i]);
#ifdef	USE_BIT_MAP
			if((bit_map&(1<<i)) && (f=cur->like+ pf[i])>score)
#else
			if(((f=cur->tok.like+ pf[i])>score))
#endif
			{
				tgt=cur;
				score=f;
				//wtk_debug("%f/%f=%f\n",cur->tok.like,pf[i],f);
			}
		}
		//exit(0);
		if(score<LSMALL)
		{
			*res=fst_null_tok;
			continue;
		}
		//wtk_debug("check in\n");
		if(use_hist)
		{
			score-=r->normalize_score;
		}
		if(score>r->gen_max_tok.like)
		{
			r->gen_max_tok=*res;
			r->gen_max_tok.like=score;
			r->gen_max_trans=inst->trans;
		}
		//wtk_debug("score=%f/%f\n",score,r->cur_emit_thresh);
		if(score>r->cur_emit_thresh)
		{
			transf=pf[tgt-inst->states];
			//wtk_debug("transf=%f\n",transf);
#ifdef USE_ALIGN
			if(r->align_pool)
			{
				res->align=tgt->align;
				if(!res->align || res->align->state != j || res->align->trans != inst->trans)
				{
					res->align = wtk_wfstr_new_align(r, j,tgt->ac_like+transf, r->frame - 1, inst->trans, res->align);
				}
			}
#endif
			f=wtk_wfstr_calc_prob(r,r->obs,hmm->pState[j]);
			//wtk_debug("f=%f\n",f);
			score+=f;
			transf+=f;
			if(ac_lookahead_alpha>0)
			{
				lookahead_score=ac_lookahead_alpha*(f-tgt->ac_lookahead_score);
				score+=lookahead_score;
				res->ac_lookahead_score=f;
			}else
			{
				lookahead_score=0;
			}
			if(r->prune)
			{
				//wtk_debug("score=%f\n",score);
				wtk_prune_add(r->prune,score);
				//wtk_histogram_add_score(r->histogram,score,LZERO);
			}
			if(score>r->best_emit_score)
			{
				r->best_emit_score=score;
			}
			if(score>max_score)
			{
				max_score=score;
			}
			res->like=score;
#ifdef USE_EXT_LIKE
			res->raw_like=tgt->raw_like+transf+lookahead_score;
#endif
			res->ac_like=tgt->ac_like+transf;
			res->lm_like=tgt->lm_like;
			res->path=tgt->path;
			++nactive;
#ifdef	USE_BIT_MAP
			inst->bit_map+=1<<j;
#endif
		}else
		{
			*res=fst_null_tok;
		}
	}
	r->step_inst1_tokset=inst->states;
	inst->states=bk_tok;
	inst->states[1]=fst_null_tok;
	inst->n_active_hyps=nactive;
	return max_score;
}


wtk_wfstr_float_t  wtk_wfstr_step_hmm1_emit_state2(wtk_wfstr_t *r,wtk_fst_inst_t *inst)
{
	wtk_hmm_t *hmm;
	register wtk_fst_tok_t *res,*cur;
	wtk_fst_tok_t *bk_tok,*tgt=NULL;
	wtk_matrix_t *trans;
	short **pp,*p;
	int i,endi,j,n,x;
	float *pf;
	//double score,f;
	float score,f;
	wtk_wfstr_float_t ac_lookahead_alpha=r->cfg->ac_lookahead_alpha;
	wtk_wfstr_float_t max_score,lookahead_score,transf;
	int nactive=0;
	int use_hist=r->cfg->use_prune;
	float ac_scale=r->cfg->ac_scale;
	//float trans_scale=r->cfg->trans_scale;
	int custom=inst->trans->to_state->custom;
	wtk_dnn_state_t *s;

	//wtk_debug("train=%d/%d\n",inst->trans->in_id,inst->trans->out_id);
#ifdef	USE_BIT_MAP
	int bit_map=0;

	bit_map=inst->bit_map;
	//wtk_wfstr_check_bit_map(inst);
//	if(bit_map==0)
//	{
//		return LZERO;
//	}
	inst->bit_map=0;
#endif
	hmm=inst->hmm;
	n=hmm->num_state;
	pp=r->hmmset->seIndexes[hmm->tIdx]+2;
	bk_tok=r->step_inst1_tokset;
	trans=hmm->transP;
	max_score=LZERO;
	for(j=2,res=bk_tok+2;j<n;++j,++res,++pp)
	{
		p=*pp;
		i=p[0];endi=p[1];
#ifdef	USE_BIT_MAP
		x=((bit_map>>i)<<(32-endi));
		if(x==0)
		{
			*res=fst_null_tok;
			continue;
		}
#endif
		cur=inst->states+i;
		pf=trans[j];
		//wtk_debug("like=%f/%f\n",cur->like,pf[i]);
		if(cur->like>LSMALL)
		{
			score=cur->like+pf[i];
			if(score<r->cur_emit_raw_thresh)
			{
				score=r->cur_emit_raw_thresh;
			}
		}else
		{
			//score=LZERO;
			score=r->cur_emit_raw_thresh;
		}
		//wtk_debug("score=%f thresh=%f\n",score,r->cur_emit_raw_thresh);
		tgt=cur;
		x=i;
		//wtk_debug("score=%f/%f/%f\n",cur->tok.like,pf[i],score);
		//wtk_debug("v[%d] score=%f/%f norm=%f thresh=%f\n",j,cur->tok.like,pf[i],r->normalize_score,r->cur_emit_thresh)
		for(++cur,++i;i<=endi;++i,++cur)
		{
			//wtk_debug("%f/%f\n",*(xf),pf[i]);
			//wtk_debug("v[%d] score=%f/%f \n",j,cur->tok.like,pf[i]);
#ifdef	USE_BIT_MAP
			if((bit_map&(1<<i)) && (f=cur->like+ pf[i])>score)
#else
			if(((f=cur->tok.like+ pf[i])>score))
#endif
			{
				tgt=cur;
				score=f;
				//wtk_debug("%f/%f=%f\n",cur->tok.like,pf[i],f);
			}
		}
//		if(inst->trans->to_state->custom)
//		{
//			wtk_debug("found frame=%d n=%d in=%d %f/%f nactive=%d ac=%f/%f in=%d/%d\n",r->frame,inst->n_active_hyps,inst->trans->in_id,
//					score,r->cur_emit_raw_thresh,nactive,r->cfg->ac_scale,r->cfg->custom_ac_scale,inst->trans->in_id,inst->trans->out_id);
//		}
		//exit(0);
		//wtk_debug("score=%f thresh=%f\n",score,r->cur_emit_raw_thresh);
		if(score<=r->cur_emit_raw_thresh)
		{
			*res=fst_null_tok;
			continue;
		}
		//wtk_debug("check in\n");
		if(use_hist)
		{
			score-=r->normalize_score;
		}
		if(score>r->gen_max_tok.like)
		{
			r->gen_max_tok=*res;
			r->gen_max_tok.like=score;
			r->gen_max_trans=inst->trans;
			//wtk_debug("score[%d]=%f/%f\n",r->frame,score,r->gen_max_tok.like);
		}
		//wtk_debug("score=%f/%f\n",score,r->cur_emit_thresh);
		//if(score>r->cur_emit_thresh)
		{
			transf=pf[tgt-inst->states];
			//wtk_debug("transf=%f\n",transf);
#ifdef USE_ALIGN
			if(r->align_pool)
			{
				res->align=tgt->align;
				if(!res->align || res->align->state != j || res->align->trans != inst->trans)
				{
					res->align = wtk_wfstr_new_align(r, j,tgt->ac_like+transf, r->frame - 1, inst->trans, res->align);
				}
			}
#endif
			if(r->cfg->use_dnn)
			{
				s=hmm->pState[j]->dnn;
				if(r->dnn_get)
				{
					//wtk_debug("id=%d frame=%d\n",pre->id,rec->frame);
					f=r->dnn_get(r->dnn_ths,r->f,s->index)+s->gconst;
					//wtk_debug("outp=%f,gconf=%f\n",outp,s->gconst);
				}else
				{
					f=r->obs[s->index]+s->gconst;
					//wtk_debug("frame=%d %f/%f/%f\n",rec->frame,obs[s->index],s->gconst,outp);
				}
			}else
			{
				f=wtk_wfstr_calc_prob(r,r->obs,hmm->pState[j]);
			}
			//wtk_debug("transf=%f/%f\n",transf,f);
			if(custom)
			{
				f*=r->cfg->custom_ac_scale;
				//wtk_debug("weight=%f\n",inst->trans->weight);
			}
			//wtk_debug("f=%f\n",f);
			score+=f*ac_scale;//+transf*trans_scale;
			//wtk_debug("score=%f/%f/%f\n",score,f,ac_scale);
			transf+=f;
			if(ac_lookahead_alpha>0)
			{
				lookahead_score=ac_lookahead_alpha*(f-tgt->ac_lookahead_score);
				score+=lookahead_score;
				res->ac_lookahead_score=f;
			}else
			{
				lookahead_score=0;
			}
			if(r->prune)
			{
				//wtk_debug("score=%f\n",score);
				wtk_prune_add(r->prune,score);
				//wtk_histogram_add_score(r->histogram,score,LZERO);
			}
			if(score>r->best_emit_score)
			{
				r->best_emit_score=score;
				//wtk_debug("v[%d]=%f\n",r->frame,r->best_emit_score);
			}
			if(score>max_score)
			{
				max_score=score;
			}
			res->like=score;
#ifdef USE_EXT_LIKE
			res->raw_like=tgt->raw_like+transf+lookahead_score;
#endif
			res->ac_like=tgt->ac_like+transf;
			res->lm_like=tgt->lm_like;
			res->path=tgt->path;
			++nactive;

#ifdef	USE_BIT_MAP
			inst->bit_map+=1<<j;
#endif
		}
//		else
//		{
//			*res=fst_null_tok;
//		}
	}
	r->step_inst1_tokset=inst->states;
	inst->states=bk_tok;
	inst->states[1]=fst_null_tok;
	inst->n_active_hyps=nactive;
	//wtk_debug("nactive=%d\n",nactive);
	return max_score;
}

wtk_wfstr_float_t  wtk_wfstr_step_hmm1_emit_state2_3(wtk_wfstr_t *r,wtk_fst_inst_t *inst)
{
	wtk_hmm_t *hmm;
	register wtk_fst_tok_t *res,*cur;
	wtk_fst_tok_t *bk_tok,*tgt=NULL;
	wtk_matrix_t *trans;
	short **pp,*p;
	int i,endi,j,n,x;
	float *pf;
	//double score,f;
	float score,f;
	wtk_wfstr_float_t ac_lookahead_alpha=r->cfg->ac_lookahead_alpha;
	wtk_wfstr_float_t max_score,transf,lookahead_score;
	int nactive=0;
	int use_hist=r->cfg->use_prune;
	float ac_scale=r->cfg->ac_scale;

#ifdef	USE_BIT_MAP
	int bit_map=0;

	bit_map=inst->bit_map;
	//wtk_wfstr_check_bit_map(inst);
//	if(bit_map==0)
//	{
//		return LZERO;
//	}
	inst->bit_map=0;
#endif
	hmm=inst->hmm;
	n=hmm->num_state;
	pp=r->hmmset->seIndexes[hmm->tIdx]+2;
	bk_tok=r->step_inst1_tokset;
	trans=hmm->transP;
	max_score=LZERO;
	for(j=2,res=bk_tok+2;j<n;++j,++res,++pp)
	{
		p=*pp;
		i=p[0];endi=p[1];
#ifdef	USE_BIT_MAP
		x=((bit_map>>i)<<(32-endi));
		if(x==0)
		{
			*res=fst_null_tok;
			continue;
		}
#endif
		cur=inst->states+i;
		pf=trans[j];
		if(cur->like>LSMALL)
		{
			score=cur->like+pf[i];
//			if(score<r->cur_emit_raw_thresh)
//			{
//				score=r->cur_emit_raw_thresh;
//			}
		}else
		{
			score=LZERO;
			///score=r->cur_emit_raw_thresh;
		}
		tgt=cur;
		x=i;
		//wtk_debug("score=%f/%f/%f\n",cur->tok.like,pf[i],score);
		//wtk_debug("v[%d] score=%f/%f norm=%f thresh=%f\n",j,cur->tok.like,pf[i],r->normalize_score,r->cur_emit_thresh)
		for(++cur,++i;i<=endi;++i,++cur)
		{
			//wtk_debug("%f/%f\n",*(xf),pf[i]);
			//wtk_debug("v[%d] score=%f/%f \n",j,cur->tok.like,pf[i]);
#ifdef	USE_BIT_MAP
			if((bit_map&(1<<i)) && (f=cur->like+ pf[i])>score)
			{
				tgt=cur;
				score=f;
			}
#else
			if(((f=cur->like+ pf[i])>score))
			{
				tgt=cur;
				score=f;
				//wtk_debug("%f/%f=%f\n",cur->tok.like,pf[i],f);
			}
#endif
		}
		//exit(0);
		if(score<LSMALL)
		//if(score<=r->cur_emit_raw_thresh)
		{
			*res=fst_null_tok;
			continue;
		}
		//wtk_debug("check in\n");
		//wtk_debug("score=%f/%f\n",score,r->cur_emit_thresh);
		if(use_hist)
		{
			score-=r->normalize_score;
		}
		//move from out to inside
		if(score>r->gen_max_tok.like)
		{
			r->gen_max_tok=*res;
			r->gen_max_tok.like=score;
			r->gen_max_trans=inst->trans;
		}
		//if(score>r->cur_emit_raw_thresh)
		if(score>r->cur_emit_thresh)
		{
			transf=pf[tgt-inst->states];
			//wtk_debug("transf=%f\n",transf);
#ifdef USE_ALIGN
			if(r->align_pool)
			{
				res->align=tgt->align;
				if(!res->align || res->align->state != j || res->align->trans != inst->trans)
				{
					res->align = wtk_wfstr_new_align(r, j,tgt->ac_like+transf, r->frame - 1, inst->trans, res->align);
				}
			}
#endif
			f=wtk_wfstr_calc_prob(r,r->obs,hmm->pState[j]);
			//wtk_debug("f=%f\n",f);
			score+=f*ac_scale;
			f+=transf;
			//if(ac_lookahead_alpha>0)
			{
				lookahead_score=ac_lookahead_alpha*(f-tgt->ac_lookahead_score);
				//lookahead_score=ac_lookahead_alpha*f;
				score+=lookahead_score;
				res->ac_lookahead_score=f;
			}
//			else
//			{
//				lookahead_score=0;
//			}
			//transf+=f;
			if(r->prune)
			{
				//wtk_debug("score=%f\n",score);
				wtk_prune_add(r->prune,score);
				//wtk_histogram_add_score(r->histogram,score,LZERO);
			}
			if(score>r->best_emit_score)
			{
				r->best_emit_score=score;
			}
			if(score>max_score)
			{
				max_score=score;
			}
			res->like=score;
#ifdef USE_EXT_LIKE
			res->raw_like=tgt->raw_like+f+lookahead_score;
#endif
			res->ac_like=tgt->ac_like+f;
			res->lm_like=tgt->lm_like;
			res->path=tgt->path;
			++nactive;
#ifdef	USE_BIT_MAP
			inst->bit_map+=1<<j;
#endif
		}
		else
		{
			*res=fst_null_tok;
		}
	}
	r->step_inst1_tokset=inst->states;
	inst->states=bk_tok;
	inst->states[1]=fst_null_tok;
	inst->n_active_hyps=nactive;
	//wtk_debug("ki=%d k1=%d k2=%d k3=%d\n",ki,k1,k2,k3);
	return max_score;
}

void wtk_wfstr_step_hmm1_exit_state(wtk_wfstr_t *r,wtk_fst_inst_t *inst)
{
	wtk_fst_tok_t *res;
	wtk_fst_tok_t *cur,*end;
	wtk_fst_tok_t *tgt;
	wtk_wfstr_float_t f,score;
	float *pf;
	short *p;
	int ns;

	ns=inst->hmm->num_state;
	p=r->hmmset->seIndexes[inst->hmm->tIdx][ns];
	res=inst->states+ns;
	cur=inst->states+p[0];
	end=inst->states+p[1];
	pf=inst->hmm->transP[ns]+p[0];
	score=cur->like+*(pf);
	tgt=cur;
	for(++cur;cur<=end;++cur)
	{
		if((f=cur->like+*(++pf))>score)
		{
			tgt=cur;
			score=f;
		}
	}
	if(score <= LZERO)
	{
		*res=fst_null_tok;
	}
#ifdef USE_REL_CHECK
	else if(r->cfg->use_rel && tgt->path && (!tgt->path->hook || !wtk_rel2_is_path_active(r->rescore.rel,tgt->path)))
	//else if(r->cfg->use_rel && tgt->path && (!tgt->path->hook))
	{
		*res=fst_null_tok;
	}
#endif
	else
	{
		//wtk_debug("ns=%d\n",ns);
#ifdef	USE_BIT_MAP
		inst->bit_map+=1<<ns;
#endif
		if(score>r->best_end_score)
		{
			r->best_end_score=score;
			r->exit_max_tok=*tgt;
		}
		*res=*tgt;
		res->like=score;
		f=*(inst->hmm->transP[ns]+(tgt-inst->states));
#ifdef USE_EXT_LIKE
		res->raw_like=tgt->raw_like+f;
#endif
		res->ac_like=tgt->ac_like+f;
		++inst->n_active_hyps;
		if(score>r->best_exit_score)
		{
			r->best_exit_score=score;
		}
#ifdef USE_ALIGN
		if(r->align_pool)
		{
			res->align = wtk_wfstr_new_align(r, -1, res->ac_like, r->frame,
					inst->trans, res->align);
		}
#endif
		if(inst->trans->out_id==0)
		{
			if(r->phn_prune)
			{
				++r->inst1_valid_phns;
			}
		}else
		{
			if(r->wrd_prune)
			{
				++r->inst1_valid_wrds;
			}
		}
	}
}

void wtk_wfstr_step_hmm1(wtk_wfstr_t *r,wtk_fst_inst_t *inst)
{
	wtk_wfstr_float_t max_score;
	wtk_fst_tok_t *res;

	//wtk_wfstr_check_bit_map(inst);
#ifdef	USE_BIT_MAP
	if(inst->bit_map==0)
	{
		max_score=LZERO;
	}else
#endif
	{
		max_score=wtk_wfstr_step_hmm1_emit_state2(r,inst);
	}
	//wtk_debug("max_score=%f\n",max_score);
	//wtk_debug("emit: %d\n",inst->n_active_hyps);
	if(max_score>LZERO)
	{
		wtk_wfstr_step_hmm1_exit_state(r,inst);
	}else
	{
		res=inst->states+inst->hmm->num_state;//r->hmmset->max_hmm_state;
		if(res->like>LZERO)
		{
			*res=fst_null_tok;
		}
	}
	//wtk_wfstr_check_bit_map(inst);
}


void wtk_wfstr_step_inst1(wtk_wfstr_t *r)
{
	//wtk_fst_net_nwlp_t *nwlp=r->net->cfg->nwlp;
	wtk_queue_node_t *n,*nxt;
	wtk_fst_inst_t *inst;
	wtk_fst_tok_t *tok;
	float thresh;
	float f;
	wtk_queue_t *q=&(r->inst_q);

	r->inst1_valid_phns=0;
	r->inst1_valid_wrds=0;
	r->best_emit_score=LZERO;
	r->best_exit_score=LZERO;
	r->best_end_score=LZERO;
	thresh=r->cur_start_thresh;
	n=r->inst_q.pop;
	while(n)
	{
		inst=data_offset2(n,wtk_fst_inst_t,inst_n);
		tok=inst->states+1;
		f=tok->like;
		if(f>LZERO && f<thresh)
		{
			//pruning entry state;
			*tok=fst_null_tok;
			--inst->n_active_hyps;
#ifdef	USE_BIT_MAP
			inst->bit_map=inst->bit_map&0xFFFFD;
#endif
			//wtk_wfstr_check_bit_map(inst);
		}
		wtk_wfstr_step_hmm1(r,inst);
		//wtk_debug("n=%d\n",inst->n_active_hyps);
		if(inst->n_active_hyps==0)
		{
			//wtk_debug("tok->path=%p\n",tok->tok.path);
			//n=wtk_wfstr_return_inst(r,q,n);
			nxt=n->next;
			wtk_queue_remove(q,n);
			//inst=data_offset2(n,wtk_fst_inst_t,inst_n);
			inst->trans->hook.inst=NULL;
			wtk_wfstr_push_inst(r,inst);
			n=nxt;
		}else
		{
			//wtk_debug("f=%d path=%p\n",r->frame,tok->tok.path);
			n=n->next;
		}
	}
	//wtk_debug("len=%d /%f\n",r->inst_q.length,r->best_emit_score);
	//exit(0);
	//wtk_debug("v[%d]=%d %f\n",r->frame,r->inst_q.length,r->best_emit_score);
}


void wtk_wfstr_get_valid_exit_state(wtk_wfstr_t *r,int *phn,int *wrd)
{
	wtk_queue_node_t *n;
	wtk_fst_inst_t *inst;
	wtk_fst_tok_t *tok;
	wtk_fst_trans_t *trans;
	int eps_id=r->net->cfg->eps_id;
	int n1,n2;
	wtk_hmm_t *hmm;
	wtk_prune_t *hg_phn=r->phn_prune;
	wtk_prune_t *hg_wrd=r->wrd_prune;

	n1=n2=0;
	for(n=r->inst_q.pop;n;n=n->next)
	{
		inst=data_offset(n,wtk_fst_inst_t,inst_n);
		hmm=inst->hmm;//wtk_fst_rec_get_inst_hmm(r,inst);
		tok=inst->states+hmm->num_state;
		if(tok->like>LZERO)
		{
			trans=inst->trans;
			if(trans->out_id == eps_id)
			{
				if(tok->like>r->cur_end_thresh)
				{
					++n1;
					if(hg_phn)
					{
						wtk_prune_add(hg_phn,tok->like);
					}
				}
			}else
			{
				if(tok->like>r->cur_wrd_thresh)
				{
					++n2;
					if(hg_wrd)
					{
						wtk_prune_add(hg_wrd,tok->like);
					}
				}
			}
		}
	}
	if(hg_phn)
	{
		wtk_prune_update_thresh(hg_phn);
		//wtk_debug("phn=%f %f/%f count=%d\n",hg_phn->thresh,hg_phn->min,hg_phn->max,hg_phn->count);
		wtk_prune_reset(hg_phn);
	}
	if(hg_wrd)
	{
		wtk_prune_update_thresh(hg_wrd);
		//wtk_debug("wrd=%f %f/%f count=%d\n",hg_wrd->thresh,hg_wrd->min,hg_wrd->max,hg_wrd->count);
		wtk_prune_reset(hg_wrd);
	}
	*phn=n1;
	*wrd=n2;
}

void wtk_wfstr_prune_inst(wtk_wfstr_t *r)
{
	wtk_queue_t *q=&(r->new_inst_q);
	wtk_prune_t *hg=r->expand_prune;
	//wtk_fst_rec_cfg_t *cfg=r->cfg;
	wtk_fst_tok_t *tok;
	wtk_queue_node_t *n;
	wtk_fst_inst_t *inst;
	double f;

	//wtk_debug("v[%d]=%d/%d/%d\n",r->frame,r->inst_q.length,r->new_inst_q.length,hg->cfg->count);
	//wtk_debug("len=%d thresh=%d cnt=%d\n",q->length,r->cfg->expand_emit_thresh,r->expand_thresh_cnt);
	if(q->length<=r->cfg->expand_prune_thresh)
	{
		return;
	}
	f=r->best_expand_score;
	wtk_prune_reset(hg);
	for(n=q->pop;n;n=n->next)
	{
		inst=data_offset2(n,wtk_fst_inst_t,inst_n);
		tok=inst->states+1;
		if(tok->like>LZERO)
		{
			wtk_prune_add(hg,tok->like-f);
		}
	}
	wtk_prune_update_thresh(hg);
	f=hg->thresh+r->best_expand_score;
	//wtk_debug("f=%f max=%f min=%f bwam=%f thresh=%f\n",f,hg->max,hg->min,hg->cfg->beam,hg->thresh);
	//exit(0);
	n=q->pop;
	while(n)
	{
		inst=data_offset2(n,wtk_fst_inst_t,inst_n);
		tok=inst->states+1;
		//wtk_debug("like=%f/%f len=%d\n",tok->like,f,q->length);
		if(tok->like>LZERO && tok->like<=f)
		{
			n=wtk_wfstr_return_inst(r,q,n);
		}else
		{
			//wtk_debug("[%f]\n",tok->tok.like);
			n=n->next;
		}
	}
}



void wtk_wfstr_step_inst2(wtk_wfstr_t *r)
{
	wtk_wfstrec_cfg_t *cfg=r->cfg;
	wtk_queue_node_t *n,*nxt;
	wtk_fst_inst_t *inst;
	wtk_fst_tok_t *tok;
	wtk_fst_trans_t *trans;
	wtk_fst_net_t *net=r->net;
	int eps_id=net->cfg->eps_id;
	wtk_hmm_t *hmm;
	int ret;
	wtk_queue_t *q=&(r->inst_q);
	int phns,wrds;
	char phn_use_hg=0;
	char wrd_use_hg=0;
	int b;

	phns=-1;wrds=-1;
	if((cfg->phn_prune_thresh>0 && r->inst1_valid_phns>cfg->phn_prune_thresh)
		|| (cfg->wrd_prune_thresh>0 && r->inst1_valid_wrds>cfg->wrd_prune_thresh))
	{
		//prune here;
		wtk_wfstr_get_valid_exit_state(r,&(phns),&(wrds));
		//wtk_debug("frame=%d inst=%d phns=%d/%d wrds=%d/%d\n",r->frame,r->inst_q.length,phns,r->inst1_valid_phns,wrds,r->inst1_valid_wrds);
		if(r->phn_prune && cfg->phn_prune_thresh>0 && phns>cfg->phn_prune_thresh)
		{
			phn_use_hg=1;
		}
		if(r->wrd_prune && cfg->wrd_prune_thresh>0 && wrds>cfg->wrd_prune_thresh)
		{
			wrd_use_hg=1;
		}
	}
	r->best_start_score=LZERO;
	r->best_expand_score=LZERO;
	n=r->inst_q.pop;
	while(n)
	{
		inst=data_offset2(n,wtk_fst_inst_t,inst_n);
		hmm=inst->hmm;//wtk_wfstr_get_inst_hmm(r,inst);
		tok=inst->states+hmm->num_state;
		//wtk_debug("like=%f\n",tok->like);
		if(tok->like>LZERO)
		{
			trans=inst->trans;
			//wtk_wfstr_check_depth_thresh(r,inst);
			if(trans->out_id == eps_id)
			{
				if(tok->like>r->cur_end_thresh)
				{
					b=1;
					if(phn_use_hg)
					{
						b=tok->like>r->phn_prune->thresh?1:0;
					}
					if(b)
					{
						ret=wtk_wfstr_process_tok(r,tok,trans);//,inst->states+r->hmmset->max_hmm_state-1);
						if(ret==-1)
						{
							inst->n_active_hyps=0;
						}
					}
				}
			}else
			{
				if(tok->like>r->cur_wrd_thresh)
				{
					b=1;
					if(wrd_use_hg)
					{
						b=tok->like>r->wrd_prune->thresh?1:0;
					}
					if(b)
					{
						ret=wtk_wfstr_process_tok(r,tok,trans);//,inst->states+r->hmmset->max_hmm_state-1);
						if(ret==-1)
						{
							inst->n_active_hyps=0;
						}
					}
				}
				//TODO check tok 是否要置zero ?
			}
			//*tok=fst_null_tok;
			//wtk_wfstr_check_depth_thresh(r,inst);
			--inst->n_active_hyps;
			//wtk_debug("nactive[%d]=%d\n",r->frame,inst->n_active_hyps);
			if(inst->n_active_hyps<=0)
			{
				//n=wtk_wfstr_return_inst(r,&(r->inst_q),n);
				nxt=n->next;
				wtk_queue_remove(q,n);
				//inst=data_offset2(n,wtk_fst_inst_t,inst_n);
				inst->trans->hook.inst=NULL;
				wtk_wfstr_push_inst(r,inst);
				n=nxt;
			}else
			{
				n=n->next;
			}
		}else
		{
			//wtk_debug("v[%d]: next=%p:%d\n",r->frame,n->next,inst->n_active_hyps);
			n=n->next;
		}
	}
	//wtk_debug("v[%d]: inst=%d\n",r->frame,r->inst_q.length);//,r->lat_net->active_node_q.length);
	if(r->expand_prune)
	{
		wtk_wfstr_prune_inst(r);
	}
	wtk_fst_join_new_inst_q(r);
	//wtk_debug("v[%d]: inst=%d\n",r->frame,r->inst_q.length);//,r->lat_net->active_node_q.length);
}



int wtk_wfstr_wrd_inst(wtk_wfstr_t *r)
{
	wtk_queue_node_t *n;
	wtk_fst_inst_t *inst;
	int cnt=0;

	for(n=r->inst_q.pop;n;n=n->next)
	{
		inst=data_offset(n,wtk_fst_inst_t,inst_n);
		if(inst->trans->out_id>0)
		{
			++cnt;
		}
	}
	return cnt;
}

int wtk_wfstr_phn_inst(wtk_wfstr_t *r)
{
	wtk_queue_node_t *n;
	wtk_fst_inst_t *inst;
	int cnt=0;

	for(n=r->inst_q.pop;n;n=n->next)
	{
		inst=data_offset(n,wtk_fst_inst_t,inst_n);
		if(inst->trans->out_id==0)
		{
			++cnt;
		}
	}
	return cnt;
}

void wtk_wfstr_process_rescore(wtk_wfstr_t *r)
{
	wtk_fst_net3_t *lat_net;
	double t;

	if(r->cfg->use_lat)
	{
		lat_net=r->lat_net;
		lat_net->last_frame=r->frame;
		if(lat_net->end)
		{
			wtk_wfstr_reset_net3_end(r,lat_net);
			lat_net->end=0;
		}
		if( r->cfg->lat_net.prune_frames>0 &&
		(r->cfg->lat_net.node_prune_thresh<0 || lat_net->active_node_q.length>r->cfg->lat_net.node_prune_thresh)
				&& r->frame%r->cfg->lat_net.prune_frames==0)
		{
			t=time_get_cpu();
			wtk_wfstr_prune_lat(r,lat_net->cfg->prune_beam,-1);
			r->lat_prune_time+=time_get_cpu()-t;
		}
	}
}


void wtk_wfstr_process_rescore2(wtk_wfstr_t *r)
{
}

wtk_queue_node_t* wtk_wfstr_check_output_id(wtk_wfstr_t *r)
{
	static wtk_queue_node_t *xn=NULL;
	wtk_queue_node_t *qn;
	wtk_fst_inst_t *inst;
	wtk_fst_tok_t *tokset;
	int i;
	int b=0;
	int ki;

	//if(r->frame!=109){return;}
	//if(r->frame<158){return;}
	printf("\n\n");
	wtk_debug("==========> frame=%d len=%d xn=%p\n",r->frame,r->inst_q.length,xn);
//	if(xn)
//	{
//		qn=wtk_queue_find_node(&(r->inst_q),xn);
//		wtk_debug("qn=%p\n",qn);
//	}
	for(ki=0,qn=r->inst_q.pop;qn;qn=qn->next,++ki)
	{
		inst=data_offset2(qn,wtk_fst_inst_t,inst_n);
//		if(qn==xn)
//		{
//			wtk_debug("foudn qn=%p\n",qn);
//		}
		for(i=1;i<=inst->hmm->num_state;++i)
		{
			tokset=inst->states+i;
			if(tokset->like>LZERO && tokset->path)
			{
				//if(xn==qn)
				{
					wtk_wfstr_print_path(r,tokset->path);
				}
				if(0 && wtk_wfst_path_has_out_id2(tokset->path,3) && wtk_wfst_path_last_wrd(tokset->path)==4)
				{
					//wtk_debug("v[%d]: qn=%p i=%d\n",ki,qn,i);
					wtk_wfstr_print_path(r,tokset->path);
					//wtk_fst_net_cfg_print_trans_next_x(r->net->cfg,tokset->tok.path->trans);
					//exit(0);
					//xn=qn;
					b=1;
					goto end;
				}
			}
		}
	}
end:
	wtk_debug("v[%d]=%d\n",r->frame,b);
	if(b)
	{
		//exit(0);
	}
	return qn;
}

void wtk_wfstr_can_be_end(wtk_wfstr_t *r)
{
	//wtk_debug("v[%d]: nsil=%d nphn=%d %f nwrd=%d\n",r->frame,r->nsil,r->nphn,r->nsil*1.0/r->nphn,r->nwrd);
	if(r->frame>r->cfg->min_sil_frame && (r->nwrd>0) && (r->nsil*1.0/r->nphn>r->cfg->min_sil_thresh) && (r->nwrd<r->cfg->max_sil_wrd))
	{
//		wtk_debug("v[%d]: nsil=%d nphn=%d %f nwrd=%d\n",r->frame,r->nsil,r->nphn,r->nsil*1.0/r->nphn,r->nwrd);
//		if(r->max_pth)
//		{
//			wtk_debug("max_tok=%d [%.*s]\n",r->max_pth->trans->out_id,r->net_cfg->sym_out->strs[r->max_pth->trans->out_id]->len,
//					r->net_cfg->sym_out->strs[r->max_pth->trans->out_id]->data
//					);
//		}
//		if(r->max_pth && r->last_pth && r->max_pth->trans->out_id==r->last_pth->trans->out_id)
//		{
//			++r->end_hint_frame;
//		}else
//		{
//			r->end_hint_frame=0;
//		}
		++r->end_hint_frame;
		if(r->end_hint_frame>r->cfg->min_sil_end_frame)
		{
			r->end_hint=1;
		}
	}else
	{
		r->end_hint_frame=0;
	}
	//r->last_pth=r->max_pth;
}


void wtk_wfstr_feed3(wtk_wfstr_t *r,wtk_vector_t *obs)
{
	wtk_wfstrec_cfg_t *cfg=r->cfg;
	int n;

	++r->frame;
	//wtk_debug("v[%d]: inst=%d\n",r->frame,r->inst_q.length);//,r->lat_net->active_node_q.length);
	//wtk_vector_print(obs);
	//exit(0);
	r->best_final_tokset=fst_null_tok;
	r->best_ebnf_final_tokset=fst_null_tok;
	r->gen_max_tok=fst_null_tok;
	r->exit_max_tok=fst_null_tok;
	r->gen_max_trans=NULL;
	if(r->cfg->hlda_fn)
	{
		if(r->hmmset->cfg &&r->hmmset->cfg->use_fix)
		{
			wtk_fixi_mult_mv(r->hlda.fix,r->cfg->hlda_matrix,obs,r->hmmset->cfg->mean_scale);
		}else
		{
			wtk_matrix_multiply_vector(r->hlda.vector,r->cfg->hlda_matrix,obs);
			r->obs=r->hlda.vector;
		}
	}else
	{
		if(r->hmmset->cfg &&r->hmmset->cfg->use_fix && obs)
		{
			wtk_fixi_scale(r->hlda.fix,obs,r->hmmset->cfg->mean_scale);
		}
		if(obs)
		{
			r->obs=obs;
		}else
		{
			r->obs=r->hlda.vector;
		}
	}
	//wtk_vector_print(r->obs);
	//exit(0);
	//wtk_debug("best[%d]=%f\n",r->frame,r->best_emit_score);
	r->normalize_score=(r->best_emit_score>LZERO)?r->best_emit_score:0.0;
	if(r->prune)
	{
		n=r->cfg->max_emit_hyps;
		//r->cur_emit_thresh=wtk_histogram_calc_thresh(r->histogram,n);
		r->cur_emit_thresh=wtk_prune_get_thresh2(r->prune,n);
		//wtk_debug("thresh=%f count=%d\n",r->cur_emit_thresh,r->prune->count);
		r->cur_emit_thresh-=r->normalize_score;
		//wtk_debug("v[%d]: emit=%f,norm=%f\n",r->frame,r->cur_emit_thresh,r->normalize_score);
		if(r->emit_beam>0.0 && r->cur_emit_thresh<-r->emit_beam)
		{
			r->cur_emit_thresh=-r->emit_beam;
		}
		wtk_prune_reset(r->prune);
		//wtk_histogram_reset(r->histogram);
	}else
	{
		r->cur_emit_thresh=(r->emit_beam>0)?r->best_emit_score-r->emit_beam:LSMALL;
	}
	r->cur_emit_raw_thresh=r->cur_emit_thresh+r->normalize_score;
	//wtk_debug("best[%d]= emit=%f raw=%f\n",r->frame,r->emit_beam,r->cur_emit_raw_thresh);
	if(r->cfg->use_single_best)
	{
		r->cur_start_thresh=(cfg->phn_start_beam>0)?
			(r->best_emit_score-cfg->phn_start_beam):LSMALL;
	}else
	{
		r->cur_start_thresh=(cfg->phn_start_beam>0)?
			(r->best_start_score-cfg->phn_start_beam):LSMALL;
	}
	//wtk_debug("best=%f/%f\n",r->best_start_score,r->best_emit_score);
	//wtk_debug("step inst1 len=%d\n",r->inst_q.length);
	wtk_wfstr_step_inst1(r);
	//wtk_debug("step inst1 len=%d\n",r->inst_q.length);
	if(r->use_rescore)
	{
		r->cur_lat_thresh=(cfg->lat_beam>0)?(r->best_exit_score-cfg->lat_beam):LSMALL;
	}
	if(r->cfg->use_single_best)
	{
		r->cur_end_thresh=(cfg->phn_end_beam>0)?
			(r->best_emit_score-cfg->phn_end_beam):LSMALL;
		r->cur_wrd_thresh=(cfg->word_beam>0)?
			(r->best_emit_score-cfg->word_beam):LSMALL;
	}else
	{
		r->cur_end_thresh=(cfg->phn_end_beam>0)?
			(r->best_end_score-cfg->phn_end_beam):LSMALL;
		r->cur_wrd_thresh=(cfg->word_beam>0)?
			(r->best_end_score-cfg->word_beam):LSMALL;
	}
	if(r->use_rescore)
	{
		//wtk_debug("rescore len=%d\n",r->inst_q.length);
		wtk_wfstr_process_rescore(r);
	}
	if(r->cfg->use_end_hint)
	{
		r->nsil=0;
		r->nphn=0;
		r->nwrd=0;
		r->max_pth=NULL;
	}
	//wtk_debug("step inst2 len=%d\n",r->inst_q.length);
	//wtk_wfstr_check_output_id(r);
	//wtk_debug("v[%d]=%d\n",r->frame,r->inst_q.length);

	wtk_wfstr_step_inst2(r);
	//wtk_debug("step inst2 len=%d\n",r->inst_q.length);
	if(r->cfg->use_end_hint)
	{
		wtk_wfstr_can_be_end(r);
	}
	//wtk_wfstr_process_tok(r,&(r->init_tokset),0);
	//wtk_debug("max_path=%p\n",r->gen_max_tok.path);
}


void wtk_wfstr_feed2(wtk_wfstr_t *r,wtk_feat_t *f)
{
	if(r->dnn_get)
	{
		//wtk_debug("frame=%d f=%p app=%p index=%d cnt=%d\n",r->frame,f,f->app_hook,f->index,r->dnn_cnt);
		if(!f->app_hook)
		{
			r->f=f;
			++r->cache_frame;
		}else
		{
			//++r->frame;
			f=(wtk_feat_t*)(f->app_hook);
		}
		wtk_wfstr_feed3(r,f->rv);
	}else
	{
		if(f->app_hook)
		{
			f=(wtk_feat_t*)(f->app_hook);
		}else
		{
			r->f=f;
		}
		wtk_wfstr_feed(r,f->rv);
	}
}

void wtk_wfstr_feed(wtk_wfstr_t *r,wtk_vector_t *obs)
{
	++r->cache_frame;
	wtk_wfstr_feed3(r,obs);
}

//------------------------ collect path -------------------------
#ifdef  COLLECT_PATH
void wtk_wfstr_ref_path(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	wtk_queue_t *q;

	if(pth->usage==0)
	{
		//q=&(r->path_no_q);
		q=pth->queue;
		wtk_queue_remove(q,&(pth->q_n));
		pth->queue=&(r->path_yes_q);
		wtk_queue_push_front(pth->queue,&(pth->q_n));
	}
	++pth->usage;
}

void wtk_wfstr_dec_rec_path_ref(wtk_wfstr_t *r,wtk_wfst_path_t *path)
{
	path=path->prev;
	if(!path){return;}
	--path->usage;
	if(path->usage==0)
	{
		//wtk_queue_remove(&(r->path_yes_q),&(path->q_n));
		wtk_queue_remove(path->queue,&(path->q_n));
		path->queue=&(r->path_no_q);
		wtk_queue_push_front(path->queue,&(path->q_n));
	}
}

void wtk_wfstr_move_path_yes_ref(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	/*
	wtk_queue_remove(&(r->path_no_q),&(pth->q_n));
	wtk_queue_push_front(&(r->path_yes_q),&(pth->q_n));
	*/
	wtk_queue_remove(pth->queue,&(pth->q_n));
	pth->queue=&(r->path_yes_q);
	wtk_queue_push_front(pth->queue,&(pth->q_n));
}

void wtk_wfstr_unlink_path(wtk_wfstr_t *rec,wtk_wfst_path_t *pth)
{
	//wtk_fst_net3_t *net;
	//wtk_fst_arc_t *arc;

	//wtk_debug("v[%d]: remove path=%p:%d\n",i,pth,pth->count);
	//net=rec->lat_net;
	//arc=(wtk_fst_arc_t*)(pth->hook);
	//wtk_debug("remove[%d]: arc=%p %p=>%p\n",i,arc,arc->from_node,arc->to_node);
	//wtk_fst_net3_unlink_arc(net,arc);
	wtk_queue_remove(pth->queue,&(pth->q_n));
	pth->prev=NULL;
	pth->nxt_chain=NULL;
	pth->frame=0;
	pth->hook=NULL;
	pth->queue=NULL;
	wtk_vpool_push(rec->pth_pool,pth);
}

void wtk_wfstr_collect_path(wtk_wfstr_t *r)
{
	wtk_fst_inst_t *inst;
	wtk_wfst_path_t *pth;
	wtk_queue_node_t *n,*p;
	int i,len;

	len=r->hmmset->max_hmm_state;
	for(n=r->inst_q.pop;n;n=n->next)
	{
		inst=data_offset2(n,wtk_fst_inst_t,inst_n);
		for(i=1;i<=len;++i)
		{
			pth=inst->states[i].tok.path;
			if(pth && !pth->used)
			{
				if(pth->usage>0)
				{
					wtk_wfstr_move_path_yes_ref(r,pth);
				}
				pth->used=1;
			}
		}
	}
	for(n=r->path_no_q.pop;n;n=p)
	{
		p=n->next;
		pth=data_offset2(n,wtk_wfst_path_t,q_n);
		if(!pth->used)
		{
			wtk_wfstr_dec_rec_path_ref(r,pth);
			wtk_wfstr_unlink_path(r,pth);
		}else
		{
			pth->used=0;
		}
	}
	for(n=r->path_yes_q.pop;n;n=n->next)
	{
		pth=data_offset2(n,wtk_wfst_path_t,q_n);
		if(!pth->used)
		{
			break;
		}
		pth->used=0;
	}
}
#endif
//----------------------- finish ---------------------------------------

wtk_string_t* wtk_wfstr_finish(wtk_wfstr_t *r)
{
	wtk_strbuf_t *buf=r->buf;
	wtk_string_t *v;

	//wtk_debug("x0=%d,x1=%d,x2=%d,xn=%d\n",x0,x1,x2,xn);
	wtk_wfstr_finish2(r,buf," ",1);
	v=wtk_heap_dup_string(r->heap,buf->data,buf->pos);
	return v;
}

wtk_string_t* wtk_wfstr_get_outwrd(wtk_wfstr_t* r, wtk_wfst_path_t* pth)
{
	wtk_string_t *v = NULL;

	while(pth)
	{
		if(pth->trans->out_id > 0)
		{
			v = r->net->cfg->sym_out->strs[pth->trans->out_id];
			break;
		}

		pth = pth->prev;
	}

	return v;
}

wtk_fst_tok_t* wtk_wfstr_get_tok(wtk_wfstr_t *r)
{
	wtk_fst_tok_t *tok=NULL;
	float like;

//	wtk_debug("like=%f/%f/%f\n",r->best_final_tokset.tok.raw_like,r->gen_max_tok.raw_like,r->exit_max_tok.raw_like);
//	wtk_wfstr_print_path(r,r->best_final_tokset.tok.path);
//	wtk_wfstr_print_path(r,r->gen_max_tok.path);
//	wtk_wfstr_print_path(r,r->exit_max_tok.path);
	if(r->cfg->use_max_like_path)
	{
		like=LZERO;
		if(r->best_final_tokset.path)
		{
			tok=&(r->best_final_tokset);
#ifdef USE_EXT_LIKE
			like=r->best_final_tokset.raw_like+r->cfg->max_final_tok_pad_like;
#else
			like=r->best_final_tokset.ac_like+r->best_final_tokset.lm_like+r->cfg->max_final_tok_pad_like;
#endif
		}
#ifdef USE_EXT_LIKE
		if(r->gen_max_tok.path && r->gen_max_tok.raw_like>like)
#else
		if(r->gen_max_tok.path && (r->gen_max_tok.ac_like+r->gen_max_tok.lm_like)>like)
#endif
		{
			tok=&(r->gen_max_tok);
#ifdef USE_EXT_LIKE
			like=r->gen_max_tok.raw_like;
#else
			like=r->gen_max_tok.ac_like+r->gen_max_tok.lm_like;
#endif
		}
#ifdef USE_EXT_LIKE
		if(r->exit_max_tok.path && r->exit_max_tok.raw_like>like)
#else
		if(r->exit_max_tok.path && (r->exit_max_tok.ac_like+r->exit_max_tok.lm_like)>like)
#endif
		{
			tok=&(r->exit_max_tok);
#ifdef USE_EXT_LIKE
			like=r->exit_max_tok.raw_like;
#else
			like=r->exit_max_tok.ac_like+r->exit_max_tok.lm_like;
#endif
		}
	}else
	{
		if(r->ebnf_net)
		{
			//wtk_debug("path=%p\n",r->best_final_tokset.path);
			if(r->best_final_tokset.path)
			{
//				wtk_debug("=========== 1 \n");
//				if(r->gen_max_tok.path)
//				{
//					wtk_debug("max=%d\n",r->gen_max_tok.path->trans->to_state->custom);
//				}
//				if(r->exit_max_tok.path)
//				{
//					wtk_debug("exit=%d\n",r->exit_max_tok.path->trans->to_state->custom);
//				}
				if(r->best_ebnf_final_tokset.path)
				{
					float f1,f2;

					f1=r->best_final_tokset.path->ac_like/r->best_final_tokset.path->frame;
					//wtk_debug("ac=%f frame=%d conf=%f\n",r->best_final_tokset.path->ac_like,r->best_final_tokset.path->frame,f1);
					f2=r->best_ebnf_final_tokset.path->ac_like/r->best_ebnf_final_tokset.path->frame;
					//wtk_debug("ac=%f frame=%d conf=%f\n",r->best_ebnf_final_tokset.path->ac_like,r->best_ebnf_final_tokset.path->frame,f2);
					//wtk_debug("f1=%f,f2=%f\n",f1,f2);
					if(f2>f1)
					{
						return &(r->best_ebnf_final_tokset);
					}
				}
				tok=&(r->best_final_tokset);
			}else if(r->best_ebnf_final_tokset.path)
			{
				//wtk_debug("---------------2\n");
				tok=&(r->best_ebnf_final_tokset);
			}
			else if(r->cfg->use_forceout)
			{
				if(r->gen_max_tok.path)
				{
					tok=&(r->gen_max_tok);
				}else
				{
					tok=&(r->exit_max_tok);
				}
			}
		}else
		{
			if(r->best_final_tokset.path)
			{
				tok=&(r->best_final_tokset);
			}else if(r->cfg->use_forceout)
			{
				if(r->gen_max_tok.path)
				{
					tok=&(r->gen_max_tok);
				}else
				{
					tok=&(r->exit_max_tok);
				}
			}
		}
	}
	return tok;
}

wtk_wfst_path_t *wtk_wfstr_get_path(wtk_wfstr_t *r)
{
	wtk_fst_tok_t *tok;

	tok=wtk_wfstr_get_tok(r);
	return tok?tok->path:NULL;
}

double wtk_wfstr_calc_conf(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	double like=0,f;
	int frame=0;
	int i;
	wtk_string_t *v;
	//int frame2=pth->frame;
	double score;
	int cnt;

	cnt=0;
	score=0;
	//like=pth->ac_like;
	//wtk_debug("tot=%f frame=%d like=%f\n",pth->ac_like,pth->frame,pth->ac_like/pth->frame);
	while(pth)
	{
		if(r->net->print && r->net->print->get_insym)
		{
			v=r->net->print->get_insym(r->net->print->ths,pth->trans->in_id);
		}else
		{
			v=r->net_cfg->sym_in->ids[pth->trans->in_id]->str;
		}
		if(wtk_string_cmp_s(v,"sil")==0 || wtk_string_cmp_s(v,"<eps>")==0)
		{

		}else
		{
			if(pth->prev)
			{
				i=pth->frame-pth->prev->frame;
				f=pth->ac_like-pth->prev->ac_like;
			}else
			{
				i=pth->frame;
				f=pth->ac_like;
			}
			like+=f;
			frame+=i;
			//like+=pth->ac_like;
			score+=f/i;
			++cnt;
			wtk_debug("[%.*s] like=%f/%f ,frame=%d/%d\n",v->len,v->data,f,like,i,frame);
		}
		//wtk_debug("f=%d ac=%f [%.*s]\n",pth->frame,pth->ac_like,v->len,v->data);
		pth=pth->prev;
	}
	if(cnt>0)
	{
		score/=cnt;
	}
	f=like/frame;
	//wtk_debug("===============> like=%f fame=%d ac=%f score=%f\n",like,frame,like/frame,score);
	//exit(0);
	wtk_debug("f=%f score=%f\n",f,score);
	f=min(f,score);
	return f;
}

float wtk_wfstr_get_path_conf(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	return pth->ac_like/(pth->frame);//*r->cfg->ac_lookahead_alpha);
}

float wtk_wfstr_get_conf(wtk_wfstr_t *r)
{
	wtk_fst_tok_t *tok;

	if(r->frame<=0)
	{
		return 0;
	}
	tok=wtk_wfstr_get_tok(r);
//	wtk_debug("ac=%f frame=%d like=%f\n",tok->path->ac_like,tok->path->frame,tok->path->ac_like/tok->path->frame);
	//wtk_wfstr_print_path(r,tok->path);
//	wtk_wfstr_calc_conf(r,tok->path);
#ifndef	X_DNN_CONF
	if(tok && tok->path && tok->path->frame>0)
	{
		//wtk_debug("ac=%f,frame=%d %f\n",tok->path->ac_like,tok->path->frame,wtk_wfstr_calc_conf(r,tok->path));
		//exit(0);
		return tok->path->ac_like/(tok->path->frame);//*r->cfg->ac_lookahead_alpha);
	}else
	{
		return 0;
	}
#else
	float conf;
	float score_avg = LZERO;
	int n_states = 0;

	if(tok)
	{
		score_avg=tok->score_avg;
		n_states=tok->n_states;
//		if(tok->path)
//		{
//			wtk_wfstr_print_path(r,tok->path);
//		}
	}

	if(n_states > 0)
	{
		conf = score_avg / n_states;
		conf -=r->cfg->dnn.conf_bias;
		//conf=log(conf);
	}else
	{
		conf=0;
	}
	wtk_debug("%f/%d/%f\n",score_avg,n_states,conf);
	if(r->cfg->dnn.conf_min!=r->cfg->dnn.conf_max)
	{
		// normlize
		conf = (conf -r->cfg->dnn.conf_min) / (r->cfg->dnn.conf_max-r->cfg->dnn.conf_min);
		if(conf>1)
		{
			conf = 1;
		}else if(conf<0)
		{
			conf=0;
		}
	}
	return conf;
#endif
}


void wtk_wfstr_finish4(wtk_wfstr_t *r,wtk_wfst_path_t *pth,wtk_strbuf_t *buf,char *sep,int sep_bytes)
{
	wtk_fst_net_t *net;
	wtk_fst_net_cfg_t *cfg;//=r->net->cfg;
	wtk_string_t *v;

	if(!pth)
	{
		return;
	}
	if(r->ebnf_net && pth->trans->to_state->custom)
	{
		net=r->ebnf_net;
	}else
	{
		net=r->net;
	}
	cfg=net->cfg;
	wtk_strbuf_reset(buf);
	//wtk_debug("score=%f\n",pth->ac_like);
	//wtk_wfstr_print_path(r,pth);
	while(pth)
	{
		//if(pth->trans->out_id>2)
		if(pth->trans && wtk_fst_net_cfg_is_visible_wrd(cfg,pth->trans->out_id))
		{
			if(r->net->print)
			{
				v=r->net->print->get_outsym(net->print->ths,pth->trans->out_id);
			}else
			{
				v=cfg->sym_out->strs[pth->trans->out_id];
			}
			if(wtk_string_cmp_s(v,"sil")!=0)
			{
				if(buf->pos>0 && sep_bytes>0)
				{
					wtk_strbuf_push_front(buf,sep,sep_bytes);
				}
				//wtk_debug("[%.*s]=%d\n",v->len,v->data,pth->frame);
				wtk_strbuf_push_front(buf,v->data,v->len);
			}
		}
		pth=pth->prev;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	//wtk_strbuf_delete(buf);
	//wtk_debug("rec: %fMB\n",wtk_wfstr_bytes(r)*1.0/(1024*1024));
	//wtk_debug("inst_max=%d\n",r->inst_max_use);
	//wtk_fst_net_kv_reset(net->kv);
	//wtk_wfstr_print_path(r,r->best_final_tokset.tok.path);
}

void wtk_wfstr_finish2(wtk_wfstr_t *r,wtk_strbuf_t *buf,char *sep,int sep_bytes)
{
	wtk_wfst_path_t *pth;

	//wtk_debug("===================\n");
	wtk_strbuf_reset(buf);
	pth=wtk_wfstr_get_path(r);
	//wtk_debug("ac=%f lm=%f\n",pth->ac_like,pth->lm_like);
	//wtk_wfstr_print_path(r,pth);
	//printf("\n");
	//wtk_debug("ac=%f lm=%f raw=%f\n",pth->ac_like,pth->lm_like,pth->raw_like);
	//wtk_wfstr_print_path(r,pth);
	wtk_wfstr_finish4(r,pth,buf,sep,sep_bytes);

}

void wtk_wfstr_get_insym(wtk_wfstr_t *r,wtk_wfst_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_fst_net_cfg_t *cfg=r->net->cfg;
	wtk_string_t *v;

	wtk_strbuf_reset(buf);
	while(pth)
	{
		//if(pth->trans->out_id>2)
		if(pth->trans->in_id>0)
		{
			v=cfg->sym_in->ids[pth->trans->in_id]->str;
			if(buf->pos>0)
			{
				wtk_strbuf_push_front_s(buf," ");
			}
			wtk_strbuf_push_front(buf,v->data,v->len);
		}
		pth=pth->prev;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	//wtk_strbuf_delete(buf);
	//wtk_debug("rec: %fMB\n",wtk_wfstr_bytes(r)*1.0/(1024*1024));
	//wtk_debug("inst_max=%d\n",r->inst_max_use);
	//wtk_fst_net_kv_reset(net->kv);
	//wtk_wfstr_print_path(r,r->best_final_tokset.tok.path);
}



int wtk_wfstr_update_lat(wtk_wfstr_t *rec)
{
	wtk_fst_net3_t *lat_net;
	wtk_fst_node_t *end;
//	wtk_fst_tok_t *tok;

	lat_net=rec->lat_net;
	if(!lat_net->end)
	{
		if(1)
		{
			//wtk_debug("max=%p\n",rec->gen_max_trans);
			if(!rec->gen_max_trans || rec->gen_max_trans->to_state->custom)
			{
				return -1;
			}
			end=wtk_fst_net3_get_trans_node(lat_net,rec->gen_max_trans,rec->frame);
			//end=wtk_fst_net3_get_trans_node(lat_net,tok->path->trans,rec->frame);
			//end->eof=1;
			wtk_fst_node_set_eof(end,1);
			lat_net->end=end;
		}else
		{
			wtk_wfstr_lat_add_end(rec);
			if(!lat_net->end)
			{
				return -1;
			}
			{
				wtk_fst_tok_t *tok;

				tok=wtk_wfstr_get_tok(rec);
				wtk_wfstr_print_path(rec,tok->path);
			}
		}
	}
	lat_net->last_frame=rec->frame;
	wtk_fst_net3_update_eof(lat_net);
	wtk_fst_net3_finish(lat_net);
	//wtk_debug("s=%p e=%p n=%p len=%d\n",lat_net->start,lat_net->end,lat_net->null_node,lat_net->active_node_q.length);
	if(rec->cfg->dump_lat)
	{
		//wtk_debug("write lat\n");
		wtk_fst_net3_write_lat2(lat_net,"2.lat");
	}
	//exit(0);
	return 0;
}

int wtk_wfstr_create_lat_fst(wtk_wfstr_t *rec,wtk_fst_net2_t *net)
{
	int ret;

	ret=wtk_wfstr_update_lat(rec);
	if(ret!=0){goto end;}
	ret=wtk_fst_net3_to_net2(rec->lat_net,net);
end:
	return ret;
}
#ifdef USE_ALIGN
void wtk_fst_nxtpath_from_path(wtk_wfst_nxtpath_t *nxt_path, wtk_wfst_path_t *path)
{
    nxt_path->prev = path->prev;
    //nxt_path->like=path->like;
    nxt_path->lm_like = path->lm_like;
    nxt_path->ac_like = path->ac_like;
    nxt_path->nxt_chain = path->nxt_chain;
    nxt_path->align = path->align;
}
static wtk_string_t null=wtk_string("!NULL");
void wtk_fst_rec_lat_from_path(wtk_wfstr_t *rec, wtk_lat_t *lat, wtk_wfst_path_t *path, int *ln)
{
    wtk_wfst_nxtpath_t tmp, *nxt_path;
    wtk_lnode_t *ne, *ns;
    wtk_string_t **sym = rec->net->cfg->sym_out->strs;
    wtk_larc_t *la;
    wtk_fst_float_t pr_aclike, pr_lmlike;

    wtk_wfst_align_t *align, *al, *pr;
    int i, frame, n;
    double like, dur;
    float frame_dur = rec->cfg->frame_dur;
    wtk_string_t *labpr = NULL, *labid;
    char buf[64];

    wtk_fst_nxtpath_from_path(&tmp, path);
    ne = lat->lnodes - path->usage;
    //wtk_debug("ne=%d\n",-path->usage);
    ne->time = path->frame * rec->cfg->frame_dur;
    //wtk_debug("ne->time = %f, frame = %d, usage = %d\n", ne->time, path->frame, path->usage);
    //wtk_debug("label=%d\n",path->label);
    if( path->prev == NULL || ( (!path->prev->trans || path->prev->trans->out_id <= 0) && *ln == -1 ) ) {
        //wtk_debug("fst nil ...\n");
        ne->info.fst_wrd = &null;
    } else {
        ne->info.fst_wrd = sym[path->prev->trans->out_id];
        //wtk_debug("[%.*s]\n",ne->info.fst_wrd->len,ne->info.fst_wrd->data);
    }
    ne->v = 1;
    ne->score = path->ac_like;
    for(nxt_path = &tmp; nxt_path; nxt_path = nxt_path->nxt_chain) {
        la = lat->larcs + (++(*ln));
        //wtk_debug("nxt_path->prev = %p\n", nxt_path->prev);
        if(nxt_path->prev) {
            ns = lat->lnodes - nxt_path->prev->usage;
            pr_aclike = nxt_path->prev->ac_like;
            //pr_aclike=nxt_path->prev->ac_score;
            pr_lmlike = nxt_path->prev->lm_like;
        } else {
            ns = lat->lnodes;
            //wtk_debug("time = %f \n", ns->time);
            pr_aclike = 0;
            //pr_aclike=0;
            pr_lmlike = 0;
        }
        //wtk_debug("la=%p,ln=%d,usage=%d\n",la,*ln,nxt_path->prev?-nxt_path->prev->usage:0);
        la->start = ns;
        la->end = ne;
        la->prlike = 0;
        //la->score=nxt_path->score;
        //la->score=nxt_path->ac_like;
        //la->aclike=nxt_path->ac_score-pr_aclike;
        la->lmlike = (nxt_path->lm_like - pr_lmlike) / rec->net->cfg->lmscale;
        la->aclike = nxt_path->ac_like - pr_aclike;

        la->farc = ns->foll;
        la->parc = ne->pred;
        ns->foll = ne->pred = la;

        //wtk_debug("wrd[%p]=%f\n",ne->info.fst_wrd,la->aclike);
        if(nxt_path->prev && !ns->info.fst_wrd) {
            wtk_fst_rec_lat_from_path(rec, lat, nxt_path->prev, ln);
        }
        /*-----------------------  add align  ---------------*/
        align = nxt_path->align;
        if (align) {
            for (i = 0, al = align; al; al = al->prev, ++i)
                ;
            la->nalign = i;
            la->lalign = (wtk_lalign_t*) wtk_heap_malloc(rec->heap,
                         sizeof(wtk_lalign_t) * i);
            frame = path->frame;
            //printf("path%p: frame = %d , aclike = %f lmlike = %f \n", path, path->frame, path->ac_like, path->lm_like);
            //Allow for wp diff between path and align
            like = nxt_path->ac_like;
            for (pr = 0, al = align; al; al = al->prev) {
                //hmm = al->node->info.hmm;
                //wtk_debug("hmm: %.*s\n",hmm->name->len,hmm->name->data);
                //wtk_debug("pr=%p,like=%f\n",pr,pr?pr->like:-1);
                if (al->state < 0) {
                    if (!pr) {
                        pr = al;
                        labpr = rec->net->cfg->sym_in->ids[align->trans->in_id]->str;
                        //wtk_netnode_print(pr->node);
                        continue;
                    }
                    dur = (pr->frame - al->frame) * frame_dur; //lat->frame_dur;
                    like = pr->like - al->like;
                    pr = al;
                    labid = labpr;
                    labpr = rec->net->cfg->sym_in->ids[align->trans->in_id]->str;
                    //wtk_debug("labpr = %.*s\n", labpr->len, labpr->data);
                } else {
                    n = sprintf(buf, "s%d", al->state);
                    labid = wtk_heap_dup_string(rec->heap, buf, n);
                    //labid=wtk_label_find(label,buf,n,1)->name;
                    dur = (frame - al->frame) * frame_dur;
                    like = like - al->like;
                    frame = al->frame;
                }
                --i;
                la->lalign[i].state = al->state;
                la->lalign[i].name = labid;
                la->lalign[i].dur = dur;
                la->lalign[i].like = like;
                //printf("al%p: state = %d name = %s al->like = %f like = %f \n", al, al->state, labid->data, al->like, like);

                like = al->like;
            }
            if (pr) {
                if (nxt_path->prev) {
                    dur = (pr->frame - nxt_path->prev->frame) * frame_dur;
                    like = pr->like - nxt_path->prev->ac_like;
                } else {
                    dur = pr->frame * frame_dur;
                    like = pr->like;
                }
                --i;
                la->lalign[i].state = -1;
                la->lalign[i].name = labpr;
                la->lalign[i].dur = dur;
                la->lalign[i].like = like;
                //printf("state = %d labpr = %s like = %f\n", -1, labpr->data, like);
            }
        }

    }
}

void wtk_wfst_path_mark(wtk_wfst_path_t *path, int *nn, int *nl)
{
    wtk_wfst_nxtpath_t *nxt_path;

//  wtk_debug("nn=%d,nl=%d,usage=%d\n",*nn,*nl,path->usage);
//  wtk_debug("%p: frame = %d\n", path->prev, path->frame);
    if(path->usage < 0) {
        return;
    }
    path->usage = -(*nn)++;
    ++(*nl);
    //*nl=*nl+1;
    if(path->prev) {
        wtk_wfst_path_mark(path->prev, nn, nl);
    }
    for(nxt_path = path->nxt_chain; nxt_path; nxt_path = nxt_path->nxt_chain) {
        ++(*nl);
        //*nl=*nl+1;
        if(nxt_path->prev) {
            wtk_wfst_path_mark(nxt_path->prev, nn, nl);
        }
    }
}

wtk_lat_t* wtk_wfst_create_lat2(wtk_wfstr_t *rec, wtk_fst_tok_t *res)
{
//#define USE_PATH
    int nn, nl, ln;
    wtk_heap_t *heap = rec->heap;
    wtk_lat_t *lat;
    wtk_wfst_path_t *pth;
    wtk_wfst_path_t path;

    //path.like = res->like;
#ifdef USE_PATH
    path.ac_like = res->tok.path->ac_like;
    path.lm_like = res->tok.path->lm_like;
#else
    path.ac_like = res->ac_like;
    path.lm_like = res->lm_like;
#endif
    path.frame = rec->frame;
    //path.label=0;
    path.trans = 0;
    path.usage = 0;
    path.prev = res->path;
    path.align = 0;
    path.nxt_chain = 0;
    //wtk_debug("path->score=%f,like=%f\n",path.score,path.like);
    pth = &path;
    nn = 1;
    nl = 0;
    wtk_wfst_path_mark(pth, &nn, &nl);
    lat = wtk_lat_new_h(heap);
    wtk_lat_create(lat, nn, nl);
    lat->lmscale = rec->net->cfg->lmscale;
    lat->wdpenalty = rec->net->cfg->wordpen;
    lat->prscale = 1.0;
    lat->acscale = 1.0;
    lat->lnodes[0].time = -0.0001;
    lat->lnodes[0].info.fst_wrd = &null;
    lat->lnodes[0].score = 0;
    ln = -1;
    wtk_fst_rec_lat_from_path(rec, lat, pth, &ln);
    return lat;
}

wtk_lat_t* wtk_wfst_create_lat(wtk_wfstr_t *rec)
{
	wtk_fst_tok_t tokset;
    wtk_lat_t *lat;

    if(rec->best_final_tokset.path) {
        lat = wtk_wfst_create_lat2(rec, &(rec->best_final_tokset));
    } else {
        rec->is_forceout = 1;
        if(rec->cfg->use_forceout) {
        	//wtk_debug("[gen_max_tok.path=%p %d %d/%d]\n",rec->gen_max_tok.path, rec->gen_max_tok.path->trans->to_state->id, rec->gen_max_tok.path->trans->in_id, rec->gen_max_tok.path->trans->out_id);
        	if (rec->gen_max_tok.path){
        		tokset = rec->gen_max_tok;
        	}else
        		tokset = rec->exit_max_tok;
            lat = wtk_wfst_create_lat2(rec, &(tokset));
        } else {
            lat = 0;
        }
    }

    return lat;
}
#endif
//---------------------- print/debug section ---------------------------
int wtk_wfstr_bytes(wtk_wfstr_t *r)
{
	int bytes;

	bytes=sizeof(wtk_wfstr_t);
	if(r->inst_pool)
	{
		bytes+=wtk_vpool2_bytes(r->inst_pool);
	}
	wtk_debug("inst=%fMB\n",bytes*1.0/(1024*1024));
	bytes+=wtk_heap_bytes(r->heap);
	wtk_debug("inst=%fMB\n",bytes*1.0/(1024*1024));
	bytes+=wtk_vpool2_bytes(r->pth_pool);
	wtk_debug("inst=%fMB\n",bytes*1.0/(1024*1024));
	bytes+=wtk_vpool2_bytes(r->tokset_pool);
	wtk_debug("inst=%fMB\n",bytes*1.0/(1024*1024));
	if(r->cfg->use_lat && r->lat_net)
	{
		bytes+=wtk_fst_net3_bytes(r->lat_net);
	}
	if(r->net)
	{
		bytes+=wtk_fst_net_bytes(r->net);
	}
	return bytes;
}
#ifdef USE_ALIGN
void wtk_fst_align_print(wtk_wfst_align_t *a)
{
    if(a->prev) {
        wtk_fst_align_print(a->prev);
    }
    wtk_debug("state=%d frame=%d trans=%p %d:%d\n", a->state, a->frame, a->trans,
              a->trans->in_id, a->trans->out_id);
}
void wtk_fst_rec_print_align2(wtk_wfstr_t *r, wtk_wfst_path_t *pth, wtk_strbuf_t *buf)
{
    wtk_fst_net_cfg_t *cfg = r->net->cfg;
    wtk_string_t *v;

    //wtk_debug("id=%d\n",pth->trans->out_id);
    if(pth->prev) {
        wtk_fst_rec_print_align2(r, pth->prev, buf);
    }
    if(pth->trans) { // && pth->trans->out_id>0)
        if(buf->pos > 0) {
            wtk_strbuf_push_c(buf, ' ');
        }
        wtk_debug("align=%p\n", pth->align);
        wtk_fst_align_print(pth->align);
        //wtk_debug("[%d=%.*s]\n",cfg->sym_in->nid,cfg->sym_in->ids[0]->str->len,cfg->sym_in->ids[0]->str->data);
        if(pth->trans->in_id > 0) { // && 0)
            v = cfg->sym_in->ids[pth->trans->in_id]->str;
            wtk_strbuf_push(buf, v->data, v->len);
            wtk_strbuf_push_c(buf, ':');
        }
        v = cfg->sym_out->strs[pth->trans->out_id];
        wtk_strbuf_push(buf, v->data, v->len);
    }
}

void wtk_fst_rec_print_align(wtk_wfstr_t *r, wtk_wfst_path_t *pth)
{
    wtk_strbuf_t *buf = r->buf;

    //wtk_debug("============== score=%f =====================\n",pth->ac_like);
    wtk_strbuf_reset(buf);
    wtk_fst_rec_print_align2(r, pth, buf);
    wtk_debug("path[%d]:[%.*s]\n", r->frame, buf->pos, buf->data);
}
#endif
int wtk_wfstr_print(wtk_wfstr_t *r)
{
	int bytes;
	int t;

	wtk_debug("==================== rec mem =====================\n");
	bytes=sizeof(wtk_wfstr_t);
	if(r->inst_pool)
	{
		t=wtk_vpool2_bytes(r->inst_pool);
		bytes+=t;
		//wtk_debug("type=%d inst=%fMB use=%d free=%d\n",r->inst_pool->type,t*1.0/(1024*1024),r->inst_pool->hoard.use_length,r->inst_pool->hoard.cur_free);
		wtk_debug("inst=%fMB use=%d free=%d\n",t*1.0/(1024*1024),r->inst_pool->cache->used,r->inst_pool->cache->nslot);
	}
	t=wtk_vpool2_bytes(r->pth_pool);
	bytes+=t;
	//wtk_debug("path=%fMB use=%d free=%d\n",t*1.0/(1024*1024),r->pth_pool->hoard.use_length,r->pth_pool->hoard.cur_free);
	wtk_debug("pth=%fMB use=%d free=%d\n",t*1.0/(1024*1024),r->pth_pool->cache->used,r->pth_pool->cache->nslot);
	t=wtk_vpool2_bytes(r->tokset_pool);
	bytes+=t;
	//wtk_debug("tok=%fMB,use=%d,cur_free=%d\n",t*1.0/(1024*1024),r->tokset_pool->hoard.use_length,r->tokset_pool->hoard.cur_free);
	wtk_debug("tok=%fMB use=%d free=%d\n",t*1.0/(1024*1024),r->tokset_pool->cache->used,r->tokset_pool->cache->nslot);
	if(r->cfg->use_lat && r->lat_net)
	{
		t=wtk_fst_net3_bytes(r->lat_net);
		bytes+=t;
		wtk_debug("latnet=%fMB\n",t*1.0/(1024*1024));
	}
	if(r->net)
	{
		bytes+=wtk_fst_net_print(r->net);
	}
	t=wtk_heap_bytes(r->heap);
	bytes+=t;
	wtk_debug("heap=%fMB\n",t*1.0/(1024*1024));
	wtk_debug("tot=%fMB\n",bytes*1.0/(1024*1024));
	wtk_debug("==============================================\n");
	return bytes;
}


void wtk_wfstr_print_path2(wtk_wfstr_t *r,wtk_wfst_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_fst_net_cfg_t *cfg=r->net_cfg;//r->net->cfg;
	wtk_string_t *v;

	//wtk_debug("id=%d\n",pth->trans->out_id);
	if(pth->prev)
	{
		wtk_wfstr_print_path2(r,pth->prev,buf);
	}
	if(pth->trans)// &&  pth->trans->out_id>0)
	{
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}

		//wtk_debug("[%d=%.*s]\n",cfg->sym_in->nid,cfg->sym_in->ids[0]->str->len,cfg->sym_in->ids[0]->str->data);
		if(pth->trans->in_id>0)// 0)
		{
			if(r->net->print && r->net->print->get_insym)
			{
				v=r->net->print->get_insym(r->net->print->ths,pth->trans->in_id);
			}else
			{
				v=cfg->sym_in->ids[pth->trans->in_id]->str;
			}
			wtk_strbuf_push(buf,v->data,v->len);
			//wtk_strbuf_push_f(buf,"-%d",pth->trans->to_state->id);
			wtk_strbuf_push_c(buf,':');
		}
		//wtk_debug("frame=%d in=%d/%d ac=%f lm=%f raw=%f\n",pth->frame,pth->trans->in_id,pth->trans->out_id,pth->ac_like,pth->lm_like,pth->raw_like);
		//v=cfg->sym_out->strs[pth->trans->out_id];
		//wtk_debug("print=%p / %p\n",r->net->print,cfg->sym_out->strs);
		//wtk_debug("print=%p/%p\n",r->net->print,r->net->print->ths);
		if(r->net->print)
		{
			v=r->net->print->get_outsym(r->net->print->ths,pth->trans->out_id);
		}else
		{
			v=cfg->sym_out->strs[pth->trans->out_id];
		}
		//wtk_debug("id=%d v=%p\n",pth->trans->out_id,v);
		wtk_strbuf_push(buf,v->data,v->len);
		//wtk_strbuf_push_f(buf,"[f=%d to=%d arc=%d/%d]",pth->frame,pth->trans->to_state->id,r->net_cfg->pstate_in[pth->trans->to_state->id],pth->trans->to_state->ntrans);
		//k_debug("v[%d]=%d\n",pth->trans->to_state->id,r->net_cfg->pstate_in[pth->trans->to_state->id]);
		wtk_strbuf_push_f(buf,"[frame=%d ac=%f like=%f]",pth->frame,pth->ac_like,pth->raw_like);
		//wtk_strbuf_push_f(buf,"[frame=%d ac=%f to=%d]",pth->frame,pth->ac_like,pth->trans->to_state->id);
		//wtk_strbuf_push_f(buf,"[frame=%d like=%f/%f in=%d]",pth->frame,pth->ac_like,pth->lm_like,pth->trans->in_id);
		//wtk_strbuf_push_f(buf,"[frame=%d ac=%f like=%f hook=%p pth=%p]",pth->frame,pth->ac_like,pth->like,pth->hook,pth);
		//wtk_strbuf_push_f(buf,"[%d:%d]",pth->trans->in_id,pth->trans->out_id);
		/*
		wtk_strbuf_push_f(buf,"[f=%d hook=%p id=%d trans=%p pth=%p s=%f a=%f l=%f to=%d]",pth->frame,pth->hook,
				pth->trans->out_id,pth->trans,pth,pth->raw_like,pth->ac_like,pth->lm_like,
				pth->trans->to_state->id);
		*/
		/*
		wtk_strbuf_push_f(buf,"[f=%d hook=%p id=%d trans=%p pth=%p]",pth->frame,pth->hook,
				pth->trans->out_id,pth->trans,pth);
		*/
		/*
		wtk_strbuf_push_f(buf,"[f=%d hook=%p id=%d trans=%p pth=%p s=%f a=%f l=%f]",pth->frame,pth->hook,
				pth->trans->out_id,pth->trans,pth,pth->raw_like,pth->ac_like,pth->lm_like);
		*/
	}else
	{
		//wtk_strbuf_push_f(buf," null[frame=%d like=%f/%f]",pth->frame,pth->ac_like,pth->lm_like);
	}
}

void wtk_wfstr_print_path(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	wtk_strbuf_t *buf=r->buf;

	//wtk_debug("print path=%p\n",pth);
	if(!pth){return;}
	//wtk_debug("============== score=%f =====================\n",pth->ac_like);
	wtk_strbuf_reset(buf);
	wtk_wfstr_print_path2(r,pth,buf);
	//wtk_debug("path[%d ac=%f/%f]:[%.*s]\n",r->frame,pth->ac_like,pth->raw_like,buf->pos,buf->data);
	//wtk_debug("===================================\n");
}

void wtk_wfstr_print_path_mlf2(wtk_wfstr_t *r,wtk_wfst_path_t *pth,wtk_wfst_path_t *wrd)
{
	wtk_string_t *v;
	int s,e;

	if(!pth)
	{
		if(r->net->print)
		{
			v=r->net->print->get_outsym(r->net->print->ths,wrd->trans->out_id);
		}else
		{
			v=r->net_cfg->sym_out->strs[wrd->trans->out_id];
		}
		s=0;
		e=wrd->frame;
		//毫秒　10纳秒
		printf("%d00000 %d00000 %.*s\n",s,e,v->len,v->data);
		//exit(0);
		return;
	}
	if(pth->trans->out_id>0)
	{
		if(r->net->print)
		{
			v=r->net->print->get_outsym(r->net->print->ths,wrd->trans->out_id);
		}else
		{
			v=r->net_cfg->sym_out->strs[wrd->trans->out_id];
		}
		wtk_wfstr_print_path_mlf2(r,pth->prev,pth);
		//wtk_debug("%.*s\n",v->len,v->data);
		s=pth->frame;
		e=wrd->frame;
		//毫秒　纳秒
		printf("%d00000 %d00000 %.*s\n",s,e,v->len,v->data);
	}else
//	}else if(pth->trans->in_id>0)
//	{
//		if(r->net->print && r->net->print->get_insym)
//		{
//			v=r->net->print->get_insym(r->net->print->ths,pth->trans->in_id);
//		}else
//		{
//			v=r->net_cfg->sym_in->ids[pth->trans->in_id]->str;
//		}
//		//wtk_debug("%d=[%.*s]\n",pth->frame,v->len,v->data);
//		//if(wtk_string_cmp_withstart_s(v,"sil")==0)
//		if(wtk_string_cmp_s(v,"sil")==0)
//		{
//			wtk_wfstr_print_path_mlf2(r,pth->prev,pth);
//			//wtk_debug("%.*s\n",v->len,v->data);
//			s=pth->frame;
//			e=wrd->frame;
//			//毫秒　纳秒
//			printf("%d00000 %d00000 %.*s\n",s,e,v->len,v->data);
//		}else
//		{
//			wtk_wfstr_print_path_mlf2(r,pth->prev,wrd);
//		}
//	}else
	{
		wtk_wfstr_print_path_mlf2(r,pth->prev,wrd);
	}
}

void wtk_wfstr_print_path_mlf(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	if(pth)
	{
		wtk_wfstr_print_path_mlf2(r,pth->prev,pth);
	}else
	{
		printf("\n");
	}

	//exit(0);
}

void wtk_wfstr_print_path_phn_mlf2(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	int s,e;
	int in,out;
	wtk_string_t *v1=NULL,*v2=NULL;

	if(pth->prev)
	{
		wtk_wfstr_print_path_phn_mlf2(r,pth->prev);
		s=pth->prev->frame;
	}else
	{
		s=0;
	}
	e=pth->frame;
	in=pth->trans->in_id;
	out=pth->trans->out_id;
	if(in>0)
	{
		if(r->net->print && r->net->print->get_insym)
		{
			v1=r->net->print->get_insym(r->net->print->ths,in);
		}else
		{
			v1=r->net_cfg->sym_in->ids[in]->str;
		}
	}
	if(out>0)
	{
		if(r->net->print)
		{
			v2=r->net->print->get_outsym(r->net->print->ths,out);
		}else
		{
			v2=r->net_cfg->sym_out->strs[out];
		}
	}
	//毫秒　纳秒
	if(v1||v2)
	{
		if(v1)
		{
			printf("%d00000 %d00000 %.*s",s,e,v1->len,v1->data);
		}
		if(v2)
		{
			printf(" %.*s",v2->len,v2->data);
		}
		printf("\n");
	}
}

void wtk_wfstr_print_path_phn_mlf(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	if(pth)
	{
		wtk_wfstr_print_path_phn_mlf2(r,pth);
	}else
	{
		printf("\n");
	}
}

void wtk_wfstr_print_path4(wtk_wfstr_t *r,wtk_wfst_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_fst_net_cfg_t *cfg=r->net->cfg;
	wtk_string_t *v;

	//wtk_debug("id=%d\n",pth->trans->out_id);
	if(pth->prev)
	{
		wtk_wfstr_print_path4(r,pth->prev,buf);
	}
	if(pth->trans && pth->trans->out_id>0)
	{
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}

		/*
		v=cfg->sym_in->ids[pth->trans->in_id]->str;
		wtk_strbuf_push(buf,v->data,v->len);
		wtk_strbuf_push_c(buf,':');
		*/

		//v=cfg->sym_out->strs[pth->trans->out_id];
		if(r->net->print)
		{
			v=r->net->print->get_outsym(r->net->print->ths,pth->trans->out_id);
		}else
		{
			v=cfg->sym_out->strs[pth->trans->out_id];
		}
		wtk_strbuf_push(buf,v->data,v->len);
		//wtk_strbuf_push_f(buf,"[%d]",pth->trans->to_state->id);
		//wtk_strbuf_push_f(buf,"[f=%d to=%d w=%f]",pth->frame,pth->trans->to_state->id,pth->trans->weight);
		/*
		wtk_strbuf_push_f(buf,"[f=%d hook=%p id=%d trans=%p pth=%p s=%f a=%f l=%f to=%d]",pth->frame,pth->hook,
				pth->trans->out_id,pth->trans,pth,pth->raw_like,pth->ac_like,pth->lm_like,
				pth->trans->to_state->id);
		*/

		/*
		wtk_strbuf_push_f(buf,"[f=%d hook=%p id=%d trans=%p pth=%p]",pth->frame,pth->hook,
				pth->trans->out_id,pth->trans,pth);
		*/
		/*
		wtk_strbuf_push_f(buf,"[f=%d hook=%p id=%d trans=%p pth=%p s=%f a=%f l=%f]",pth->frame,pth->hook,
				pth->trans->out_id,pth->trans,pth,pth->raw_like,pth->ac_like,pth->lm_like);
		*/
	}
}

void wtk_wfstr_print_path3(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	wtk_strbuf_t *buf=r->buf;

	//wtk_debug("============== score=%f =====================\n",pth->ac_like);
	if(!pth)
	{
		wtk_debug("pth=nil\n");
		return;
	}
	wtk_strbuf_reset(buf);
	wtk_wfstr_print_path4(r,pth,buf);
	wtk_debug("path[%d]:[%.*s]\n",r->frame,buf->pos,buf->data);
	//wtk_debug("===================================\n");
}

int wtk_wfstr_path_is(wtk_wfstr_t *r,wtk_wfst_path_t *pth,char *data,int bytes)
{
	wtk_strbuf_t *buf=r->buf;

	//wtk_debug("============== score=%f =====================\n",pth->ac_like);
	wtk_strbuf_reset(buf);
	wtk_wfstr_print_path2(r,pth,buf);
	if(buf->pos>bytes)
	{
		buf->pos=bytes;
	}
	if(wtk_str_equal(buf->data,buf->pos,data,bytes))
	{
		return 1;
	}else
	{
		return 0;
	}
}

void wtk_wfstr_print_tok(wtk_wfstr_t *r,wtk_fst_tok_t *tok)
{
	wtk_strbuf_t *buf=r->buf;

	//wtk_debug("================= tok=%p ============\n",tok);
	printf("------------- tok=%p ------------\n",tok);
	printf("score=%f\n",tok->like);
	if(tok->path)
	{
		wtk_strbuf_reset(buf);
		wtk_wfstr_print_path2(r,tok->path,buf);
		printf("path=[%.*s]\n",buf->pos,buf->data);
	}else
	{
		printf("path=nil\n");
	}
}



void wtk_wfstr_print_inst2(wtk_wfstr_t *r,wtk_fst_inst_t *inst)
{
	int i;

	wtk_debug("============ inst=%p =============\n",inst);
	//printf("from=%d;to=%d\n",inst->trans->from_state->id,inst->trans->to_state->id);
	for(i=1;i<=r->hmmset->max_hmm_state;++i)
	{
		printf("tokset[%d/%d]:\n",i,r->hmmset->max_hmm_state);
		//wtk_wfstr_print_tokset(r,inst->states+i);
	}
}

void wtk_wfstr_print_final(wtk_wfstr_t *r)
{
	wtk_wfst_path_t * pth;

	pth=wtk_wfstr_get_path(r);
	if(pth)
	{
		wtk_wfstr_print_path(r,pth);
	}
}

wtk_wfst_path_t* wtk_wfstr_dup_path2(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	wtk_wfst_path_t *pth2;

	pth2=(wtk_wfst_path_t *)wtk_vpool2_pop(r->pth_pool);
	*pth2=*pth;
	return pth2;
}


wtk_wfst_path_t *wtk_wfstr_dup_path(wtk_wfstr_t *r,wtk_wfst_path_t *src)
{
	wtk_wfst_path_t *pth;

	pth=(wtk_wfst_path_t*)wtk_vpool2_pop(r->pth_pool);
	*pth=*src;
	//pth->nxt_chain=0;
	if(src->prev)
	{
		pth->prev=wtk_wfstr_dup_path(r,src->prev);
	}
	return pth;
}

void wtk_wfstr_print_inst(wtk_wfstr_t *r)
{
	wtk_queue_node_t *n;
	wtk_fst_inst_t *inst;
	int i;

	for(i=0,n=r->inst_q.pop;n;n=n->next,++i)
	{
		inst=data_offset2(n,wtk_fst_inst_t,inst_n);
		//wtk_debug("v[%d]=[%d],score=%f\n",i,inst->trans->to_state,inst->states[1].score);
		wtk_debug("v[%d],score=%f\n",i,inst->states[1].like);
	}
}

void wtk_wfstr_print_inst_queue(wtk_queue_t *q)
{
	wtk_queue_node_t *n;
	wtk_fst_inst_t *inst;
	int i;

	for(i=0,n=q->pop;n;n=n->next,++i)
	{
		inst=data_offset2(n,wtk_fst_inst_t,inst_n);
		//wtk_debug("v[%d]=[%d],score=%f\n",i,inst->trans->to_state,inst->states[1].score);
		wtk_debug("v[%d],score=%f\n",i,inst->states[1].like);
	}
}

void wtk_wfstr_trace_path(wtk_wfstr_t *r,wtk_wfst_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_fst_net_cfg_t *cfg=r->net->cfg;
	wtk_string_t *v;

	wtk_strbuf_reset(buf);
	while(pth)
	{
		v=cfg->sym_out->strs[pth->trans->out_id];//label];
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		wtk_strbuf_push(buf,v->data,v->len);
		pth=pth->prev;
	}
}

void wtk_wfstr_expand_trans(wtk_wfstr_t* r,wtk_fst_tok_t *tokset,wtk_fst_trans_t *to_trans)
{
	wtk_fst_net_nwlp_t *nwlp=r->net->cfg->nwlp;
	int eps_id=r->net->cfg->eps_id;
	wtk_fst_tok_t *dup_tok,*tok;
	wtk_fst_tok_t dup_tokset;
	wtk_fst_trans_t *trans;
	wtk_fst_state_t *state;
	double score;
	int i;
	float pure_lm;

	dup_tok=&(dup_tokset);
	tok=tokset;
	state=to_trans->to_state;
	if(state->type==WTK_FST_TOUCH_STATE)
	{
		//wtk_debug("load %d\n",state->id);
		wtk_fst_net_load_state(r->net,state);
	}
	for(i=0,trans=state->v.trans;i<state->ntrans;++i,++trans)
	{
		if(trans->in_id==eps_id && trans->out_id==eps_id)
		{
			score=tok->like+trans->weight;//+trans->to_state->nwlmp;
			if(nwlp)
			{
				score+=nwlp->probs[trans->to_state->id];
			}
			dup_tokset=*tokset;
			dup_tok->like=score;
#ifdef USE_EXT_LIKE
			dup_tok->raw_like+=trans->weight;
#endif
			if(trans->out_id>0)
			{
				pure_lm=trans->weight-r->net->cfg->wordpen;
			}else
			{
				pure_lm=trans->weight;
			}
			dup_tok->lm_like+=pure_lm;
			wtk_wfstr_add_path(r,&dup_tokset,trans);
			//wtk_debug("attach state\n");
			wtk_wfstr_attach_state(r,&dup_tokset,trans,trans->to_state);
			wtk_wfstr_expand_trans(r,&dup_tokset,trans);//,entry_tokset);
		}
	}
}

wtk_wfst_path_t* wtk_wfstr_path_prev_wrd_path(wtk_wfstr_t *rec,wtk_wfst_path_t *pth)
{
	if(!pth->prev)
	{
		return NULL;
	}
	if(pth->prev && pth->prev->trans->out_id!=rec->net_cfg->eps_id)
	{
		return pth->prev;
	}
	return wtk_wfstr_path_prev_wrd_path(rec,pth->prev);
}

void wtk_wfstr_link_path_in_lat(wtk_wfstr_t *r,wtk_wfst_path_t *pth,
		wtk_wfst_path_t *prev,wtk_fst_net3_t *net)
{
	float arc_like,ac_like,lm_like;
	wtk_fst_node_t *from_node,*node;
	wtk_fst_arc_t *arc;

	if(pth->prev!=prev)
	{
		wtk_wfstr_link_path_in_lat(r,pth->prev,prev,net);
	}
	if(!prev->hook){return;}
	if(pth==prev)
	{
		wtk_debug("found dup\n");
		exit(0);
	}
	from_node=wtk_fst_path_get_lat_node(prev);
	//wtk_debug("=====================================\n");
	//wtk_wfstr_print_path(r,pth);
	//wtk_wfstr_print_path(r,prev);
	node=wtk_fst_net3_get_trans_node(net,pth->trans,pth->frame);
	ac_like=pth->ac_like-prev->ac_like;
	lm_like=pth->lm_like-prev->lm_like;
#ifdef USE_EXT_LIKE
	arc_like=pth->raw_like-prev->raw_like;
#else
	arc_like=pth->ac_like+pth->lm_like-prev->ac_like-prev->lm_like;
#endif
	arc=wtk_fst_net3_pop_arc(net,from_node,node,pth->trans,arc_like,ac_like,lm_like,r->frame);
	wtk_fst_arc_link_path(arc,pth);
}

void wtk_wfstr_compose_expand(wtk_wfstr_t *r,wtk_fst_net3_t *net)
{
	wtk_queue_node_t *qn;
	wtk_fst_inst_t *inst;
	wtk_fst_tok_t *tokset;
	wtk_wfst_path_t *pth,*prev;
	wtk_fst_node_t *eof;
	int i;

	eof=net->null_prev_node;
	if(!eof){return;}
	//remove prev node;
	/*
	wtk_fst_net_remove_node_from_prev(net,n);
	wtk_fst_net3_push_node(net,n);
	net->null_prev_node=NULL;
	*/
	for(qn=r->inst_q.pop;qn;qn=qn->next)
	{
		inst=data_offset(qn,wtk_fst_inst_t,inst_n);
		for(i=1;i<=r->hmmset->max_hmm_state;++i)
		{
			tokset=inst->states+i;
			pth=tokset->path;
			if(!pth)// || pth->hook)
			{
				continue;
			}
			if(pth->hook)
			{
				/*
				if(pth->hook!=eof)
				{
					continue;
				}*/
				continue;
			}else
			{
				if(pth->trans->out_id>0)
				{
					continue;
				}
				prev=wtk_wfstr_path_prev_wrd_path(r,pth);
				if(!prev || prev->hook!=eof)
				{
					continue;
				}
				//wtk_wfstr_print_path(r,pth);
				//wtk_wfstr_print_path(r,prev);
				wtk_wfstr_link_path_in_lat(r,pth,prev,net);
				//exit(0);
			}
		}
	}
}

void wtk_wfstr_get_mono_phn(wtk_wfstr_t *r,wtk_string_t *v,wtk_string_t *to)
{
typedef enum
{
	WTK_PHMM_INIT,
	WTK_PHMM_MONO,
}wtk_phmm_state_t;
	char *s,*e;
	wtk_phmm_state_t state;

	wtk_string_set(to,0,0);
	s=v->data;e=s+v->len;
	state=WTK_PHMM_INIT;
	while(s<e)
	{
		switch(state)
		{
		case WTK_PHMM_INIT:
			if(*s=='-')
			{
				to->data=s+1;
				state=WTK_PHMM_MONO;
			}
			break;
		case WTK_PHMM_MONO:
			if(*s=='+')
			{
				to->len=s-to->data;
				//wtk_debug("len=%d\n",to->len);
				s=e;
			}
			break;
		default:
			break;
		}
		++s;
	}
#ifndef USE_X
	if(to->len==0 && (wtk_string_cmp_s(v,"sil")!=0))
	{
		*to=*v;
	}
#endif
}

void wtk_wfstr_get_phn_path(wtk_wfstr_t *r,wtk_wfst_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_fst_net_cfg_t *cfg=r->net->cfg;
	wtk_string_t *v;
	wtk_string_t x;

	if(pth->prev)
	{
		wtk_wfstr_get_phn_path(r,pth->prev,buf);
	}
	if(pth->trans)
	{
		if(pth->trans->in_id>0)
		{
			v=cfg->sym_in->ids[pth->trans->in_id]->str;
			//wtk_debug("[%.*s]\n",v->len,v->data);
			wtk_wfstr_get_mono_phn(r,v,&x);
			//x=*v;
			//wtk_debug("[%.*s]\n",x.len,x.data);
			if(x.len>0)
			{
				if(buf->pos>0)
				{
					wtk_strbuf_push_c(buf,' ');
					//wtk_strbuf_push_c(buf,'\n');
				}
				wtk_strbuf_push(buf,x.data,x.len);
			}
		}
	}
}

wtk_string_t* wtk_wfstr_get_phn_str(wtk_wfstr_t *rec)
{
	wtk_wfst_path_t *pth;
	wtk_strbuf_t *buf=rec->buf;
	wtk_string_t *v;

	pth=wtk_wfstr_get_path(rec);
	if(pth)
	{
		wtk_strbuf_reset(buf);
		wtk_wfstr_get_phn_path(rec,pth,buf);
		v=wtk_heap_dup_string(rec->heap,buf->data,buf->pos);
	}else
	{
		v=NULL;
	}
	return v;
}

#ifdef USE_ALIGN
void wtk_wfst_align_print(wtk_wfst_align_t *a)
{
	if(a->prev)
	{
		wtk_wfst_align_print(a->prev);
	}
	wtk_debug("state=%d frame=%d trans=%p %d:%d\n",a->state,a->frame,a->trans,
			a->trans->in_id,a->trans->out_id);
}

void wtk_wfstr_print_align2(wtk_wfstr_t *r,wtk_wfst_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_fst_net_cfg_t *cfg=r->net->cfg;
	wtk_string_t *v;

	//wtk_debug("id=%d\n",pth->trans->out_id);
	if(pth->prev)
	{
		wtk_wfstr_print_align2(r,pth->prev,buf);
	}
	if(pth->trans)// && pth->trans->out_id>0)
	{
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		//wtk_debug("align=%p\n",pth->align);
		//wtk_fst_align_print(pth.->align);
		//wtk_debug("[%d=%.*s]\n",cfg->sym_in->nid,cfg->sym_in->ids[0]->str->len,cfg->sym_in->ids[0]->str->data);
		if(pth->trans->in_id>0)// && 0)
		{
			v=cfg->sym_in->ids[pth->trans->in_id]->str;
			wtk_strbuf_push(buf,v->data,v->len);
			wtk_strbuf_push_c(buf,':');
		}
		v=cfg->sym_out->strs[pth->trans->out_id];
		wtk_strbuf_push(buf,v->data,v->len);
	}
}

void wtk_wfstr_print_align(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	wtk_strbuf_t *buf=r->buf;

	//wtk_debug("============== score=%f =====================\n",pth->ac_like);
	wtk_strbuf_reset(buf);
	wtk_wfstr_print_align2(r,pth,buf);
	//wtk_debug("path[%d]:[%.*s]\n",r->frame,buf->pos,buf->data);
}

void wtk_wfstr_align_to_trans(wtk_wfstr_t *r,wtk_heap_t *heap,
		wtk_wfst_path_t *pth,
		wtk_wfst_align_t *nxt,wtk_wfst_align_t *a,wtk_lablist_t *lab_list,
		float frame_scale)
{
	static wtk_string_t strs[]=
	{
			wtk_string("s1"),
			wtk_string("s2"),
			wtk_string("s3"),
			wtk_string("s4"),
			wtk_string("s5"),
	};
	wtk_lab_t *lab;
	float score;
	int idx;
	wtk_fst_trans_t *trans;

	if(a->prev)
	{
		wtk_wfstr_align_to_trans(r,heap,pth,a,a->prev,lab_list,frame_scale);
	}
	if(!nxt)
	{
		return;
	}
	if(a->state==-1)
	{
		return;
	}
	score=nxt->like-a->like;
	//wtk_debug("like=%f/%f/%f\n",a->like,nxt->like,score);
	lab=wtk_lab_new_h(heap,lab_list->max_aux_lab);
	lab->score=score;
	lab->start=a->frame*frame_scale;
	lab->end=nxt->frame*frame_scale;
	if(a->state==-1)
	{
		idx=4;
	}else
	{
		idx=a->state-1;
	}
	lab->name=strs+idx;
	trans=a->trans;
	if(!a->prev)
	{
		if(trans->in_id>0)
		{
			lab->aux_lab[1]=r->net_cfg->sym_in->ids[trans->in_id]->str;
		}
		if(trans->in_id>0)
		{
			lab->aux_lab[2]=r->net_cfg->sym_out->strs[trans->out_id];
		}
		if(pth->prev)
		{
			score=pth->ac_like-pth->prev->ac_like;
		}else
		{
			score=pth->ac_like;
		}
		lab->aux_score[1]=score;
	}
	wtk_queue_push(&(lab_list->lable_queue),&(lab->lablist_n));
	//exit(0);
}


wtk_wfst_align_t* wtk_fst_path_head_align(wtk_wfst_path_t *pth)
{
	wtk_wfst_align_t *a;

	a=pth->align;
	while(a)
	{
		if(a->prev)
		{
			a=a->prev;
		}else
		{
			break;
		}
	}
	return a;
}

void wtk_wfstr_pth_to_lab(wtk_wfstr_t *r,wtk_heap_t *heap,
		wtk_wfst_path_t *pth,
		wtk_lablist_t *lab_list,
		float frame_scale)
{
	wtk_lab_t *lab;
	float score;
	wtk_fst_trans_t *trans;


	lab=wtk_lab_new_h(heap,lab_list->max_aux_lab);
	lab->score=0;
	if(pth->prev)
	{
		lab->start=pth->prev->frame*frame_scale;
	}else
	{
		lab->start=0;
	}
	lab->end=pth->frame*frame_scale;
	trans=pth->trans;
	if(trans->in_id>0)
	{
		lab->name=r->net_cfg->sym_in->ids[trans->in_id]->str;
	}
	//wtk_debug("pth=%p prev=%p\n",pth,pth->prev);
	//if(!pth->prev)
	{
//		if(trans->in_id>0)
//		{
//			//lab->aux_lab[1]=r->net_cfg->sym_in->ids[trans->in_id]->str;
//		}
		if(trans->out_id>0)
		{
			if(r->net->print)
			{
				lab->aux_lab[2]=r->net->print->get_outsym(r->net->print->ths,trans->out_id);
			}else
			{
				lab->aux_lab[2]=r->net_cfg->sym_out->strs[trans->out_id];
			}
		}
		if(pth->prev)
		{
			score=pth->ac_like-pth->prev->ac_like;
		}else
		{
			score=pth->ac_like;
		}
		lab->aux_score[1]=score;
		lab->score=score;
	}
	wtk_queue_push(&(lab_list->lable_queue),&(lab->lablist_n));
	//exit(0);
}

void wtk_wfstr_pth_to_trans(wtk_wfstr_t *r,wtk_heap_t *heap,
		wtk_wfst_path_t *nxt,
		wtk_wfst_path_t *pth,wtk_lablist_t *lab_list,float frame_scale)
{
	wtk_wfst_align_t *a;

	if(pth->prev)
	{
		wtk_wfstr_pth_to_trans(r,heap,pth,pth->prev,lab_list,frame_scale);
	}
	if(r->align_pool && pth->align)
	{
		if(nxt)
		{
			a=wtk_fst_path_head_align(nxt);
		}else
		{
			a=NULL;
		}
		//wtk_debug("score=%f\n",score);
		wtk_wfstr_align_to_trans(r,heap,pth,a,pth->align,lab_list,frame_scale);
	}else
	{
		wtk_wfstr_pth_to_lab(r,heap,pth,lab_list,frame_scale);
	}
}

void wtk_wfstr_print_path_lab(wtk_wfstr_t *r,wtk_wfst_path_t *pth)
{
	wtk_fst_net_cfg_t *cfg=r->net->cfg;
	wtk_string_t *v,*v2;

	//wtk_debug("id=%d\n",pth->trans->out_id);
	if(pth->prev)
	{
		wtk_wfstr_print_path_lab(r,pth->prev);
	}
	if(pth->trans)
	{
		v=cfg->sym_out->strs[pth->trans->out_id];
		v2=cfg->sym_in->ids[pth->trans->in_id]->str;
		printf("%d %.*s:%.*s %f\n",pth->frame,v2->len,v2->data,v->len,v->data,pth->ac_like);
	}
}

wtk_transcription_t* wtk_wfstr_get_trans(wtk_wfstr_t *r,float frame_scale)
{
	wtk_wfst_path_t *pth;
	wtk_heap_t *heap=r->heap;
	wtk_transcription_t *trans=NULL;
	wtk_lablist_t *lab;
	//float frame_scale;

	//wtk_fst_rec_print_final(r);
	//frame_scale=0.02*1.0E07;
	pth=wtk_wfstr_get_path(r);
	//wtk_debug("%p:%d pth=%p pth=%p\n",r,r->frame,pth,r->best_final_tokset.tok.path);
	if(!pth){goto end;}
	trans=wtk_transcription_new_h(heap);
	if(!r->best_final_tokset.path)
	{
		trans->forceout=1;
	}
	lab=wtk_lablist_new_h(heap,2);
	wtk_queue_push(&(trans->lab_queue),&(lab->trans_n));
	wtk_wfstr_pth_to_trans(r,heap,NULL,pth,lab,frame_scale);
end:
	/*
	if(r->cfg->model && !r->cfg->state)
	{
		wtk_fst_rec_print_path_lab(r,pth);
	}*/

	return trans;
}
#endif

void wtk_wfstr_touch_end(wtk_wfstr_t *rec)
{
	if(rec->cfg->use_lat)
	{
		rec->lat_net->end_ac_like=rec->best_final_tokset.ac_like;
		rec->lat_net->end_lm_like=rec->best_final_tokset.lm_like;
	}
	rec->conf=wtk_wfstr_get_conf(rec);
}

int wtk_fst_path_nword(wtk_wfst_path_t *pth)
{
	int cnt;

	cnt=0;
	while(pth)
	{
		if(pth->trans->out_id>0)
		{
			++cnt;
		}
		pth=pth->prev;
	}
	return cnt;
}

int wtk_fst_path_nphn(wtk_wfst_path_t *pth)
{
	int cnt;

	cnt=0;
	while(pth)
	{
		if(pth->trans->in_id>0)
		{
			++cnt;
		}
		pth=pth->prev;
	}
	return cnt;
}





