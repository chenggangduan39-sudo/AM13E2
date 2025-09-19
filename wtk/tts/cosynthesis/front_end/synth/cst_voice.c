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
/*               Date:  December 2000                                    */
/*************************************************************************/
/*                                                                       */
/*    Voice definition                                                   */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_voice.h"

/* slightly modified for using HTS - Tomoki Toda 12/05/08 */

cst_voice *new_voice()
{
    cst_voice *v = cst_alloc(struct cst_voice_struct,1);
    v->features = new_features();
    v->ffunctions = new_features();

    /* for HTS */
    v->htsvoicedir = NULL;

    /* for pitch by dmd at 2016.07.01*/
    wtk_pitch_cfg_init(&(v->pitch_cfg));
    v->pitch = wtk_pitch_new(&(v->pitch_cfg));
    v->pitch_shit = 0.0;
    return v;
}

void delete_voice(cst_voice *v)
{
    if (v)
    {
	delete_features(v->features);
	delete_features(v->ffunctions);
	/* for HTS */
	if (v->htsvoicedir != NULL) cst_free(v->htsvoicedir);

	/* for pitch by dmd at 2016.07.01 */
	wtk_pitch_cfg_clean(&(v->pitch_cfg));
	if (v->pitch){
		wtk_pitch_delete(v->pitch);
	}

	cst_free(v);
    }
}

void delete_features_glb(cst_features *f);

void delete_voice_glb(cst_voice *v)
{
    if (v)
    {
	delete_features_glb(v->features);
	delete_features_glb(v->ffunctions);
	/* for HTS */
	if (v->htsvoicedir != NULL) cst_free(v->htsvoicedir);

	/* for pitch by dmd at 2016.07.01 */
	wtk_pitch_cfg_clean(&(v->pitch_cfg));
	if (v->pitch){
		wtk_pitch_delete(v->pitch);
	}

	cst_free(v);
    }
}

