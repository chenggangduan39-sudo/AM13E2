/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2001                             */
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

#ifndef _CST_HTS_H__
#define _CST_HTS_H__

#include "cst_file.h"
#include "cst_val.h"
#include "cst_features.h"
#include "cst_wave.h"
#include "cst_track.h"
#include "cst_sts.h"
#include "cst_hrg.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct hts_lab_struct hts_lab;

struct hts_lab_struct {
    char *lab;
    float dur;
    float dur_mean;
    float dur_var;
    hts_lab *next;
};

void set_wave(cst_utterance *, double *, int, int);
void put_htslabdur(cst_utterance *, hts_lab *);
void free_htslab(hts_lab *hl);
//void print_htslab(hts_lab *hl);
hts_lab *get_htslab(cst_utterance *u);
void mod_htslab(hts_lab *, char *);
int hts_syl_numphonemes(cst_item *s);
int hts_p_syl_numphonemes(cst_item *s);
int hts_n_syl_numphonemes(cst_item *s);
cst_item *hts_skip_pau_phoneme(cst_item *s);
cst_item *hts_n_syl_phoneme(cst_item *s);
cst_item *hts_syl_vowel(cst_item *s);

/*---------------------------------------------------------------------------*/
//add by hlwang
void print_htslab(hts_lab *hl, const char *filename);
float getdurstrch(cst_utterance *u);
void print_htsphn_dur(cst_utterance *u,hts_lab *hl, const char *filename, float tdur);
const char *get_file_name(cst_utterance *u, const char *idx);
void print_flitephn_dur(cst_utterance *u, const char *fn, float tdur);
hts_lab *load_htslab(const char *fn);
hts_lab *remove_pause(hts_lab *lab);
#ifdef __cplusplus
}
#endif

#endif
