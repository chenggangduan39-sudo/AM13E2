/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2001                            */
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
/*               Date:  January 2001                                     */
/*************************************************************************/

/* coded for using HTS - Tomoki Toda 12/05/08 */

#include "cst_math.h"
#include "cst_hrg.h"
#include "cst_utt_utils.h"
#include "cst_wave.h"
#include "cst_track.h"
#include "cst_diphone.h"
#include "cst_hts.h"
#include "cst_units.h"
#include "cst_sigpr.h"

/*----------------------------------------------------------------------*/
/*add by hlwang for output flite duration       */
#include "cst_cart.h"
#include "cst_synth.h"

void set_wave(cst_utterance *utt, double *data, int len, int fs)
{
    cst_wave *w = 0;
    int t;
    
    w = new_wave();
    cst_wave_resize(w, len, 1);
    w->sample_rate = fs;
    for (t = 0; t < len; t++) w->samples[t] = (short)data[t];

    utt_set_wave(utt,w);

    return;
}

void put_htslabdur(cst_utterance *utt, hts_lab *hl)
{
    const cst_item *it;
    for (it = relation_head(utt_relation(utt, "Segment")); 
	 it != NULL && hl != NULL; it = item_next(it), hl = hl->next) {
	item_set_float(it, "end", hl->dur);
        
    }

    return;
}

void free_htslab(hts_lab *hl)
{
    if (hl != NULL) {
	if (hl->next != NULL) {free_htslab(hl->next);	hl->next = NULL;}
	if (hl->lab != NULL) {free(hl->lab);	hl->lab = NULL;}
	free(hl);	hl = NULL;
    }

    return;
}

void print_htslab(hts_lab *hl, const char *filename) {
    FILE *fp=NULL;
    if(filename!=NULL)
    {
        if((fp=fopen(filename,"wt"))==NULL)
        {
            printf("HError: cann't open the file %s to write.\n",filename);
            exit(1);
        }
        else
        {
                    for (; hl != NULL; hl = hl->next) 
                        fprintf(fp,"%s\n", hl->lab);
                    fclose(fp);
        }
    }
    else  
        for (; hl != NULL; hl = hl->next) 
            printf("%s\n", hl->lab);
   
    return;
}

void mod_htslab(hts_lab *hl, char *al)
{
    for (; hl != NULL; hl = hl->next) {
	strcat(hl->lab, al);
    }

    return;
}

#include <stdio.h>

void output_rhythmtext(cst_utterance *u, const char *filename)
{
	cst_item *s;
	static int id = 0;

	FILE *fp = fopen(filename, "at");
	if(!fp){
		return;
	}
	fprintf(fp, "<%08d>\n", id);
	/* wav */
	fprintf(fp, "Wav=\n");

	/* text */
	fprintf(fp, "Text=%s\n", feat_string(u->features,"input_text"));

	/* rhythmtext */
	fprintf(fp, "RhythmText=");
	for (s=NULL,s=relation_head(utt_relation(u,"Phrase")); s; s=item_next(s)){
		int boud = 1;
		cst_item *word = item_daughter(s);
		cst_item *syl  = NULL;
		const char *accent = NULL;
		const char *endtone= NULL;

		for(; item_next(word); word=item_next(word)){
			syl = path_to_item(word,"R:SylStructure.daughtern");
			accent = NULL;
			for(; syl; syl=item_prev(syl)){
				if(NULL == accent || '0' == *accent)
					accent = ffeature_string(syl, "accent");
			}
			if(NULL != accent && '0' != *accent)
				fprintf(fp, "%s{%s}#%d ", ffeature_string(word, "name"), accent, boud);
			else
				fprintf(fp, "%s#%d ", ffeature_string(word, "name"), boud);
		}
		boud = 4;
		if(!item_next(s)) boud = 5;
		syl = path_to_item(word,"R:SylStructure.daughtern");
		accent = NULL;
		endtone= ffeature_string(syl, "endtone");
		for(; syl; syl=item_prev(syl)){
			if(NULL == accent || '0' == *accent)
				accent = ffeature_string(syl, "accent");
		}
		if(NULL != accent && '0' != *accent)
			fprintf(fp, "%s{%s,%s}#%d ", ffeature_string(word, "name"), accent, endtone, boud);
		else
			fprintf(fp, "%s{%s}#%d ", ffeature_string(word, "name"), endtone, boud);
	}
	fprintf(fp, "\n");

	/* Pinyin */
	fprintf(fp, "Pinyin=");
	for(s=relation_head(utt_relation(u,"Word")); s; s=item_next(s)){
		cst_item *syl = path_to_item(s,"R:SylStructure.daughtern");
		char buff[256] = {0};
		int len = 0;
		len = sprintf(buff+len, "[");
		for(; item_prev(syl); syl=item_prev(syl));
		for(; syl; syl=item_next(syl)){
			cst_item *seg = item_last_daughter(syl);
			len += sprintf(buff+len, "(");
			for (; item_prev(seg); seg=item_prev(seg));
			for (; seg; seg=item_next(seg)){
				len += sprintf(buff+len, "%s ", item_name(seg));
			}
			len --;
			len += sprintf(buff+len, ")");
			len += sprintf(buff+len, "%s ", ffeature_string(syl, "stress"));
		}
		len --;
		len += sprintf(buff+len, "] ");
		fprintf(fp,"%s",buff);
	}
	fprintf(fp, "\n");

	/* Pos */
	fprintf(fp, "Pos=");
	for(s=relation_head(utt_relation(u,"Word")); s; s=item_next(s)){
		const char *name= ffeature_string(s, "name");
		const char *pos = ffeature_string(s, "gpos");
		fprintf(fp, "%s/%s ", name, pos);
	}
	fprintf(fp, "\n");

	fprintf(fp, "</%08d>\n", id);
	fclose(fp);
	id ++;
}

#include "wtk/core/wtk_strbuf.h"

hts_lab *get_htslab(cst_utterance *u)
{
    cst_item *s;
    hts_lab *htslab = NULL;
    hts_lab *hl, *phl = NULL;
	int nsyl_in = 0;
    /* memory allocation */
    htslab = cst_alloc(hts_lab, 1);
    htslab->lab = NULL;	htslab->next = NULL;
    phl = hl = htslab;
    for (s=relation_head(utt_relation(u,"Segment")); s; s=item_next(s)) {
	/* memory allocation */
	hl->lab = cst_alloc(char, 1024);
	strcpy(hl->lab, "");
#if 1
	wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);
	if (!strcmp("0", ffeature_string(s,"pp.name")))
	{
		wtk_strbuf_push_f(buf,"x",1);
	}else
	{
		wtk_strbuf_push_f(buf,"%s", ffeature_string(s,"pp.name"));
	}
	if (!strcmp("0", ffeature_string(s,"p.name")))
	{
		wtk_strbuf_push(buf,"^x",2);
	}else
	{
		wtk_strbuf_push_f(buf,"^%s", ffeature_string(s,"p.name"));
	}
	wtk_strbuf_push_f(buf,"-%s", ffeature_string(s,"name"));

	if (!strcmp("0", ffeature_string(s,"n.name")))
	{
		wtk_strbuf_push(buf,"+x", 2);
	}else
	{
		wtk_strbuf_push_f(buf,"+%s", ffeature_string(s,"n.name"));
	}
	if (!strcmp("0", ffeature_string(s,"nn.name")))
	{
		wtk_strbuf_push(buf,"=x", 2);
	}else
	{
		wtk_strbuf_push_f(buf,"=%s", ffeature_string(s,"nn.name"));
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"@x",2);
	}else
	{
		wtk_strbuf_push_f(buf,"@%d",ffeature_int(s,"pos_in_syl") + 1);
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"_x",2);
	}else
	{
		wtk_strbuf_push_f(buf,"_%d",hts_syl_numphonemes(s) - ffeature_int(s,"pos_in_syl"));
	}

	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"/A:%s",ffeature_string(s,"p.R:SylStructure.parent.stress"));
	}else
	{
		wtk_strbuf_push_f(buf,"/A:%s", ffeature_string(s,"R:SylStructure.parent.R:Syllable.p.stress"));
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"_%s",ffeature_string(s,"p.R:SylStructure.parent.accented"));
	}else
	{
		wtk_strbuf_push_f(buf,"_%s",ffeature_string(s,"R:SylStructure.parent.R:Syllable.p.accented"));
	}
	wtk_strbuf_push_f(buf,"_%d",hts_p_syl_numphonemes(s));

	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"/B:x",4);
	}else
	{
		wtk_strbuf_push_f(buf,"/B:%s",ffeature_string(s,"R:SylStructure.parent.stress"));
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"-x",2);
	}else
	{
		wtk_strbuf_push_f(buf,"-%s",ffeature_string(s,"R:SylStructure.parent.accented"));
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"-x",2);
	}else
	{
		wtk_strbuf_push_f(buf,"-%d",hts_syl_numphonemes(s));
	}
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push(buf,"@x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"@%d",ffeature_int(s,"R:SylStructure.parent.pos_in_syl") + 1);
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"-x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"-%d",ffeature_int(s,"R:SylStructure.parent.parent.word_numsyls") - ffeature_int(s,"R:SylStructure.parent.pos_in_syl"));
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"&x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"&%d",ffeature_int(s,"R:SylStructure.parent.syl_in") + 1);
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"-x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"-%d",ffeature_int(s,"R:SylStructure.parent.syl_out") + 1);	
	}
	// printf("%s\n",ffeature_string(s,"name"));
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push(buf,"#x",2);
		nsyl_in = ffeature_int(s,"p.R:SylStructure.parent.ssyl_in");
	}
	else
	{
		// wtk_strbuf_push_f(buf,"#%d", ffeature_int(s,"R:SylStructure.parent.ssyl_in") + 1);
		wtk_strbuf_push_f(buf,"#%d", ffeature_int(s,"R:SylStructure.parent.ssyl_in")-nsyl_in);	//ssy 按照王飞改
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"-x",2);
	}
	else
	{
		// wtk_strbuf_push_f(buf,"-%d",ffeature_int(s,"R:SylStructure.parent.ssyl_out") + 1);
		wtk_strbuf_push_f(buf,"-%d",ffeature_int(s,"R:SylStructure.parent.ssyl_out"));	//ssy 按照王飞改
	}
	/* position in phrase (accented syllable) */
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push(buf,"$x",2);
	}
	else
	{
		// wtk_strbuf_push_f(buf,"$%d",ffeature_int(s,"R:SylStructure.parent.asyl_in") + 1);
		wtk_strbuf_push_f(buf,"$%d",ffeature_int(s,"R:SylStructure.parent.asyl_in")); //ssy 和王飞保持一致默认0
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"-x",2);
	}
	else
	{
		// wtk_strbuf_push_f(buf,"-%d", ffeature_int(s,"R:SylStructure.parent.asyl_out") + 1);
		wtk_strbuf_push_f(buf,"-%d", ffeature_int(s,"R:SylStructure.parent.asyl_out")); //ssy 和王飞保持一致默认0
	}

	/* distance from stressed syllable */
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push(buf,"!x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"!%d",ffeature_int(s,"R:SylStructure.parent.prev_stress"));
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"-x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"-%d", ffeature_int(s,"R:SylStructure.parent.next_stress"));
	}
	/* distance from accented syllable */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,";x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,";%d",ffeature_int(s,"R:SylStructure.parent.prev_accent"));
	}
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push(buf,"-x",2);
	}else
	{
		wtk_strbuf_push_f(buf,"-%d",ffeature_int(s,"R:SylStructure.parent.next_accent"));
	}
	/* name of the vowel of current syllable */
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push(buf,"|x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"|%s",ffeature_string(hts_syl_vowel(s),"name"));
	}

	/*## next syllable */
	/* n.stress */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"/C:%s",ffeature_string(s,"n.R:SylStructure.parent.stress"));
	}
	else
	{
		wtk_strbuf_push_f(buf,"/C:%s",ffeature_string(s,"R:SylStructure.parent.R:Syllable.n.stress"));
	}
	/* n.accent */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"+%s", ffeature_string(s,"n.R:SylStructure.parent.accented"));
	}
	else
	{
		wtk_strbuf_push_f(buf,"+%s",ffeature_string(s,"R:SylStructure.parent.R:Syllable.n.accented"));
	}
	/* n.length */
	wtk_strbuf_push_f(buf,"+%d",hts_n_syl_numphonemes(s));

/*############################## */
	/*#  WORD */
	/*## previous word */
	/* p.gpos */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"/D:%s",ffeature_string(s,"p.R:SylStructure.parent.parent.R:Word.gpos"));
	}
	else
	{
		wtk_strbuf_push_f(buf,"/D:%s",ffeature_string(s,"R:SylStructure.parent.parent.R:Word.p.gpos"));
	}
	/* p.length (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"_%d", ffeature_int(s,"p.R:SylStructure.parent.parent.word_numsyls"));
	}
	else
	{
		wtk_strbuf_push_f(buf,"_%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Word.p.word_numsyls"));
	}

	/*## current word */
	/* c.gpos */
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push_f(buf,"/E:x",4);
	}
	else
	{
		wtk_strbuf_push_f(buf,"/E:%s", ffeature_string(s,"R:SylStructure.parent.parent.R:Word.gpos"));
	}
	/* c.length (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"+x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"+%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Word.word_numsyls"));
	}

	/* position in phrase (word) */
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push(buf,"@x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"@%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Word.word_in") + 1);
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"+x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"+%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Word.word_out") + 1);
	}
	/* position in phrase (content word) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"&x",2);
	}
	else
	{
		// wtk_strbuf_push_f(buf,"&%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Word.cword_in") + 1);
		wtk_strbuf_push_f(buf,"&%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Word.cword_in"));	//ssy 根据王飞算法改造
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"+x",2);
	}
	else
	{
		// wtk_strbuf_push_f(buf,"+%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Word.cword_out") + 1);
		wtk_strbuf_push_f(buf,"+%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Word.cword_out"));	//ssy 根据王飞算法改
	}

	/* distance from content word in phrase */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"#x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"#%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Word.prev_cword"));
	}
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push(buf,"+x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"+%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Word.next_cword"));
	}

	/*## next word */
	/* n.gpos */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"/F:%s",ffeature_string(s,"n.R:SylStructure.parent.parent.R:Word.gpos"));
	}
	else
	{
		wtk_strbuf_push_f(buf,"/F:%s",ffeature_string(s,"R:SylStructure.parent.parent.R:Word.n.gpos"));
	}
	/* n.length (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"_%d", ffeature_int(s,"n.R:SylStructure.parent.parent.word_numsyls"));
	}
	else
	{
		wtk_strbuf_push_f(buf,"_%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Word.n.word_numsyls"));
	}
	/*############################## */
	/*#  PHRASE */
	/*## previous phrase */
	/* length of previous phrase (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		// wtk_strbuf_push_f(buf,"/G:%d", ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.phrase_numsyls"));
		wtk_strbuf_push_f(buf,"/G:%d", ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.p.phrase_numsyls"));
	}
	else
	{
		wtk_strbuf_push_f(buf,"/G:%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.p.phrase_numsyls"));
	}
	/* length of previous phrase (word) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		// wtk_strbuf_push_f(buf,"_%d", ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.phrase_numwords"));
		wtk_strbuf_push_f(buf,"_%d",ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.p.phrase_numwords"));	//ssy 按照王飞改
	}
	else
	{
		wtk_strbuf_push_f(buf,"_%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.p.phrase_numwords"));
	}
	/*## current phrase */
	/* length of current phrase (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push(buf,"/H:x",4);
	}
	else
	{
		wtk_strbuf_push_f(buf, "/H:%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.phrase_numsyls"));
	}
	/* length of current phrase (word) */
	if (!strcmp("pau", ffeature_string(s,"name"))) 
	{
		wtk_strbuf_push(buf,"=x",2);
	}
	else
	{
		wtk_strbuf_push_f(buf,"=%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.phrase_numwords"));
	}

	/* position in major phrase (phrase) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
	  if (item_next(s) != NULL)
	  {
		  //wtk_strbuf_push_f(buf,"@%d", ffeature_int(s,"n.R:SylStructure.parent.R:Syllable.sub_phrases") + 1);
		  wtk_strbuf_push_f(buf,"^%d", ffeature_int(s,"n.R:SylStructure.parent.R:Syllable.sub_phrases") + 1);  //fit research
	  }
	  else
	  {
		  //wtk_strbuf_push_f(buf,"@%d",ffeature_int(s,"p.R:SylStructure.parent.R:Syllable.sub_phrases") + 1);
		  wtk_strbuf_push_f(buf,"^%d",ffeature_int(s,"p.R:SylStructure.parent.R:Syllable.sub_phrases") + 1);  //fit research
	  }
	}
	else
	{
		 //wtk_strbuf_push_f(buf,"@%d",ffeature_int(s,"R:SylStructure.parent.R:Syllable.sub_phrases") + 1);
		wtk_strbuf_push_f(buf,"^%d",ffeature_int(s,"R:SylStructure.parent.R:Syllable.sub_phrases") + 1);   //fit research
	}
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
	  if (item_next(s) != NULL)
	  {
		  wtk_strbuf_push_f(buf,"=%d",ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.numphrases") - ffeature_int(s,"n.R:SylStructure.parent.R:Syllable.sub_phrases"));
	  }
	  else
	  {
		  wtk_strbuf_push_f(buf,"=%d",ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.numphrases") - ffeature_int(s,"p.R:SylStructure.parent.R:Syllable.sub_phrases"));
	  }
	}
	else
	{
		wtk_strbuf_push_f(buf,"=%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.numphrases") - ffeature_int(s,"R:SylStructure.parent.R:Syllable.sub_phrases"));
	}
	/* type of tobi endtone of current phrase */
	// wtk_strbuf_push_f(buf,"|%s",ffeature_string(s, "R:SylStructure.parent.parent.R:Phrase.parent.daughtern.R:SylStructure.daughtern.endtone"));
	wtk_strbuf_push_f(buf,"|%s","L-L%");	//ssy 改成L-L%
	/*## next phrase */
	/* length of next phrase (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"/I:%d", ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.phrase_numsyls"));
	}
	else
	{
		wtk_strbuf_push_f(buf,"/I:%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.n.phrase_numsyls"));
	}
	/* length of next phrase (word) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
		wtk_strbuf_push_f(buf,"=%d",ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.phrase_numwords"));
	}
	else
	{
		wtk_strbuf_push_f(buf,"=%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.n.phrase_numwords"));
	}

	/*#  UTTERANCE */
	/* length (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
	  if (item_next(s) != NULL)
	  {
		  wtk_strbuf_push_f(buf,"/J:%d", ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.numsyls"));
	  }
	  else
	  {
		  wtk_strbuf_push_f(buf,"/J:%d",ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.numsyls"));
	  }
	}
	else
	{
		wtk_strbuf_push_f(buf,"/J:%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.numsyls"));
	}

	/* length (word) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
	  if (item_next(s) != NULL)
	  {
	    wtk_strbuf_push_f(buf, "+%d", ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.numwords"));
	  }
	  else
	  {
		  wtk_strbuf_push_f(buf,"+%d", ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.numwords"));
	  }
	}
	else
	{
		 wtk_strbuf_push_f(buf,"+%d",ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.numwords"));
	}

	/* length (phrase) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	{
	  if (item_next(s) != NULL)
	  {
		  wtk_strbuf_push_f(buf,"-%d",ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.numphrases"));
	  }
	  else
	  {
		  wtk_strbuf_push_f(buf,"-%d", ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.numphrases"));
	  }
	}
	else
	{
		wtk_strbuf_push_f(buf,"-%d", ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.numphrases"));
	}
	memcpy(hl->lab,buf->data,buf->pos);
	wtk_strbuf_delete(buf);
#else
	/*############################## */
	/*###  SEGMENT */
        /* pp.name */
	if (!strcmp("0", ffeature_string(s,"pp.name"))) sprintf(hl->lab, "%sx", hl->lab);
	else sprintf(hl->lab, "%s%s", hl->lab, ffeature_string(s,"pp.name"));
	/* p.name */
	if (!strcmp("0", ffeature_string(s,"p.name"))) sprintf(hl->lab, "%s^x", hl->lab);
	else sprintf(hl->lab, "%s^%s", hl->lab, ffeature_string(s,"p.name"));
	/* c.name */
	sprintf(hl->lab, "%s-%s", hl->lab, ffeature_string(s,"name"));
        /*sprintf(hl->lab, "%s%s", hl->lab, ffeature_string(s,"name"));*/
	/* n.name */
	if (!strcmp("0", ffeature_string(s,"n.name"))) sprintf(hl->lab, "%s+x", hl->lab);
	else sprintf(hl->lab, "%s+%s", hl->lab, ffeature_string(s,"n.name"));
	/* nn.name */
	if (!strcmp("0", ffeature_string(s,"nn.name"))) sprintf(hl->lab, "%s=x", hl->lab);
	else sprintf(hl->lab, "%s=%s", hl->lab, ffeature_string(s,"nn.name"));

	/* position in syllable (segment) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s@x", hl->lab);
	else sprintf(hl->lab, "%s@%d", hl->lab, ffeature_int(s,"pos_in_syl") + 1);
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s_x", hl->lab);
	else sprintf(hl->lab, "%s_%d", hl->lab, hts_syl_numphonemes(s) - ffeature_int(s,"pos_in_syl"));

	/*##############################*/
	/*###  SYLLABLE */
	/*## previous syllable */
	/* p.stress */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s/A:%s", hl->lab, ffeature_string(s,"p.R:SylStructure.parent.stress"));
	else
	  sprintf(hl->lab, "%s/A:%s", hl->lab, ffeature_string(s,"R:SylStructure.parent.R:Syllable.p.stress"));
	/* p.accent */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s_%s", hl->lab, ffeature_string(s,"p.R:SylStructure.parent.accented"));
	else
	  sprintf(hl->lab, "%s_%s", hl->lab, ffeature_string(s,"R:SylStructure.parent.R:Syllable.p.accented"));
	/* p.length */
	sprintf(hl->lab, "%s_%d", hl->lab, hts_p_syl_numphonemes(s));

	/*## current syllable */
	/* c.stress */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s/B:x", hl->lab);
	else
	  sprintf(hl->lab, "%s/B:%s", hl->lab, ffeature_string(s,"R:SylStructure.parent.stress"));
	/* c.accent */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s-x", hl->lab);
	else
	  sprintf(hl->lab, "%s-%s", hl->lab, ffeature_string(s,"R:SylStructure.parent.accented"));
	/* c.length */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s-x", hl->lab);
	else sprintf(hl->lab, "%s-%d", hl->lab, hts_syl_numphonemes(s));

	/* position in word (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s@x", hl->lab);
	else
	  sprintf(hl->lab, "%s@%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.pos_in_syl") + 1);
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s-x", hl->lab);
	else
	  sprintf(hl->lab, "%s-%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.word_numsyls") - ffeature_int(s,"R:SylStructure.parent.pos_in_syl"));

	/* position in phrase (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s&x", hl->lab);
	else
	  sprintf(hl->lab, "%s&%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.syl_in") + 1);
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s-x", hl->lab);
	else
	  sprintf(hl->lab, "%s-%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.syl_out") + 1);

	/* position in phrase (stressed syllable) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s#x", hl->lab);
	else
	  sprintf(hl->lab, "%s#%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.ssyl_in") + 1);
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s-x", hl->lab);
	else
	  sprintf(hl->lab, "%s-%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.ssyl_out") + 1);

	/* position in phrase (accented syllable) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s$x", hl->lab);
	else
	  sprintf(hl->lab, "%s$%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.asyl_in") + 1);
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s-x", hl->lab);
	else
	  sprintf(hl->lab, "%s-%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.asyl_out") + 1);

	/* distance from stressed syllable */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s!x", hl->lab);
	else
	  sprintf(hl->lab, "%s!%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.prev_stress"));
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s-x", hl->lab);
	else
	  sprintf(hl->lab, "%s-%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.next_stress"));
	  
	/* distance from accented syllable */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s;x", hl->lab);
	else
	  sprintf(hl->lab, "%s;%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.prev_accent"));
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s-x", hl->lab);
	else
	  sprintf(hl->lab, "%s-%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.next_accent"));

	/* name of the vowel of current syllable */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s|x", hl->lab);
	else sprintf(hl->lab, "%s|%s", hl->lab, ffeature_string(hts_syl_vowel(s),"name"));

	/*## next syllable */
	/* n.stress */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s/C:%s", hl->lab, ffeature_string(s,"n.R:SylStructure.parent.stress"));
	else
	  sprintf(hl->lab, "%s/C:%s", hl->lab, ffeature_string(s,"R:SylStructure.parent.R:Syllable.n.stress"));
	/* n.accent */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s+%s", hl->lab, ffeature_string(s,"n.R:SylStructure.parent.accented"));
	else
	  sprintf(hl->lab, "%s+%s", hl->lab, ffeature_string(s,"R:SylStructure.parent.R:Syllable.n.accented"));
	/* n.length */
	sprintf(hl->lab, "%s+%d", hl->lab, hts_n_syl_numphonemes(s));

	/*############################## */
	/*#  WORD */
	/*## previous word */
	/* p.gpos */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s/D:%s", hl->lab, ffeature_string(s,"p.R:SylStructure.parent.parent.R:Word.gpos"));
	else
	  sprintf(hl->lab, "%s/D:%s", hl->lab, ffeature_string(s,"R:SylStructure.parent.parent.R:Word.p.gpos"));
	/* p.length (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s_%d", hl->lab, ffeature_int(s,"p.R:SylStructure.parent.parent.word_numsyls"));
	else
	  sprintf(hl->lab, "%s_%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Word.p.word_numsyls"));

	/*## current word */
	/* c.gpos */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s/E:x", hl->lab);
	else
	  sprintf(hl->lab, "%s/E:%s", hl->lab, ffeature_string(s,"R:SylStructure.parent.parent.R:Word.gpos"));
	/* c.length (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s+x", hl->lab);
	else
	  sprintf(hl->lab, "%s+%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Word.word_numsyls"));

	/* position in phrase (word) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s@x", hl->lab);
	else
	  sprintf(hl->lab, "%s@%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Word.word_in") + 1);
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s+x", hl->lab);
	else
	  sprintf(hl->lab, "%s+%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Word.word_out") + 1);

	/* position in phrase (content word) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s&x", hl->lab);
	else
	  sprintf(hl->lab, "%s&%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Word.cword_in") + 1);
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s+x", hl->lab);
	else
	  sprintf(hl->lab, "%s+%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Word.cword_out") + 1);

	/* distance from content word in phrase */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s#x", hl->lab);
	else
	  sprintf(hl->lab, "%s#%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Word.prev_cword"));
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s+x", hl->lab);
	else
	  sprintf(hl->lab, "%s+%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Word.next_cword"));

	/*## next word */
	/* n.gpos */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s/F:%s", hl->lab, ffeature_string(s,"n.R:SylStructure.parent.parent.R:Word.gpos"));
	else
	  sprintf(hl->lab, "%s/F:%s", hl->lab, ffeature_string(s,"R:SylStructure.parent.parent.R:Word.n.gpos"));
	/* n.length (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s_%d", hl->lab, ffeature_int(s,"n.R:SylStructure.parent.parent.word_numsyls"));
	else
	  sprintf(hl->lab, "%s_%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Word.n.word_numsyls"));

	/*############################## */
	/*#  PHRASE */
	/*## previous phrase */
	/* length of previous phrase (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s/G:%d", hl->lab, ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.phrase_numsyls"));
	else
	  sprintf(hl->lab, "%s/G:%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.p.phrase_numsyls"));
	/* length of previous phrase (word) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s_%d", hl->lab, ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.phrase_numwords"));
	else
	  sprintf(hl->lab, "%s_%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.p.phrase_numwords"));

	/*## current phrase */
	/* length of current phrase (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s/H:x", hl->lab);
	else
	  sprintf(hl->lab, "%s/H:%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.phrase_numsyls"));
	/* length of current phrase (word) */
	if (!strcmp("pau", ffeature_string(s,"name"))) sprintf(hl->lab, "%s=x", hl->lab);
	else
	  sprintf(hl->lab, "%s=%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.phrase_numwords"));

	/* position in major phrase (phrase) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  if (item_next(s) != NULL)
	    sprintf(hl->lab, "%s@%d", hl->lab, ffeature_int(s,"n.R:SylStructure.parent.R:Syllable.sub_phrases") + 1);
	  else
	    sprintf(hl->lab, "%s@%d", hl->lab, ffeature_int(s,"p.R:SylStructure.parent.R:Syllable.sub_phrases") + 1);
	else
	  sprintf(hl->lab, "%s@%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.R:Syllable.sub_phrases") + 1);
	if (!strcmp("pau", ffeature_string(s,"name")))
	  if (item_next(s) != NULL)
	    sprintf(hl->lab, "%s=%d", hl->lab, ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.numphrases") - ffeature_int(s,"n.R:SylStructure.parent.R:Syllable.sub_phrases"));
	  else
	    sprintf(hl->lab, "%s=%d", hl->lab, ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.numphrases") - ffeature_int(s,"p.R:SylStructure.parent.R:Syllable.sub_phrases"));
	else
	  sprintf(hl->lab, "%s=%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.numphrases") - ffeature_int(s,"R:SylStructure.parent.R:Syllable.sub_phrases"));

	/* type of tobi endtone of current phrase */
	sprintf(hl->lab, "%s|%s", hl->lab, ffeature_string(s, "R:SylStructure.parent.parent.R:Phrase.parent.daughtern.R:SylStructure.daughtern.endtone"));

	/*## next phrase */
	/* length of next phrase (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s/I:%d", hl->lab, ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.phrase_numsyls"));
	else
	  sprintf(hl->lab, "%s/I:%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.n.phrase_numsyls"));
	/* length of next phrase (word) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  sprintf(hl->lab, "%s=%d", hl->lab, ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.phrase_numwords"));
	else
	  sprintf(hl->lab, "%s=%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.n.phrase_numwords"));

	/*#  UTTERANCE */
	/* length (syllable) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  if (item_next(s) != NULL)
	    sprintf(hl->lab, "%s/J:%d", hl->lab, ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.numsyls"));
	  else
	    sprintf(hl->lab, "%s/J:%d", hl->lab, ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.numsyls"));
	else
	  sprintf(hl->lab, "%s/J:%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.numsyls"));

	/* length (word) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  if (item_next(s) != NULL)
	    sprintf(hl->lab, "%s+%d", hl->lab, ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.numwords"));
	  else
	    sprintf(hl->lab, "%s+%d", hl->lab, ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.numwords"));
	else
	  sprintf(hl->lab, "%s+%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.numwords"));

	/* length (phrase) */
	if (!strcmp("pau", ffeature_string(s,"name")))
	  if (item_next(s) != NULL)
	    sprintf(hl->lab, "%s-%d", hl->lab, ffeature_int(s,"n.R:SylStructure.parent.parent.R:Phrase.parent.numphrases"));
	  else
	    sprintf(hl->lab, "%s-%d", hl->lab, ffeature_int(s,"p.R:SylStructure.parent.parent.R:Phrase.parent.numphrases"));
	else
	  sprintf(hl->lab, "%s-%d", hl->lab, ffeature_int(s,"R:SylStructure.parent.parent.R:Phrase.parent.numphrases"));

	sprintf(hl->lab, "%s", hl->lab);
#endif
	/* memory allocation */
	phl = hl;
	hl->next = cst_alloc(hts_lab, 1);
	hl = hl->next;	hl->lab = NULL;	hl->next = NULL;
    }
    phl->next = NULL;
    free(hl);    hl = NULL;

    return htslab;
}
/*----------------------------------------------------------*/
const char *get_file_name(cst_utterance *u, const char *idx)
{
    const char *fn=NULL;
    fn=get_param_string(u->features,idx, NULL);
    return fn;
}


int hts_syl_numphonemes(cst_item *s)
{
    if (cst_streq("pau", ffeature_string(s,"name"))) return 0;
    if (cst_streq("1", ffeature_string(s,"syl_final")))
      return ffeature_int(s,"pos_in_syl") + 1;
    else return hts_syl_numphonemes(item_next(s));
}

int hts_p_syl_numphonemes(cst_item *s)
{
    cst_item *p;

    p = item_prev(s);
    if (p == NULL) return 0;
    else if (cst_streq("pau", ffeature_string(p,"name")))
      return hts_p_syl_numphonemes(p);
    else if (cst_streq("1", ffeature_string(p,"syl_final")))
      return ffeature_int(p,"pos_in_syl") + 1;
    else return hts_p_syl_numphonemes(p);
}

int hts_n_syl_numphonemes(cst_item *s)
{
    cst_item *n;

    n = hts_n_syl_phoneme(s);
    if (n == NULL) return 0;
    return hts_syl_numphonemes(n);
}

cst_item *hts_skip_pau_phoneme(cst_item *s)
{
    if (s == NULL) return NULL;
    if (cst_streq("pau", ffeature_string(s,"name")))
      return hts_skip_pau_phoneme(item_next(s));
    else return s;
}

cst_item *hts_n_syl_phoneme(cst_item *s)
{
    if (cst_streq("pau", ffeature_string(s,"name")))
      return hts_skip_pau_phoneme(s);
    if (cst_streq("1", ffeature_string(s,"syl_final"))) {
      return hts_skip_pau_phoneme(item_next(s));
    } else return hts_n_syl_phoneme(item_next(s));
}

cst_item *hts_syl_vowel(cst_item *s)
{
    cst_item *v;
    int i;

    v = s;
    while (!cst_streq("1", ffeature_string(v,"syl_final"))) v = item_next(v);
    for (i = ffeature_int(s,"R:SylStructure.parent.syl_codasize");
	 i > 0; i--) v = item_prev(v);

    return v;
}
 
/*get the time stretch factor from the utterance feature*/
//add by hlwang
float getdurstrch(cst_utterance *u)
{
    float dur_stretch;
    dur_stretch = get_param_float(u->features,"DUR_STRCH_FACTOR", 0.0);
    return dur_stretch;
}

hts_lab *load_htslab(const char *fn)
{
   // cst_item *s;
    hts_lab *htslab = NULL;
    hts_lab *hl, *phl = NULL;
    FILE *fp;
    char aline[1024];
    char *p;

    /* memory allocation */
    htslab = cst_alloc(hts_lab, 1);
    htslab->lab = NULL;	htslab->next = NULL;
    phl = hl = htslab;
    
    if((fp=fopen(fn,"rt"))==NULL)
    {
        printf("ERROR: input lable file %s is invalid\n",fn);
        exit(1);
    }
    
    while(!feof(fp))
    {
        p=fgets(aline,1024,fp);
        if(!p)
        {
        	return NULL;
        }
        aline[strlen(aline)-1]=0x00;
        hl->lab = cst_alloc(char, strlen(aline)+1);
        memmove(hl->lab,aline,strlen(aline));
        phl = hl;
	hl->next = cst_alloc(hts_lab, 1);
	hl = hl->next;	hl->lab = NULL;	hl->next = NULL;
    }
    phl->next = NULL;
    free(hl);    hl = NULL;
    return htslab;
}


const dur_stat *phone_dur_stat(const dur_stats *ds,const char *ph);

/*print the phone duration information*/
/*add by hlwang                                    */
void print_flitephn_dur(cst_utterance *u, const char *fn, float tdur)
{
    float mean[128],zvar[128];
    float var[128], zdur;
    const dur_stat *p_dur_stat;
    cst_item *s;
    const char *curword;
    FILE *fp=NULL;
    const cst_item *pitem=NULL;
    int i,j;
    cst_cart *dur_tree;
    dur_stats *ds;
    if(fn!=NULL)
    {
        if((fp=fopen(fn,"wt"))==NULL)
        {
            printf("HError: cann't open the file %s to write.\n",fn);
            exit(1);
        }
        i=0;


    dur_tree = val_cart(feat_val(u->features,"dur_cart"));
    ds = val_dur_stats(feat_val(u->features,"dur_stats"));
         for (s=relation_head(utt_relation(u,"Segment"));  s ; s=item_next(s)) {
            zdur = val_float(cart_interpret(s,dur_tree));
            p_dur_stat = phone_dur_stat(ds,item_name(s));
                 if (!strcmp("pau", ffeature_string(s,"name")))
                {                 
                    printf("pau\n");
                }
                else
                {
                    if(pitem!=item_parent(item_parent(item_as(s,"SylStructure"))))
                    {
                        if(pitem!=NULL)
                        {
                            fprintf(fp," |");
                            for(j=0;j<i;j++)
                                 fprintf(fp,"%.4f,%.4f,%.4f|",mean[j],var[j],zvar[j]);
                            fprintf(fp,"\n");
                        }
                        curword=ffeature_string(s,"R:SylStructure.parent.parent.name");
                        fprintf(fp,"%s |",curword);
                        pitem=item_parent(item_parent(item_as(s,"SylStructure")));
                        i=0;
                    }
                    fprintf(fp,"%s|",ffeature_string(s,"name"));
                    mean[i]=p_dur_stat->mean;
                    var[i]=p_dur_stat->stddev;
                    zvar[i]=zdur;
                    i++;
                }
         }
         fprintf(fp," |");
         for(j=0;j<i;j++)
            fprintf(fp,"%.4f,%.4f,%.4f|",mean[j],var[j],zvar[j]);
         fprintf(fp,"\nTOTAL: |%.4f|\n",tdur);
         fclose(fp);
    }
}

/*-------------------------------------------------------------------------*/
/*print the phone duration information of HTS*/
/*add by hlwang                                                */
void print_htsphn_dur(cst_utterance *u, hts_lab *lab,  const char *fn, float tdur)
{
    float mean[128];
    float var[128];
    hts_lab *hl;
    cst_item *s;
    const char *curword;
    FILE *fp=NULL;
    const cst_item *pitem=NULL;
    int i,j;
    if(fn!=NULL)
    {
        if((fp=fopen(fn,"wt"))==NULL)
        {
            printf("HError: cann't open the file %s to write.\n",fn);
            exit(1);
        }
        i=0;
         for (s=relation_head(utt_relation(u,"Segment")), hl = lab;  s&&hl ; s=item_next(s),hl = hl->next) {
        	if (!strcmp("pau", ffeature_string(s,"name")))
                {                 
                    printf("pau\n");
                }
                else
                {
                    if(pitem!=item_parent(item_parent(item_as(s,"SylStructure"))))
                    {
                        if(pitem!=NULL)
                        {
                            fprintf(fp," |");
                            for(j=0;j<i;j++)
                                 fprintf(fp,"%.4f,%.4f|",mean[j],var[j]);
                            fprintf(fp,"\n");
                        }
                        curword=ffeature_string(s,"R:SylStructure.parent.parent.name");
                        fprintf(fp,"%s |",curword);
                        pitem=item_parent(item_parent(item_as(s,"SylStructure")));
                        i=0;
                    }
                    fprintf(fp,"%s|",ffeature_string(s,"name"));
                    mean[i]=hl->dur_mean;
                    var[i]=hl->dur_var;
                    i++;
                }
         }
         fprintf(fp," |");
         for(j=0;j<i;j++)
            fprintf(fp,"%.4f,%.4f|",mean[j],var[j]);
         fprintf(fp,"\nTOTAL: |%.4f|\n",tdur);
         fclose(fp);
    }
}

/*----------------------------------------------------------*/
// remove the pause from 
hts_lab *remove_pause(hts_lab *lab)
{
	hts_lab *plab=NULL, *phl;// *hl;
	char *pchr=NULL;
    plab=lab;
    phl=lab;
    while(phl->next->next!=NULL)
    {
    	phl=phl->next;
    }
    pchr=strstr(lab->lab,"x^x-pau+"); 
    if(pchr!=NULL) {
    	plab=plab->next;
    	free(lab);
    }
    pchr=strstr(phl->next->lab,"-pau+x=x"); 
    if(pchr!=NULL) {
    	free(phl->next);
    	phl->next=NULL;
    }
    
    return plab;
}


// remove the pause from: a simpler method
/*hts_lab *remove_pause(hts_lab *lab)
{
	hts_lab *tmplab;
	if(lab && lab->next && lab->next->next)
	{
		tmplab=lab;
		lab=lab->next;
		free(tmplab->lab);
		free(tmplab);

	
    	for (tmplab=lab; tmplab->next->next != NULL; tmplab = tmplab->next); 
  	

		free(tmplab->next->lab);
		free(tmplab->next);
		tmplab->next=NULL;
	}
    return lab;
}*/
