/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*  Lexicon related functions                                            */
/*                                                                       */
/*************************************************************************/

#include "cst_features.h"
#include "cst_lexicon.h"
#include "cst_tokenstream.h"

CST_VAL_REGISTER_TYPE_NODEL(lexicon,cst_lexicon)

#define WP_SIZE 64

static int no_syl_boundaries(const cst_item *i, const cst_val *p);
static cst_val *lex_lookup_addenda(const char *wp,const cst_lexicon *l,
                                   int *found);

static int lex_match_entry(const char *a, const char *b);
static int lex_lookup_bsearch(const cst_lexicon *l,const char *word);
static int find_full_match(const cst_lexicon *l,
			   int i,const char *word);

cst_lexicon *new_lexicon()
{
    cst_lexicon *l = cst_alloc(cst_lexicon,1);
    l->syl_boundary = no_syl_boundaries;
    return l;
}

void delete_lexicon(cst_lexicon *lex)
{   /* But I doubt if this will ever be called, lexicons are mapped */
    /* This probably isn't complete */
    if (lex)
    {
	cst_free(lex->data);
	cst_free(lex);
    }
}

cst_val *cst_lex_load_addenda(const cst_lexicon *lex, const char *lexfile)
{   /* Load an addend from given file, check its phones wrt lex */
    cst_tokenstream *lf;
    const char *line;
    cst_val *e = NULL;
    cst_val *na = NULL;
    int i;

    lf = ts_open(lexfile,(const unsigned char *)"\n",(const unsigned char *)"",(const unsigned char *)"",(const unsigned char *)"");
    if (lf == NULL)
    {
      cst_errmsg("lex_add_addenda: cannot open lexicon file\n");
        return NULL;;
    }

    while (!ts_eof(lf))
    {
        line = (char*)ts_get(lf);
        if (line[0] == '#')
            continue;  /* a comment */
        for (i=0; line[i]; i++)
        {
            if (line[i] != ' ')
                break;
        }
        if (line[i])
        {
            e = cst_lex_make_entry(lex,line);
            if (e)
                na = cons_val(e,na);
        }
        else
           continue;  /* a blank line */
    }

    ts_close(lf);
    return val_reverse(na);
}

cst_val *cst_lex_make_entry(const cst_lexicon *lex, const char *entry)
{   /* if replace then replace entry in addenda of lex with entry */
    /* else append entry to addenda of lex                        */
    cst_tokenstream *e;
    cst_val *phones = NULL;
    cst_val *ventry;
    const char *w, *p;
    char *word;
    char *pos;
    int i;

    e = ts_open_string((const unsigned char *)entry,
		cst_ts_default_whitespacesymbols,
		(const unsigned char *)"",(const unsigned char *)"",(const unsigned char *)"");

    w = (char*)ts_get(e);
    if (w[0] == '"') /* it was a quoted entry */
    {                   /* so reparse it */
        ts_close(e);
        e = ts_open_string((const unsigned char *)entry,
                           cst_ts_default_whitespacesymbols,
                           (const unsigned char *)"",(const unsigned char *)"",(const unsigned char *)"");
        w = (char*)ts_get_quoted_token(e,'"','\\');
    }

    word = strdup(w);
    p = (char*)ts_get(e);
    if (!cst_streq(":",p)) /* there is a real pos */
    {
        pos = strdup(p);
        p = (char*)ts_get(e);
        if (!cst_streq(":",p)) /* there is a real pos */
        {
            cst_fprintf(stdout,"add_addenda: lex %s: expected \":\" in %s\n",
			           lex->name,
                       word);
            cst_free(word); cst_free(pos); ts_close(e);
            return NULL;
        }
    }
    else
        pos = (char*)cst_strdup((const unsigned char *)"nil");

    while (!ts_eof(e))
    {
        p = (char*)ts_get(e);
        /* Check its a legal phone */
        for (i=0; lex->phone_table[i]; i++)
            if (cst_streq(p,lex->phone_table[i]))
                break;
        if (cst_streq("#",p)) /* comment to end of line */
            break;
        else if (lex->phone_table[i])
            /* Only add it if its a valid phone */
            phones = cons_val(string_val(p),phones);
        else
        {
            cst_fprintf(stdout,"add_addenda: lex: %s word %s phone %s not in lexicon phoneset\n",
                        lex->name,
                        word,
                        p);
        }
    }

    ventry = cons_val(string_val(word),cons_val(string_val(pos),
                                                val_reverse(phones)));
    cst_free(word); cst_free(pos); ts_close(e);
#if 0
    printf("entry: ");
    val_print(stdout,ventry);
    printf("\n");
#endif

    return ventry;
}


#if 0
void lexicon_register(cst_lexicon *lex)
{
    /* Add given lexicon to list of known lexicons */
    cst_lexicon **old_lexs;
    int i;
    
    old_lexs = flite_lexicons;
    flite_num_lexicons++;
    flite_lexicons = cst_alloc(cst_lexicon *,flite_num_lexicons);
    for (i=0; i<flite_num_lexicons-1; i++)
	flite_lexicons[i] = old_lexs[i];
    flite_lexicons[i] = lex;
    cst_free(old_lexs);
}

cst_lexicon *lexicon_select(const char *name)
{
    int i;

    for (i=0; i < flite_num_lexicons; i++)
	if (cst_streq(name,flite_lexicons[i]->name))
	    return flite_lexicons[i];
    return NULL;
}
#endif

static int no_syl_boundaries(const cst_item *i, const cst_val *p)
{
    /* This is a default function that will normally be replace */
    /* for each lexicon                                         */
    (void)i;
    (void)p;
    return FALSE;
}

int in_lex(const cst_lexicon *l, const char *word, const char *pos)
{
    /* return TRUE is its in the lexicon */
    int r = FALSE, i;
    char *wp;

    wp = cst_alloc(char,strlen(word)+2);
    cst_sprintf(wp,"%c%s",(pos ? pos[0] : '0'),word);

    for (i=0; l->addenda[i]; i++)
    {
	if (((wp[0] == '0') || (wp[0] == l->addenda[i][0][0])) &&
	    (cst_streq(wp+1,l->addenda[i][0]+1)))
	{
	    r = TRUE;
	    break;
	}
    }

    if (!r && (lex_lookup_bsearch(l,wp) >= 0))
	r = TRUE;

    cst_free(wp);
    return r;
}

cst_val *lex_lookup(const cst_lexicon *l, const char *word, const char *pos)
{
    int index;
    int p;
    const unsigned char *q;
    char *wp;
    cst_val *phones = 0;
    int found = FALSE;

    wp = cst_alloc(char,strlen(word)+2);
    cst_sprintf(wp,"%c%s",(pos ? pos[0] : '0'),word);

    if (l->addenda)
	phones = lex_lookup_addenda(wp,l,&found);

    if (!found)
    {
	index = lex_lookup_bsearch(l,wp);

	if (index >= 0)
	{
        // printf("1111111\n");
	    if (l->phone_hufftable)
	    {
		for (p=index-2; l->data[p]; p--)
		    for (q=l->phone_hufftable[l->data[p]]; *q; q++)
			phones = cons_val(string_val(l->phone_table[*q]),
				  phones);
	    }
	    else  /* no compression -- should we still support this ? */
	    {
		for (p=index-2; l->data[p]; p--)
		    phones = cons_val(string_val(l->phone_table[l->data[p]]),
				      phones);
	    }
	    phones = val_reverse(phones);
	}
	else if (l->lts_rule_set)
	{
	    phones = lts_apply(word,
			       "",  /* more features if we had them */
			       l->lts_rule_set);
	}
	else if (l->lts_function)
	{
	    phones = (l->lts_function)(l,word,"");
	}
    }

    cst_free(wp);
    
    return phones;
}

static cst_val *lex_lookup_addenda(const char *wp,const cst_lexicon *l, 
				   int *found)
{
    /* For those other words */
    int i,j;
    cst_val *phones;
    
    phones = NULL;

    for (i=0; l->addenda[i]; i++)
    {
	if (((wp[0] == '0') || (wp[0] == l->addenda[i][0][0])) &&
	    (cst_streq(wp+1,l->addenda[i][0]+1)))
	{
	    for (j=1; l->addenda[i][j]; j++)
		phones = cons_val(string_val(l->addenda[i][j]),phones);
	    *found = TRUE;
	    return val_reverse(phones);
	}
    }
    
    return NULL;
}

static int lex_uncompress_word(char *ucword,int max_size,
			       int p,const cst_lexicon *l)
{
    int i,j=0,length;
    unsigned char *cword;

    if (l->entry_hufftable == 0)
        /* can have "compressed" lexicons without compression */
	cst_sprintf(ucword,"%s",&l->data[p]);
    else
    {
	cword = &l->data[p];
	for (i=0,j=0; cword[i]; i++)
	{
	    length = strlen((char*)(l->entry_hufftable[cword[i]]));
	    if (j+length+1<max_size)
	    {
		memmove(ucword+j,l->entry_hufftable[cword[i]],length);
		j += length;
	    }
	    else
		break;
	}
	ucword[j] = '\0';
    }

    return j;
}
static int lex_data_next_entry(const cst_lexicon *l,int p,int end)
{
    for (p++; p < end; p++)
	if (l->data[p-1] == 255)
	    return p;
    return end;
}
static int lex_data_prev_entry(const cst_lexicon *l,int p,int start)
{
    for (p--; p > start; p--)
	if (l->data[p-1] == 255)
	    return p;
    return start;
}
static int lex_data_closest_entry(const cst_lexicon *l,int p,int start,int end)
{
    int d;

    d=0;
    while ((p-d > start) && 
	   (p+d < end))
    {
	if (l->data[(p+d)-1] == 255)
	    return p+d;
	else if (l->data[(p-d)-1] == 255)
	    return p-d;
	d++;
    }
    return p-d;
}

static int lex_lookup_bsearch(const cst_lexicon *l, const char *word)
{
    int start,mid,end,c;
    /* needs to be longer than longest word in lexicon */
    char word_pos[WP_SIZE];

    start = 0;
    end = l->num_bytes;
    while (start < end) {
	mid = (start + end)/2;

	/* find previous entry start */
	mid = lex_data_closest_entry(l,mid,start,end);
	lex_uncompress_word(word_pos,WP_SIZE,mid,l);
    // printf("%s %s\n",word_pos,word);
	c = lex_match_entry(word_pos,word);

	if (c == 0)
	    return find_full_match(l,mid,word);
	else if (c > 0)
	    end = mid;
	else
	    start = lex_data_next_entry(l,mid + 1,end);

#if 0
	if (l->data[start-1] == 255)
	{
	    lex_uncompress_word(word_pos,WP_SIZE,start,l);
	    printf("start %s %d ",word_pos,start);
	}
	else
	    printf("start NULL %d ",start);
	if (l->data[mid-1] == 255)
	{
	    lex_uncompress_word(word_pos,WP_SIZE,mid,l);
	    printf("mid %s %d ",word_pos,mid);
	}
	else
	    printf("mid NULL %d ",mid);
	if (l->data[end-1] == 255)
	{
	    lex_uncompress_word(word_pos,WP_SIZE,end,l);
	    printf("end %s %d ",word_pos,end);
	}
	else
	    printf("end NULL %d ",end);
	printf("\n");
#endif

    }
    return -1;
}

static int find_full_match(const cst_lexicon *l,
			   int i,const char *word)

{
    /* found word, now look for actual match including pos */
    int w, match=i;
    /* needs to be longer than longest word in lexicon */
    char word_pos[WP_SIZE];

    for (w=i; w > 0; )
    {
	lex_uncompress_word(word_pos,WP_SIZE,w,l);
	if (!cst_streq(word+1,word_pos+1))
	    break;
	else if (cst_streq(word,word_pos))
	    return w;
	match = w;  /* if we can't find an exact match we'll take this one */
        /* go back to last entry */
	w = lex_data_prev_entry(l,w,0);
    }

    for (w=i; w < l->num_bytes;)
    {
	lex_uncompress_word(word_pos,WP_SIZE,w,l);
	if (!cst_streq(word+1,word_pos+1))
	    break;
	else if (cst_streq(word,word_pos))
	    return w;
        /* go to next entry */
	w = lex_data_next_entry(l,w,l->num_bytes);
    }

    return match;
}

static int lex_match_entry(const char *a, const char *b)
{
    int c;

    c = strcmp(a+1,b+1);

    return c;
}

/*the following code add by hlwang*/
user_lexicon **load_user_lexicon(const  char *lex_file_name)
{
    user_lexicon **pLex;
    int nLen;//,j;
    FILE *fp;
    char szLine[ALINE];
    int  nWord=0;
   char *p;

    printf("\nLoading system dictionary! Please wait.");
    /*open the Dic file which is from source tree of product*/
    fp=fopen(lex_file_name,"rt");
    if(!fp)
    {
        printf("Can't open the dictionary!\n");
	exit(0);
    }
    pLex=(user_lexicon **)calloc(WRDHASHSIZE,sizeof(user_lexicon *));

    //read word&pronunciation line by line
    p=fgets(szLine,ALINE,fp);
    if(!p)
    {
    	return NULL;
    }
    while(!feof(fp))
    {
        nWord++;
	nLen=strlen(szLine);
	szLine[nLen-1]=0x00;
	addEntry(szLine,pLex);
	
	if(nWord%3500==0) printf(".");
		p=fgets(szLine,ALINE,fp);
    }
    printf("\n %d words are Loaded!\n",nWord);
    fclose(fp);
    return pLex;
}
void addEntry(char *szLine,user_lexicon **Dict)
{
    char tmpPhn[WRDLEN*2][PHNLEN];
    char wrdName[WRDLEN];
    word_pruns *pw=NULL;	
    int  i=0,j=0,flag=0;
    unsigned long key=0;
    char *p=szLine;
    user_lexicon *head;
    user_lexicon *szUnit=(user_lexicon *)calloc(1,sizeof(user_lexicon));
    szUnit->next=NULL;
    szUnit->itsPruns=(word_pruns *)calloc(1,sizeof(word_pruns));
    szUnit->nPruns=1;
    while(*p!=0x00)
    {
        if(*p==' ') 
	{
            if(i==0) wrdName[j]=0x00;
            else  tmpPhn[i-1][j]=0x00; 
            i++;
            j=0;
	}
	else
            if(i==0)    wrdName[j++]=*p;
            else    tmpPhn[i-1][j++]=*p;
	p++;
    }
    if(i==0)    wrdName[j]=0x00;
    else    tmpPhn[i-1][j]=0x00;
    //ssy len + 1
    szUnit->wrd_name=(char *)calloc(strlen(wrdName)+1,sizeof(char));
    memmove( szUnit->wrd_name, wrdName,strlen(wrdName)+1);
    szUnit->itsPruns->phnSeq=(char **)calloc(i,sizeof(char *));            
    for(j=0;j<i;j++)
    {
        szUnit->itsPruns->phnSeq[j]=(char *)calloc(strlen(tmpPhn[j])+1,sizeof(char));
        memmove(szUnit->itsPruns->phnSeq[j], tmpPhn[j],strlen(tmpPhn[j])+1);
    }
    szUnit->itsPruns->nPhns=i;
    szUnit->itsPruns->anth_prun=NULL;

	//Hash value
	if(szUnit->itsPruns->nPhns >= WRDLEN*2)
	{
        printf("\nerr [%s]\n",szLine);
		printf("There is a long word as %s!\n",szUnit->wrd_name);
		printf("Loading dic failed!\n");
		exit(1);
	}
        p=szUnit->wrd_name;
	key=getWordHashValue(p);
        
	//add operation
	if(Dict[key]==NULL)
	{
		Dict[key]=szUnit;
		if(Dict[key]==NULL)
		{
			printf("List operation failed because of lack of memory!\n");
			exit(1);
		}
	}
	else
	{
		head=Dict[key];
                while(head->next)
                {
                    if(!strcmp(head->wrd_name,szUnit->wrd_name))
                    {
                        flag=1;
                        break;
                    }
                    else 
                        head=head->next;
                }
                if(flag==0)
                {
                    head->next=szUnit;
                    if(head->next==NULL)
                    {
                        printf("List operation failed because of lack of memory!\n");
                        exit(1);
                    }
                }
                else
                {
                    pw=head->itsPruns;
                    while(pw->anth_prun)
                        pw=pw->anth_prun;
                    pw->anth_prun=szUnit->itsPruns; 
                    //(word_pruns *)calloc(1,sizeof(word_pruns));
                    //memcpy(pw->anth_prun,szUnit->itsPruns,sizeof(word_pruns));
                    head->nPruns=head->nPruns+1;
                    cst_free(szUnit->wrd_name);
                    cst_free(szUnit);
                }
        }   
}

/*Hash function for the Word hash tables*/
unsigned  long getWordHashValue (const char *pszWord)
{
    unsigned long dHash;
    for (dHash = 0; *pszWord; pszWord++)
        dHash += *pszWord;

    return (((dHash << 7) - dHash) %WRDHASHSIZE);
}

const cst_val *lookup_user_lexicon(user_lexicon **userLex,const  char *pszWord)
{
    int i;
    const cst_val *pPruns=NULL;
    unsigned long key=getWordHashValue(pszWord);
    int nFind=FALSE;
    user_lexicon *head=userLex[key];
    while(head)
    {
         if( !strcmp(head->wrd_name, pszWord))
	{
             nFind=TRUE;
             break;
        }
	else
            head=head->next;
    }
    if(!nFind)
    {	
		printf("Error: there don't exist word \"%s\" in the dictionary!\n",pszWord);
		exit(1);
    }
    else
    {
        //return head->itsPruns;
        for(i=0;i<head->itsPruns->nPhns;i++)
        {
            pPruns = cons_val(string_val(head->itsPruns->phnSeq[i]),
				 pPruns);
        }
        pPruns = val_reverse((cst_val*)pPruns);
    }
    return pPruns;            
}


void cst_userLex_free(user_lexicon **userLex)
{
    int i,j;
    user_lexicon *pLex1,*pLex2;
    word_pruns *pprun1,*pprun2;
    for(i=0; i<WRDHASHSIZE;i++)
    {
        if(userLex[i])
        {
            pLex1=userLex[i];
            while(pLex1)
            {
                pLex2=pLex1;
                pLex1=pLex1->next;
                cst_free(pLex2->wrd_name);
                pprun1=pLex2->itsPruns;
                while(pprun1)
                {
                    pprun2=pprun1;
                    pprun1=pprun1->anth_prun;
                    for(j=0;j<pprun2->nPhns;j++)
                        cst_free(pprun2->phnSeq[j]);
                    cst_free(pprun2->phnSeq);
                    cst_free(pprun2);
                }
                cst_free(pLex2);
            } 
        }
    }        
    cst_free(userLex);
}
