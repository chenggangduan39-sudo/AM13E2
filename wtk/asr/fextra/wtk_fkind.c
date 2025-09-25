#include "wtk_fkind.h"

static wtk_string_t pmkmap[] =
{
		wtk_string("WAVEFORM"),
		wtk_string("LPC"),
		wtk_string("LPREFC"),
		wtk_string("LPCEPSTRA"),
		wtk_string("LPDELCEP"),
		wtk_string("IREFC"),
		wtk_string("MFCC"),
		wtk_string("FBANK"),
		wtk_string("MELSPEC"),
		wtk_string("USER"),
		wtk_string("DISCRETE"),
		wtk_string("PLP"),
		//wtk_string("ANON")
};

int wtk_fkind_from_string(wtk_fkind_t* p,char* d,int dl)
{
	unsigned char hasE,hasD,hasN,hasA,hasT,hasF,hasC,hasK,hasZ,has0,hasV,found;
	char *s;
	int ret,len=dl;
	wtk_fkind_t i=-1;
	wtk_string_t bps;
	int j;

	//wtk_debug("[%.*s]\n",dl,d);
	s=d+len-2;ret=-1;
	hasV=hasE=hasD=hasN=hasA=hasT=hasF=hasC=hasK=hasZ=has0=0;
	while(len>2 && *s=='_')
	{
		switch(*(s+1))
		{
	      case 'E': hasE = 1; break;
	      case 'D': hasD = 1; break;
	      case 'N': hasN = 1; break;
	      case 'A': hasA = 1; break;
	      case 'C': hasC = 1; break;
	      case 'T': hasT = 1; break;
	      case 'F': hasF = 1; break;
	      case 'K': hasK = 1; break;
	      case 'Z': hasZ = 1; break;
	      case '0': has0 = 1; break;
	      case 'V': hasV = 1; break;
	      default:	goto end;
		}
		len-=2;s-=2;
	}
	found=0;
	bps.data=d;bps.len=len;
	for(j=0;j<sizeof(pmkmap)/sizeof(wtk_string_t);++j)
	{
		wtk_string_t *str=&(pmkmap[++i]);
		if(wtk_string_equal(str,&(bps)))
		{
			found=1;break;
		}
	}
	//wtk_debug("i=%d\n",i);
	if(!found){goto end;}
	if (i == LPDELCEP)
	{
		/* for backward compatibility with V1.2 */
		i = LPCEPSTRA | HASDELTA;
	}
	if (hasE) i |= HASENERGY;
	if (hasD) i |= HASDELTA;
	if (hasN) i |= HASNULLE;
	if (hasA) i |= HASACCS;
	if (hasT) i |= HASTHIRD;
	if (hasK) i |= HASCRCC;
	if (hasC) i |= HASCOMPX;
	if (hasZ) i |= HASZEROM;
	if (has0) i |= HASZEROC;
	if (hasV) i |= HASVQ;
	*p=i;ret=0;
end:
	return ret;
}

wtk_fkind_t wtk_base_parmkind(wtk_fkind_t kind)
{
	return kind & BASEMASK;
}

static int HasEnergy(wtk_fkind_t kind){return (kind & HASENERGY) != 0;}
static int HasDelta(wtk_fkind_t kind) {return (kind & HASDELTA) != 0;}
static int HasAccs(wtk_fkind_t kind)  {return (kind & HASACCS) != 0;}
static int HasThird(wtk_fkind_t kind) {return (kind & HASTHIRD) != 0;}
static int HasNulle(wtk_fkind_t kind) {return (kind & HASNULLE) != 0;}
static int HasCompx(wtk_fkind_t kind) {return (kind & HASCOMPX) != 0;}
static int HasCrcc(wtk_fkind_t kind)  {return (kind & HASCRCC) != 0;}
static int HasZerom(wtk_fkind_t kind) {return (kind & HASZEROM) != 0;}
static int HasZeroc(wtk_fkind_t kind) {return (kind & HASZEROC) != 0;}
static int HasVQ(wtk_fkind_t kind)    {return (kind & HASVQ) != 0;}

char *wtk_fkind_to_str(wtk_fkind_t kind, char *buf)
{
   strcpy(buf,pmkmap[wtk_base_parmkind(kind)].data);
   if (HasEnergy(kind))    strcat(buf,"_E");
   if (HasDelta(kind))     strcat(buf,"_D");
   if (HasNulle(kind))     strcat(buf,"_N");
   if (HasAccs(kind))      strcat(buf,"_A");
   if (HasThird(kind))     strcat(buf,"_T");
   if (HasCompx(kind))     strcat(buf,"_C");
   if (HasCrcc(kind))      strcat(buf,"_K");
   if (HasZerom(kind))     strcat(buf,"_Z");
   if (HasZeroc(kind))     strcat(buf,"_0");
   if (HasVQ(kind))        strcat(buf,"_V");
   return buf;
}
