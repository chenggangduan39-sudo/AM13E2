/*  ---------------------------------------------------------------  */
/*           The HMM-Based Speech Synthesis System (HTS)             */
/*                       HTS Working Group                           */
/*                                                                   */
/*                  Department of Computer Science                   */
/*                  Nagoya Institute of Technology                   */
/*                               and                                 */
/*   Interdisciplinary Graduate School of Science and Engineering    */
/*                  Tokyo Institute of Technology                    */
/*                                                                   */
/*                     Copyright (c) 2001-2007                       */
/*                       All Rights Reserved.                        */
/*                                                                   */
/*  Permission is hereby granted, free of charge, to use and         */
/*  distribute this software and its documentation without           */
/*  restriction, including without limitation the rights to use,     */
/*  copy, modify, merge, publish, distribute, sublicense, and/or     */
/*  sell copies of this work, and to permit persons to whom this     */
/*  work is furnished to do so, subject to the following conditions: */
/*                                                                   */
/*    1. The source code must retain the above copyright notice,     */
/*       this list of conditions and the following disclaimer.       */
/*                                                                   */
/*    2. Any modifications to the source code must be clearly        */
/*       marked as such.                                             */
/*                                                                   */
/*    3. Redistributions in binary form must reproduce the above     */
/*       copyright notice, this list of conditions and the           */
/*       following disclaimer in the documentation and/or other      */
/*       materials provided with the distribution.  Otherwise, one   */
/*       must contact the HTS working group.                         */
/*                                                                   */
/*  NAGOYA INSTITUTE OF TECHNOLOGY, TOKYO INSTITUTE OF TECHNOLOGY,   */
/*  HTS WORKING GROUP, AND THE CONTRIBUTORS TO THIS WORK DISCLAIM    */
/*  ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL       */
/*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL NAGOYA INSTITUTE OF TECHNOLOGY, TOKYO INSTITUTE OF         */
/*  TECHNOLOGY, HTS WORKING GROUP, NOR THE CONTRIBUTORS BE LIABLE    */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY        */
/*  DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  */
/*  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTUOUS   */
/*  ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR          */
/*  PERFORMANCE OF THIS SOFTWARE.                                    */
/*                                                                   */
/*  ---------------------------------------------------------------  */
/*    model.h: model definition                                      */
/*  ---------------------------------------------------------------  */

/* slightly modified - Tomoki Toda 12/05/08 */
/* pattern trees are accepted */

#ifndef __HTS_MODEL_H
#define __HTS_MODEL_H

#include "hts_tree.h"
#include "hts_mlpg.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

/* Model: structure for individual HMM */
typedef class Model_CLASS {
  public:
    char *name;		/* the name of this HMM */
    int *durpdf;	/* duration pdf index */
    int **lf0pdf;	/* log f0 pdf indexes for each state */
    int **mceppdf;	/* spectral pdf indexes for each state */
    int **bappdf;	/* spectral pdf indexes for each state */
    int *dur;		/* duration for each state */
    int totaldur;	/* total duration */
    double **lf0mean;	/* mean vector of log f0 pdfs for each state */
    double **lf0variance; /* variance vector of log f0 for each state */
    double **mcepmean;	  /* mean vector of mel-cepstrum pdfs for each state */
    double **mcepvariance;/* variance vector of mel-cepstrum for each state */
    double **bapmean;	  /* mean vector of aperiodicity pdfs for each state */
    double **bapvariance; /* variance vector of aperiodicity for each state */
    HTS_Boolean *voiced;  /* voiced/unvoiced decision for each state */
    class Model_CLASS *next;  /* pointer to next HMM */

    Model_CLASS();
    ~Model_CLASS();
    void MemAlloc(char *, int, int, HTS_Boolean);
    void MemFree(int);
} *Model; 

/* UttMode: structure for utterance HMM */
typedef class UttModel_CLASS {
  public:
    Model mhead;
    Model mtail;
    int nModel;     /* # of models for current utterance     */
    int nState;     /* # of HMM states for current utterance */ 
    int totalframe; /* # of frames for current utterance     */

    UttModel_CLASS();
    ~UttModel_CLASS();
} *UttModel;

/* ModelSet: structure for HMM set */
typedef class ModelSet_CLASS {
  private:
    int nstate;               /* # of HMM states for individual HMM */
    int lf0stream;            /* # of stream for log f0 modeling */
    int mcepvsize;            /* vector size for mcep modeling */
    int bapvsize;             /* vector size for bap modeling */
    int lf0gvdim;             /* vector size for log f0 GV modeling */
    int mcepgvdim;            /* vector size for mcep GV modeling */
    int bapgvdim;             /* vector size for bap GV modeling */
    int ndurtree;             /* # of trees (dur) */
    int nlf0gvtree;           /* # of trees (GV of log F0) */
    int nmcepgvtree;          /* # of trees (GV of mcep) */
    int nbapgvtree;           /* # of trees (GV of bap) */
    int *nlf0tree;            /* # of trees for each state position (log F0) */
    int *nmceptree;           /* # of trees for each state position (mcep) */
    int *nbaptree;            /* # of trees for each state position (bap) */
    int *ndurpdf;             /* # of pdfs for duration */
    int *nlf0gvpdf;           /* # of pdfs for log F0 */
    int *nmcepgvpdf;          /* # of pdfs for mcep */
    int *nbapgvpdf;           /* # of pdfs for bap */
    int **nlf0pdf;            /* # of pdfs for each state position (log F0) */
    int **nmceppdf;           /* # of pdfs for each state position (mcep) */
    int **nbappdf;            /* # of pdfs for each state position (bap) */
    double ***durpdf;         /* pdfs for duration */
    double ***lf0gvpdf;       /* pdfs for lf0 */
    double ***mcepgvpdf;      /* pdfs for mcep */
    double ***bapgvpdf;       /* pdfs for bap */
    double ****mceppdf;       /* pdfs for mcep     */
    double ****bappdf;        /* pdfs for bap      */
    double *****lf0pdf;       /* pdfs for lf0      */

    void LoadDURModelSet(TreeSet, FILE *);
    void LoadLF0ModelSet(TreeSet, FILE *);
    void LoadMCPModelSet(TreeSet, FILE *);
    void LoadBAPModelSet(TreeSet, FILE *);
    void LoadLF0GVModelSet(TreeSet, FILE *);
    void LoadMCPGVModelSet(TreeSet, FILE *);
    void LoadBAPGVModelSet(TreeSet, FILE *);
    void ClearDURModelSet();
    void ClearLF0ModelSet();
    void ClearMCPModelSet();
    void ClearBAPModelSet();
    void ClearLF0GVModelSet();
    void ClearMCPGVModelSet();
    void ClearBAPGVModelSet();

  public:
    ModelSet_CLASS(char *, char *, char *, char *,
		   char *, char *, char *, TreeSet ts);
    ~ModelSet_CLASS();
    void FindDURPDF(Model, const double, double *);
    void FindDURPDF(Model, double *, double *);
    void FindLF0PDF (const int, Model, const double);
    void FindMCPPDF (const int, Model);
    void FindBAPPDF (const int, Model);
    void FindLF0GVPDF(GVPDF, int *);
    void FindMCPGVPDF(GVPDF, int *);
    void FindBAPGVPDF(GVPDF, int *);
    HTS_Boolean IsBAP();
    HTS_Boolean IsLF0GV();
    HTS_Boolean IsMCPGV();
    HTS_Boolean IsBAPGV();
    int get_nstate();
    int get_lf0stream();
    int get_mcepvsize();
    int get_bapvsize();
    void ModelAlloc(Model, char *);
    void ModelFree(Model);
    /*add by hlwang*/
    void Find_Dur_Mean_Var(Model m, double *avg, double *val);

#ifdef RESOURSE_FILE_ENCRYPTED
    ModelSet_CLASS(char *, char *, char *, char *,
        char *, char *, char *, TreeSet ts, simp_hash_file_t *);
    void LoadDURModelSet(TreeSet , simp_hash_table_item_t *);
    void LoadLF0ModelSet(TreeSet , simp_hash_table_item_t *);
    void LoadLF0GVModelSet(TreeSet , simp_hash_table_item_t *);
    void LoadMCPModelSet(TreeSet , simp_hash_table_item_t *);
    void LoadMCPGVModelSet(TreeSet , simp_hash_table_item_t *);
    void LoadBAPModelSet(TreeSet , simp_hash_table_item_t *);
    void LoadBAPGVModelSet(TreeSet , simp_hash_table_item_t *);
#endif
} *ModelSet;

#endif /* __HTS_MODEL_H */
/* -------------------- End of "model.h" -------------------- */
