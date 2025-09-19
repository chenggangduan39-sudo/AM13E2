/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 1999-2000                          */
/*                        All Rights Reserved.                           */
/*                                                                       `*/
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
/*               Date:  December 2000                                    */
/*************************************************************************/
/*                                                                       */
/*  A simple voice defintion                                             */
/*                                                                       */
/*************************************************************************/

/* slightly modified for using HTS - Tomoki Toda 12/05/08 */

#include "flite.h"
#include "cst_diphone.h"
#include "./wtk/tts/cosynthesis/front_end/lang/usenglish/usenglish.h"
#include "./wtk/tts/cosynthesis/front_end/lang/cmulex/cmulex.h"
#include "hts_wrap.h"

static cst_utterance *cmu_us_timix_postlex(cst_utterance *u);
extern cst_diphone_db cmu_us_timix_db;

/*cst_voice *cmu_us_timix_hts = NULL;*/


cst_voice *register_cmu_us_timix(const char *voxdir) 
{
    cst_voice *v = new_voice();
    
    /* Set up basic values for synthesizing with this voice */
    usenglish_init(v);
    feat_set_string(v->features,"name","cmu_us_timix_hts");

    /* Lexicon */
    cmu_lex_init();
    feat_set(v->features,"lexicon",lexicon_val(&cmu_lex));

    /* Intonation */
    feat_set_float(v->features,"int_f0_target_mean",95.0);
    feat_set_float(v->features,"int_f0_target_stddev",11.0);
    /* Post lexical rules */
    // feat_set(v->features,"postlex_func",uttfunc_val(&cmu_us_timix_postlex));

    /* Waveform synthesis: hts_synth */
    // feat_set(v->features,"wave_synth_func", uttfunc_val((cst_uttfunc)&hts_synth));

    /* HTS voice directory */
    if (voxdir != NULL) {
        v->htsvoicedir = cst_alloc(char, strlen(voxdir) + 1);
	strcpy(v->htsvoicedir, voxdir);
    } else v->htsvoicedir = NULL;

    /*cmu_us_timix_hts = v;*/

    return v; /*cmu_us_timix_hts;*/
}

cst_voice *register_cmu_us_timix_ais() 
{
    cst_voice *v = new_voice();

    /* Set up basic values for synthesizing with this voice */
    usenglish_init(v);
    feat_set_string(v->features,"name","cmu_us_timix_hts");

    /* Lexicon */
    cmu_lex_init();
    feat_set(v->features,"lexicon",lexicon_val(&cmu_lex));

    /* Intonation */
    feat_set_float(v->features,"int_f0_target_mean",95.0);
    feat_set_float(v->features,"int_f0_target_stddev",11.0);
    /* Post lexical rules */
    feat_set(v->features,"postlex_func",uttfunc_val(&cmu_us_timix_postlex));

    /* Waveform synthesis: hts_synth */
    // feat_set(v->features,"wave_synth_func", uttfunc_val((cst_uttfunc)&hts_synth));

    /*cmu_us_timix_hts = v;*/

    return v; /*cmu_us_timix_hts;*/
}


void delete_voice_glb(cst_voice *v);

void unregister_cmu_us_timix(cst_voice *v)
{
    /*if (v != cmu_us_timix_hts)
	return;*/
    /*delete_voice(v);*/
    delete_voice_glb(v);
}

static void fix_ah(cst_utterance *u)
{
    /* This should really be done in the index itself */
    const cst_item *s;

    for (s=relation_head(utt_relation(u,"Segment")); s; s=item_next(s))
	if (cst_streq(item_feat_string(s,"name"),"ah"))
	    item_set_string(s,"name","aa");
}

static cst_utterance *cmu_us_timix_postlex(cst_utterance *u)
{
    us_postlex(u);
    fix_ah(u);

    return u;
}
