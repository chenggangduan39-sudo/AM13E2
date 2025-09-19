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

/* slightly modified - Tomoki Toda 12/05/08 */

#ifndef __HTS_ENGINE_H
#define __HTS_ENGINE_H

#include "hts_global.h"
#include "hts_model.h"
#include "hts_mlpg.h"
#include "hts_vocoder.h"
#include "cst_hts.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

#define INFTY   ((double) 1.0e+38)
#define INFTY2  ((double) 1.0e+19)
#define INVINF  ((double) 1.0e-38)
#define INVINF2 ((double) 1.0e-19)

typedef class HTSVoice_CLASS {
  private:
    ModelSet ms;
    TreeSet ts;
    DWin dwinlf0;
    DWin dwinmcp;
    DWin dwinbap;
    GVPDF gvlf0;
    GVPDF gvmcp;
    GVPDF gvbap;

    char* makefilename(const char *, const char *);

#ifdef RESOURSE_FILE_ENCRYPTED
    char* makefilename(const char *name);

    void init(char *, char *, char *, char *, char *, char *, char *,
        char *, char *, char *, char *, char *, char *, char *,
        char *, char *, char *, simp_hash_file_t *);
#endif

    void init(char *, char *, char *, char *, char *, char *, char *,
	      char *, char *, char *, char *, char *, char *, char *,
	      char *, char *, char *);
    void init(char *, char *, char *, char *,
	      char *, char *, char *, char *,
	      char *, char *, char *,
	      char *, char *, char *);
    void OutLabel(FILE *, UttModel) ;
    void OutInfo(FILE *, UttModel);
    double finv(const double);
    DVECTOR pdf2speech(UttModel, FILE *, FILE *, FILE *, FILE *);
    void check_dir(char *);
    void writessignal_wav(char *, short *, long, double);
    void SetDurationPDF(Model, UttModel, double *, double *, double *);
    void SetOutputPDFs(Model, UttModel);
    void SetCDGVPDFs(Model);
    void ResetCDGVPDFs();
    void GetDur(UttModel, hts_lab *);

  public:
    globalP gp;

    HTSVoice_CLASS(char *, char *, char *, char *,
		   char *, char *, char *, char *,
		   char *, char *, char *,
		   char *, char *, char *);
    HTSVoice_CLASS(char *);
#ifdef RESOURSE_FILE_ENCRYPTED
    HTSVoice_CLASS(char *vdir, void*/*no use*/);
#endif

    ~HTSVoice_CLASS();
    void HTS_Process(FILE *, FILE *, FILE *, FILE *, FILE *, FILE *, FILE *,
		     char *);
    void HTS_SetDurStrch(float strch);
    DVECTOR HTS_Process(hts_lab *);
} *HTSVoice;

#endif /* _HTS_ENGINE_H */
/* -------------------- End of "hts_engine.h" -------------------- */
