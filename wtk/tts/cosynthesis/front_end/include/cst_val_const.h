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
/*                                                                       */
/*  Const Vals, and macros to define them                                */
/*                                                                       */
/*  Before you give up in disgust bear with me on this.  Every single    */
/*  line in this file has been *very* carefully decided on after much    */
/*  thought, experimentation and reading more specs of the C language    */
/*  than most people even thought existed.  However inspite of that, the */
/*  result is still unsatisfying from an elegance point of view but the  */
/*  given all the constraints this is probably the best compromise.      */
/*                                                                       */
/*  This file offers macros for defining const cst_vals.  I know many    */
/*  are already laughing at me for wanting runtime types on objects and  */
/*  will use this code as exemplars of why this should be done in C++, I */
/*  say good luck to them with their 4M footprint while I go for my      */
/*  50K footprint.  But I *will* do cst_vals in 8 bytes and I *will*     */
/*  have them defined const so they are in the text segment              */
/*                                                                       */
/*  The problem here is that there is not yet a standard way to do       */
/*  initialization of unions.  There is one in the C99 standard and GCC  */
/*  already supports it, but other compilers do not so I can't use that  */
/*                                                                       */
/*  So I need a way to make an object that will have the right 8 bytes   */
/*  for ints, floats, strings and cons cells that will work on any C     */
/*  compiler and will be of type const cst_val.  That unfortunately      */
/*  isn't trivial.                                                       */
/*                                                                       */
/*  For the time being ignoring byte order, and address size, which      */
/*  will be ignored in the particular discuss (though dealt with below)  */
/*  I'd like to do something like                                        */
/*                                                                       */
/*  const cst_val fredi = { CST_VAL_TYPE_INT,-1, 42 };                   */
/*  const cst_val fredf = { CST_VAL_TYPE_FLOAT,-1, 4.2 };                */
/*  const cst_val freds = { CST_VAL_TYPE_STRING,-1, "42" };              */
/*                                                                       */
/*  Even if you accept warnings this isn't going to work, if you add     */
/*  extra {} you can get rid of some warnings but the fval/ival/ *vval   */
/*  part isn't going to work, the compiler *may* try to take the value   */
/*  and assign it using the type of the first field defined in the       */
/*  union.  This could be made to work for ints and void* as pointers    */
/*  can be used as ints (even of 64 bit architectures) but the float     */
/*  isn't going to work.  Casting will cause the float to be changed to  */
/*  an int by CPP which isn't what you want, what you want is that the   */
/*  four byte region gets filled with the four bytes that represent the  */
/*  float itself.  Now you could get the four byte represention of the   */
/*  float and pretend that is an int (0xbfff9a4 for 4.2 on intel), that  */
/*  would work but that doesn't seem satifying and I'd need to have a    */
/*  preprocessor that could convert that.  You could make atoms always   */
/*  have a pointer to another piece of memory, but that would take up    */
/*  another 4 bytes not just for these constants but all other cst_vals  */
/*  create                                                               */
/*                                                                       */
/*  So you could do                                                      */
/*                                                                       */
/*  const cst_val_int fredi = { CST_VAL_TYPE_INT,-1, 42 };               */
/*  const cst_val_float fredf = { CST_VAL_TYPE_FLOAT,-1, 4.2 };          */
/*  const cst_val_string freds = { CST_VAL_TYPE_STRING,-1, "42" };       */
/*                                                                       */
/*  Though that's a slippery slope I don't want to have these explicit   */
/*  new types, the first short defines the type perfectly adequately     */
/*  and the whole point of runtime types is that there is one object     */
/*  type.                                                                */
/*                                                                       */
/*  Well just initialize them at runtime, but, that isn't thread safe,   */
/*  slows down startup, requires a instance implicitly in the code and   */
/*  on the heap. As these are const, they should go in ROM.              */
/*                                                                       */
/*  At this moment, I think the second version is the least problematic  */
/*  though it makes defining val_consts more unpleasant than they should */
/*  be and forces changes elsewhere in the code even when the compiler   */
/*  does support initialization of unions                                */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

/* slightly modified for using HTS - Tomoki Toda 12/05/08 */

#ifndef _CST_VAL_CONSTS_H__
#define _CST_VAL_CONSTS_H__

#include "cst_val_defs.h"

/* There is built-in int to string conversions here for numbers   */
/* up to 20, note if you make this bigger you have to hand change */
/* other things too                                               */

#define CST_CONST_INT_MAX 100

#ifndef NO_UNION_INITIALIZATION

/* This is the simple way when initialization of unions is supported */

#define DEF_CONST_VAL_INT(N,V) const cst_val N = {{.a={.type=CST_VAL_TYPE_INT,.ref_count=-1,.v={.ival=V}}}}
#define DEF_CONST_VAL_STRING(N,S) const cst_val N = {{.a={.type=CST_VAL_TYPE_STRING,.ref_count=-1,.v={.vval= (void *)S}}}}
#define DEF_CONST_VAL_FLOAT(N,F) const cst_val N = {{.a={.type=CST_VAL_TYPE_FLOAT,.ref_count=-1,.v={.fval=F}}}}
#define DEF_CONST_VAL_CONS(N,A,D) const cst_val N = {{.cc={.car=A,.cdr=D }}}

extern const cst_val val_int_0; 
extern const cst_val val_int_1; 
extern const cst_val val_int_2;
extern const cst_val val_int_3;
extern const cst_val val_int_4;
extern const cst_val val_int_5;
extern const cst_val val_int_6;
extern const cst_val val_int_7;
extern const cst_val val_int_8;
extern const cst_val val_int_9;
extern const cst_val val_int_10; 
extern const cst_val val_int_11; 
extern const cst_val val_int_12;
extern const cst_val val_int_13;
extern const cst_val val_int_14;
extern const cst_val val_int_15;
extern const cst_val val_int_16;
extern const cst_val val_int_17;
extern const cst_val val_int_18;
extern const cst_val val_int_19;
extern const cst_val val_int_20; 
extern const cst_val val_int_21; 
extern const cst_val val_int_22;
extern const cst_val val_int_23;
extern const cst_val val_int_24;
extern const cst_val val_int_25;
extern const cst_val val_int_26;
extern const cst_val val_int_27;
extern const cst_val val_int_28;
extern const cst_val val_int_29;
extern const cst_val val_int_30; 
extern const cst_val val_int_31; 
extern const cst_val val_int_32;
extern const cst_val val_int_33;
extern const cst_val val_int_34;
extern const cst_val val_int_35;
extern const cst_val val_int_36;
extern const cst_val val_int_37;
extern const cst_val val_int_38;
extern const cst_val val_int_39;
extern const cst_val val_int_40; 
extern const cst_val val_int_41; 
extern const cst_val val_int_42;
extern const cst_val val_int_43;
extern const cst_val val_int_44;
extern const cst_val val_int_45;
extern const cst_val val_int_46;
extern const cst_val val_int_47;
extern const cst_val val_int_48;
extern const cst_val val_int_49;
extern const cst_val val_int_50; 
extern const cst_val val_int_51; 
extern const cst_val val_int_52;
extern const cst_val val_int_53;
extern const cst_val val_int_54;
extern const cst_val val_int_55;
extern const cst_val val_int_56;
extern const cst_val val_int_57;
extern const cst_val val_int_58;
extern const cst_val val_int_59;
extern const cst_val val_int_60; 
extern const cst_val val_int_61; 
extern const cst_val val_int_62;
extern const cst_val val_int_63;
extern const cst_val val_int_64;
extern const cst_val val_int_65;
extern const cst_val val_int_66;
extern const cst_val val_int_67;
extern const cst_val val_int_68;
extern const cst_val val_int_69;
extern const cst_val val_int_70; 
extern const cst_val val_int_71; 
extern const cst_val val_int_72;
extern const cst_val val_int_73;
extern const cst_val val_int_74;
extern const cst_val val_int_75;
extern const cst_val val_int_76;
extern const cst_val val_int_77;
extern const cst_val val_int_78;
extern const cst_val val_int_79;
extern const cst_val val_int_80; 
extern const cst_val val_int_81; 
extern const cst_val val_int_82;
extern const cst_val val_int_83;
extern const cst_val val_int_84;
extern const cst_val val_int_85;
extern const cst_val val_int_86;
extern const cst_val val_int_87;
extern const cst_val val_int_88;
extern const cst_val val_int_89;
extern const cst_val val_int_90; 
extern const cst_val val_int_91; 
extern const cst_val val_int_92;
extern const cst_val val_int_93;
extern const cst_val val_int_94;
extern const cst_val val_int_95;
extern const cst_val val_int_96;
extern const cst_val val_int_97;
extern const cst_val val_int_98;
extern const cst_val val_int_99;

extern const cst_val val_string_0; 
extern const cst_val val_string_1; 
extern const cst_val val_string_2;
extern const cst_val val_string_3;
extern const cst_val val_string_4;
extern const cst_val val_string_5;
extern const cst_val val_string_6;
extern const cst_val val_string_7;
extern const cst_val val_string_8;
extern const cst_val val_string_9;
extern const cst_val val_string_10; 
extern const cst_val val_string_11; 
extern const cst_val val_string_12;
extern const cst_val val_string_13;
extern const cst_val val_string_14;
extern const cst_val val_string_15;
extern const cst_val val_string_16;
extern const cst_val val_string_17;
extern const cst_val val_string_18;
extern const cst_val val_string_19;
extern const cst_val val_string_20; 
extern const cst_val val_string_21; 
extern const cst_val val_string_22;
extern const cst_val val_string_23;
extern const cst_val val_string_24;
extern const cst_val val_string_25;
extern const cst_val val_string_26;
extern const cst_val val_string_27;
extern const cst_val val_string_28;
extern const cst_val val_string_29;
extern const cst_val val_string_30; 
extern const cst_val val_string_31; 
extern const cst_val val_string_32;
extern const cst_val val_string_33;
extern const cst_val val_string_34;
extern const cst_val val_string_35;
extern const cst_val val_string_36;
extern const cst_val val_string_37;
extern const cst_val val_string_38;
extern const cst_val val_string_39;
extern const cst_val val_string_40; 
extern const cst_val val_string_41; 
extern const cst_val val_string_42;
extern const cst_val val_string_43;
extern const cst_val val_string_44;
extern const cst_val val_string_45;
extern const cst_val val_string_46;
extern const cst_val val_string_47;
extern const cst_val val_string_48;
extern const cst_val val_string_49;
extern const cst_val val_string_50; 
extern const cst_val val_string_51; 
extern const cst_val val_string_52;
extern const cst_val val_string_53;
extern const cst_val val_string_54;
extern const cst_val val_string_55;
extern const cst_val val_string_56;
extern const cst_val val_string_57;
extern const cst_val val_string_58;
extern const cst_val val_string_59;
extern const cst_val val_string_60; 
extern const cst_val val_string_61; 
extern const cst_val val_string_62;
extern const cst_val val_string_63;
extern const cst_val val_string_64;
extern const cst_val val_string_65;
extern const cst_val val_string_66;
extern const cst_val val_string_67;
extern const cst_val val_string_68;
extern const cst_val val_string_69;
extern const cst_val val_string_70; 
extern const cst_val val_string_71; 
extern const cst_val val_string_72;
extern const cst_val val_string_73;
extern const cst_val val_string_74;
extern const cst_val val_string_75;
extern const cst_val val_string_76;
extern const cst_val val_string_77;
extern const cst_val val_string_78;
extern const cst_val val_string_79;
extern const cst_val val_string_80; 
extern const cst_val val_string_81; 
extern const cst_val val_string_82;
extern const cst_val val_string_83;
extern const cst_val val_string_84;
extern const cst_val val_string_85;
extern const cst_val val_string_86;
extern const cst_val val_string_87;
extern const cst_val val_string_88;
extern const cst_val val_string_89;
extern const cst_val val_string_90; 
extern const cst_val val_string_91; 
extern const cst_val val_string_92;
extern const cst_val val_string_93;
extern const cst_val val_string_94;
extern const cst_val val_string_95;
extern const cst_val val_string_96;
extern const cst_val val_string_97;
extern const cst_val val_string_98;
extern const cst_val val_string_99;

#else
/* Only GCC seems to currently support the C99 standard for giving      */
/* explicit names for fields for initializing unions, because we want   */
/* things to be const, and to be small structures this is really useful */
/* thus for compilers not supporting no_union_initization we use other  */
/* structure that we know (hope ?) are the same size and use agressive  */
/* casting. The goal here is wholly justified by the method here isn't  */
/* pretty                                                               */

/* These structures are defined *solely* to get round initialization    */
/* problems if you need to use these in any code you are using your are */
/* unquestionably doing the wrong thing                                 */
typedef struct cst_val_atom_struct_float {
#ifdef WORDS_BIGENDIAN
    short ref_count;
    short type;  /* order is here important */
#else
    short type;  /* order is here important */
    short ref_count;
#endif
    float fval;
} cst_val_float;

typedef struct cst_val_atom_struct_int {
#ifdef WORDS_BIGENDIAN
    short ref_count;
    short type;  /* order is here important (and unintuitive) */
#else
    short type;  /* order is here important */
    short ref_count;
#endif
    int ival;
} cst_val_int;

typedef struct cst_val_atom_struct_void {
#ifdef WORDS_BIGENDIAN
    short ref_count;
    short type;  /* order is here important */
#else
    short type;  /* order is here important */
    short ref_count;
#endif
    void *vval;
} cst_val_void;

#ifdef WORDS_BIGENDIAN
#define DEF_CONST_VAL_INT(N,V) const cst_val_int N={-1, CST_VAL_TYPE_INT, V}
#define DEF_CONST_VAL_STRING(N,S) const cst_val_void N={-1,CST_VAL_TYPE_STRING,(void *)S}
#define DEF_CONST_VAL_FLOAT(N,F) const cst_val_float N={-1,CST_VAL_TYPE_FLOAT,(float)F}
#else
#define DEF_CONST_VAL_INT(N,V) const cst_val_int N={CST_VAL_TYPE_INT,-1,V}
#define DEF_CONST_VAL_STRING(N,S) const cst_val_void N={CST_VAL_TYPE_STRING,-1,(void *)S}
#define DEF_CONST_VAL_FLOAT(N,F) const cst_val_float N={CST_VAL_TYPE_FLOAT,-1,(float)F}
#endif
#define DEF_CONST_VAL_CONS(N,A,D) const cst_val_cons N={A,D}

/* in the non-union intialization version we these consts have to be */
/* more typed than need, we'll cast the back later                   */
extern const cst_val_int val_int_0; 
extern const cst_val_int val_int_1; 
extern const cst_val_int val_int_2;
extern const cst_val_int val_int_3;
extern const cst_val_int val_int_4;
extern const cst_val_int val_int_5;
extern const cst_val_int val_int_6;
extern const cst_val_int val_int_7;
extern const cst_val_int val_int_8;
extern const cst_val_int val_int_9;
extern const cst_val_int val_int_10; 
extern const cst_val_int val_int_11; 
extern const cst_val_int val_int_12;
extern const cst_val_int val_int_13;
extern const cst_val_int val_int_14;
extern const cst_val_int val_int_15;
extern const cst_val_int val_int_16;
extern const cst_val_int val_int_17;
extern const cst_val_int val_int_18;
extern const cst_val_int val_int_19;
extern const cst_val_int val_int_20; 
extern const cst_val_int val_int_21; 
extern const cst_val_int val_int_22;
extern const cst_val_int val_int_23;
extern const cst_val_int val_int_24;
extern const cst_val_int val_int_25;
extern const cst_val_int val_int_26;
extern const cst_val_int val_int_27;
extern const cst_val_int val_int_28;
extern const cst_val_int val_int_29;
extern const cst_val_int val_int_30; 
extern const cst_val_int val_int_31; 
extern const cst_val_int val_int_32;
extern const cst_val_int val_int_33;
extern const cst_val_int val_int_34;
extern const cst_val_int val_int_35;
extern const cst_val_int val_int_36;
extern const cst_val_int val_int_37;
extern const cst_val_int val_int_38;
extern const cst_val_int val_int_39;
extern const cst_val_int val_int_40; 
extern const cst_val_int val_int_41; 
extern const cst_val_int val_int_42;
extern const cst_val_int val_int_43;
extern const cst_val_int val_int_44;
extern const cst_val_int val_int_45;
extern const cst_val_int val_int_46;
extern const cst_val_int val_int_47;
extern const cst_val_int val_int_48;
extern const cst_val_int val_int_49;
extern const cst_val_int val_int_50; 
extern const cst_val_int val_int_51; 
extern const cst_val_int val_int_52;
extern const cst_val_int val_int_53;
extern const cst_val_int val_int_54;
extern const cst_val_int val_int_55;
extern const cst_val_int val_int_56;
extern const cst_val_int val_int_57;
extern const cst_val_int val_int_58;
extern const cst_val_int val_int_59;
extern const cst_val_int val_int_60; 
extern const cst_val_int val_int_61; 
extern const cst_val_int val_int_62;
extern const cst_val_int val_int_63;
extern const cst_val_int val_int_64;
extern const cst_val_int val_int_65;
extern const cst_val_int val_int_66;
extern const cst_val_int val_int_67;
extern const cst_val_int val_int_68;
extern const cst_val_int val_int_69;
extern const cst_val_int val_int_70; 
extern const cst_val_int val_int_71; 
extern const cst_val_int val_int_72;
extern const cst_val_int val_int_73;
extern const cst_val_int val_int_74;
extern const cst_val_int val_int_75;
extern const cst_val_int val_int_76;
extern const cst_val_int val_int_77;
extern const cst_val_int val_int_78;
extern const cst_val_int val_int_79;
extern const cst_val_int val_int_80; 
extern const cst_val_int val_int_81; 
extern const cst_val_int val_int_82;
extern const cst_val_int val_int_83;
extern const cst_val_int val_int_84;
extern const cst_val_int val_int_85;
extern const cst_val_int val_int_86;
extern const cst_val_int val_int_87;
extern const cst_val_int val_int_88;
extern const cst_val_int val_int_89;
extern const cst_val_int val_int_90; 
extern const cst_val_int val_int_91; 
extern const cst_val_int val_int_92;
extern const cst_val_int val_int_93;
extern const cst_val_int val_int_94;
extern const cst_val_int val_int_95;
extern const cst_val_int val_int_96;
extern const cst_val_int val_int_97;
extern const cst_val_int val_int_98;
extern const cst_val_int val_int_99;


extern const cst_val_void val_string_0; 
extern const cst_val_void val_string_1; 
extern const cst_val_void val_string_2;
extern const cst_val_void val_string_3;
extern const cst_val_void val_string_4;
extern const cst_val_void val_string_5;
extern const cst_val_void val_string_6;
extern const cst_val_void val_string_7;
extern const cst_val_void val_string_8;
extern const cst_val_void val_string_9;
extern const cst_val_void val_string_10; 
extern const cst_val_void val_string_11; 
extern const cst_val_void val_string_12;
extern const cst_val_void val_string_13;
extern const cst_val_void val_string_14;
extern const cst_val_void val_string_15;
extern const cst_val_void val_string_16;
extern const cst_val_void val_string_17;
extern const cst_val_void val_string_18;
extern const cst_val_void val_string_19;
extern const cst_val_void val_string_20; 
extern const cst_val_void val_string_21; 
extern const cst_val_void val_string_22;
extern const cst_val_void val_string_23;
extern const cst_val_void val_string_24;
extern const cst_val_void val_string_25;
extern const cst_val_void val_string_26;
extern const cst_val_void val_string_27;
extern const cst_val_void val_string_28;
extern const cst_val_void val_string_29;
extern const cst_val_void val_string_30; 
extern const cst_val_void val_string_31; 
extern const cst_val_void val_string_32;
extern const cst_val_void val_string_33;
extern const cst_val_void val_string_34;
extern const cst_val_void val_string_35;
extern const cst_val_void val_string_36;
extern const cst_val_void val_string_37;
extern const cst_val_void val_string_38;
extern const cst_val_void val_string_39;
extern const cst_val_void val_string_40; 
extern const cst_val_void val_string_41; 
extern const cst_val_void val_string_42;
extern const cst_val_void val_string_43;
extern const cst_val_void val_string_44;
extern const cst_val_void val_string_45;
extern const cst_val_void val_string_46;
extern const cst_val_void val_string_47;
extern const cst_val_void val_string_48;
extern const cst_val_void val_string_49;
extern const cst_val_void val_string_50; 
extern const cst_val_void val_string_51; 
extern const cst_val_void val_string_52;
extern const cst_val_void val_string_53;
extern const cst_val_void val_string_54;
extern const cst_val_void val_string_55;
extern const cst_val_void val_string_56;
extern const cst_val_void val_string_57;
extern const cst_val_void val_string_58;
extern const cst_val_void val_string_59;
extern const cst_val_void val_string_60; 
extern const cst_val_void val_string_61; 
extern const cst_val_void val_string_62;
extern const cst_val_void val_string_63;
extern const cst_val_void val_string_64;
extern const cst_val_void val_string_65;
extern const cst_val_void val_string_66;
extern const cst_val_void val_string_67;
extern const cst_val_void val_string_68;
extern const cst_val_void val_string_69;
extern const cst_val_void val_string_70; 
extern const cst_val_void val_string_71; 
extern const cst_val_void val_string_72;
extern const cst_val_void val_string_73;
extern const cst_val_void val_string_74;
extern const cst_val_void val_string_75;
extern const cst_val_void val_string_76;
extern const cst_val_void val_string_77;
extern const cst_val_void val_string_78;
extern const cst_val_void val_string_79;
extern const cst_val_void val_string_80; 
extern const cst_val_void val_string_81; 
extern const cst_val_void val_string_82;
extern const cst_val_void val_string_83;
extern const cst_val_void val_string_84;
extern const cst_val_void val_string_85;
extern const cst_val_void val_string_86;
extern const cst_val_void val_string_87;
extern const cst_val_void val_string_88;
extern const cst_val_void val_string_89;
extern const cst_val_void val_string_90; 
extern const cst_val_void val_string_91; 
extern const cst_val_void val_string_92;
extern const cst_val_void val_string_93;
extern const cst_val_void val_string_94;
extern const cst_val_void val_string_95;
extern const cst_val_void val_string_96;
extern const cst_val_void val_string_97;
extern const cst_val_void val_string_98;
extern const cst_val_void val_string_99;

#endif

#define DEF_STATIC_CONST_VAL_INT(N,V) static DEF_CONST_VAL_INT(N,V)
#define DEF_STATIC_CONST_VAL_STRING(N,S) static DEF_CONST_VAL_STRING(N,S)
#define DEF_STATIC_CONST_VAL_FLOAT(N,F) static DEF_CONST_VAL_FLOAT(N,F)
#define DEF_STATIC_CONST_VAL_CONS(N,A,D) static DEF_CONST_VAL_CONS(N,A,D)

/* Some actual val consts */
/* The have casts as in the non-union intialize case the casts are necessary */
/* but in the union initial case these casts are harmless                    */

#define VAL_INT_0 (cst_val *)&val_int_0
#define VAL_INT_1 (cst_val *)&val_int_1
#define VAL_INT_2 (cst_val *)&val_int_2
#define VAL_INT_3 (cst_val *)&val_int_3
#define VAL_INT_4 (cst_val *)&val_int_4
#define VAL_INT_5 (cst_val *)&val_int_5
#define VAL_INT_6 (cst_val *)&val_int_6
#define VAL_INT_7 (cst_val *)&val_int_7
#define VAL_INT_8 (cst_val *)&val_int_8
#define VAL_INT_9 (cst_val *)&val_int_9
#define VAL_INT_10 (cst_val *)&val_int_10
#define VAL_INT_11 (cst_val *)&val_int_11
#define VAL_INT_12 (cst_val *)&val_int_12
#define VAL_INT_13 (cst_val *)&val_int_13
#define VAL_INT_14 (cst_val *)&val_int_14
#define VAL_INT_15 (cst_val *)&val_int_15
#define VAL_INT_16 (cst_val *)&val_int_16
#define VAL_INT_17 (cst_val *)&val_int_17
#define VAL_INT_18 (cst_val *)&val_int_18
#define VAL_INT_19 (cst_val *)&val_int_19
#define VAL_INT_20 (cst_val *)&val_int_20
#define VAL_INT_21 (cst_val *)&val_int_21
#define VAL_INT_22 (cst_val *)&val_int_22
#define VAL_INT_23 (cst_val *)&val_int_23
#define VAL_INT_24 (cst_val *)&val_int_24
#define VAL_INT_25 (cst_val *)&val_int_25
#define VAL_INT_26 (cst_val *)&val_int_26
#define VAL_INT_27 (cst_val *)&val_int_27
#define VAL_INT_28 (cst_val *)&val_int_28
#define VAL_INT_29 (cst_val *)&val_int_29
#define VAL_INT_30 (cst_val *)&val_int_30
#define VAL_INT_31 (cst_val *)&val_int_31
#define VAL_INT_32 (cst_val *)&val_int_32
#define VAL_INT_33 (cst_val *)&val_int_33
#define VAL_INT_34 (cst_val *)&val_int_34
#define VAL_INT_35 (cst_val *)&val_int_35
#define VAL_INT_36 (cst_val *)&val_int_36
#define VAL_INT_37 (cst_val *)&val_int_37
#define VAL_INT_38 (cst_val *)&val_int_38
#define VAL_INT_39 (cst_val *)&val_int_39
#define VAL_INT_40 (cst_val *)&val_int_40
#define VAL_INT_41 (cst_val *)&val_int_41
#define VAL_INT_42 (cst_val *)&val_int_42
#define VAL_INT_43 (cst_val *)&val_int_43
#define VAL_INT_44 (cst_val *)&val_int_44
#define VAL_INT_45 (cst_val *)&val_int_45
#define VAL_INT_46 (cst_val *)&val_int_46
#define VAL_INT_47 (cst_val *)&val_int_47
#define VAL_INT_48 (cst_val *)&val_int_48
#define VAL_INT_49 (cst_val *)&val_int_49
#define VAL_INT_50 (cst_val *)&val_int_50
#define VAL_INT_51 (cst_val *)&val_int_51
#define VAL_INT_52 (cst_val *)&val_int_52
#define VAL_INT_53 (cst_val *)&val_int_53
#define VAL_INT_54 (cst_val *)&val_int_54
#define VAL_INT_55 (cst_val *)&val_int_55
#define VAL_INT_56 (cst_val *)&val_int_56
#define VAL_INT_57 (cst_val *)&val_int_57
#define VAL_INT_58 (cst_val *)&val_int_58
#define VAL_INT_59 (cst_val *)&val_int_59
#define VAL_INT_60 (cst_val *)&val_int_60
#define VAL_INT_61 (cst_val *)&val_int_61
#define VAL_INT_62 (cst_val *)&val_int_62
#define VAL_INT_63 (cst_val *)&val_int_63
#define VAL_INT_64 (cst_val *)&val_int_64
#define VAL_INT_65 (cst_val *)&val_int_65
#define VAL_INT_66 (cst_val *)&val_int_66
#define VAL_INT_67 (cst_val *)&val_int_67
#define VAL_INT_68 (cst_val *)&val_int_68
#define VAL_INT_69 (cst_val *)&val_int_69
#define VAL_INT_70 (cst_val *)&val_int_70
#define VAL_INT_71 (cst_val *)&val_int_71
#define VAL_INT_72 (cst_val *)&val_int_72
#define VAL_INT_73 (cst_val *)&val_int_73
#define VAL_INT_74 (cst_val *)&val_int_74
#define VAL_INT_75 (cst_val *)&val_int_75
#define VAL_INT_76 (cst_val *)&val_int_76
#define VAL_INT_77 (cst_val *)&val_int_77
#define VAL_INT_78 (cst_val *)&val_int_78
#define VAL_INT_79 (cst_val *)&val_int_79
#define VAL_INT_80 (cst_val *)&val_int_80
#define VAL_INT_81 (cst_val *)&val_int_81
#define VAL_INT_82 (cst_val *)&val_int_82
#define VAL_INT_83 (cst_val *)&val_int_83
#define VAL_INT_84 (cst_val *)&val_int_84
#define VAL_INT_85 (cst_val *)&val_int_85
#define VAL_INT_86 (cst_val *)&val_int_86
#define VAL_INT_87 (cst_val *)&val_int_87
#define VAL_INT_88 (cst_val *)&val_int_88
#define VAL_INT_89 (cst_val *)&val_int_89
#define VAL_INT_90 (cst_val *)&val_int_90
#define VAL_INT_91 (cst_val *)&val_int_91
#define VAL_INT_92 (cst_val *)&val_int_92
#define VAL_INT_93 (cst_val *)&val_int_93
#define VAL_INT_94 (cst_val *)&val_int_94
#define VAL_INT_95 (cst_val *)&val_int_95
#define VAL_INT_96 (cst_val *)&val_int_96
#define VAL_INT_97 (cst_val *)&val_int_97
#define VAL_INT_98 (cst_val *)&val_int_98
#define VAL_INT_99 (cst_val *)&val_int_99

const cst_val *val_int_n(int n);

#define VAL_STRING_0 (cst_val *)&val_string_0
#define VAL_STRING_1 (cst_val *)&val_string_1
#define VAL_STRING_2 (cst_val *)&val_string_2
#define VAL_STRING_3 (cst_val *)&val_string_3
#define VAL_STRING_4 (cst_val *)&val_string_4
#define VAL_STRING_5 (cst_val *)&val_string_5
#define VAL_STRING_6 (cst_val *)&val_string_6
#define VAL_STRING_7 (cst_val *)&val_string_7
#define VAL_STRING_8 (cst_val *)&val_string_8
#define VAL_STRING_9 (cst_val *)&val_string_9
#define VAL_STRING_10 (cst_val *)&val_string_10
#define VAL_STRING_11 (cst_val *)&val_string_11
#define VAL_STRING_12 (cst_val *)&val_string_12
#define VAL_STRING_13 (cst_val *)&val_string_13
#define VAL_STRING_14 (cst_val *)&val_string_14
#define VAL_STRING_15 (cst_val *)&val_string_15
#define VAL_STRING_16 (cst_val *)&val_string_16
#define VAL_STRING_17 (cst_val *)&val_string_17
#define VAL_STRING_18 (cst_val *)&val_string_18
#define VAL_STRING_19 (cst_val *)&val_string_19
#define VAL_STRING_20 (cst_val *)&val_string_20
#define VAL_STRING_21 (cst_val *)&val_string_21
#define VAL_STRING_22 (cst_val *)&val_string_22
#define VAL_STRING_23 (cst_val *)&val_string_23
#define VAL_STRING_24 (cst_val *)&val_string_24
#define VAL_STRING_25 (cst_val *)&val_string_25
#define VAL_STRING_26 (cst_val *)&val_string_26
#define VAL_STRING_27 (cst_val *)&val_string_27
#define VAL_STRING_28 (cst_val *)&val_string_28
#define VAL_STRING_29 (cst_val *)&val_string_29
#define VAL_STRING_30 (cst_val *)&val_string_30
#define VAL_STRING_31 (cst_val *)&val_string_31
#define VAL_STRING_32 (cst_val *)&val_string_32
#define VAL_STRING_33 (cst_val *)&val_string_33
#define VAL_STRING_34 (cst_val *)&val_string_34
#define VAL_STRING_35 (cst_val *)&val_string_35
#define VAL_STRING_36 (cst_val *)&val_string_36
#define VAL_STRING_37 (cst_val *)&val_string_37
#define VAL_STRING_38 (cst_val *)&val_string_38
#define VAL_STRING_39 (cst_val *)&val_string_39
#define VAL_STRING_40 (cst_val *)&val_string_40
#define VAL_STRING_41 (cst_val *)&val_string_41
#define VAL_STRING_42 (cst_val *)&val_string_42
#define VAL_STRING_43 (cst_val *)&val_string_43
#define VAL_STRING_44 (cst_val *)&val_string_44
#define VAL_STRING_45 (cst_val *)&val_string_45
#define VAL_STRING_46 (cst_val *)&val_string_46
#define VAL_STRING_47 (cst_val *)&val_string_47
#define VAL_STRING_48 (cst_val *)&val_string_48
#define VAL_STRING_49 (cst_val *)&val_string_49
#define VAL_STRING_50 (cst_val *)&val_string_50
#define VAL_STRING_51 (cst_val *)&val_string_51
#define VAL_STRING_52 (cst_val *)&val_string_52
#define VAL_STRING_53 (cst_val *)&val_string_53
#define VAL_STRING_54 (cst_val *)&val_string_54
#define VAL_STRING_55 (cst_val *)&val_string_55
#define VAL_STRING_56 (cst_val *)&val_string_56
#define VAL_STRING_57 (cst_val *)&val_string_57
#define VAL_STRING_58 (cst_val *)&val_string_58
#define VAL_STRING_59 (cst_val *)&val_string_59
#define VAL_STRING_60 (cst_val *)&val_string_60
#define VAL_STRING_61 (cst_val *)&val_string_61
#define VAL_STRING_62 (cst_val *)&val_string_62
#define VAL_STRING_63 (cst_val *)&val_string_63
#define VAL_STRING_64 (cst_val *)&val_string_64
#define VAL_STRING_65 (cst_val *)&val_string_65
#define VAL_STRING_66 (cst_val *)&val_string_66
#define VAL_STRING_67 (cst_val *)&val_string_67
#define VAL_STRING_68 (cst_val *)&val_string_68
#define VAL_STRING_69 (cst_val *)&val_string_69
#define VAL_STRING_70 (cst_val *)&val_string_70
#define VAL_STRING_71 (cst_val *)&val_string_71
#define VAL_STRING_72 (cst_val *)&val_string_72
#define VAL_STRING_73 (cst_val *)&val_string_73
#define VAL_STRING_74 (cst_val *)&val_string_74
#define VAL_STRING_75 (cst_val *)&val_string_75
#define VAL_STRING_76 (cst_val *)&val_string_76
#define VAL_STRING_77 (cst_val *)&val_string_77
#define VAL_STRING_78 (cst_val *)&val_string_78
#define VAL_STRING_79 (cst_val *)&val_string_79
#define VAL_STRING_80 (cst_val *)&val_string_80
#define VAL_STRING_81 (cst_val *)&val_string_81
#define VAL_STRING_82 (cst_val *)&val_string_82
#define VAL_STRING_83 (cst_val *)&val_string_83
#define VAL_STRING_84 (cst_val *)&val_string_84
#define VAL_STRING_85 (cst_val *)&val_string_85
#define VAL_STRING_86 (cst_val *)&val_string_86
#define VAL_STRING_87 (cst_val *)&val_string_87
#define VAL_STRING_88 (cst_val *)&val_string_88
#define VAL_STRING_89 (cst_val *)&val_string_89
#define VAL_STRING_90 (cst_val *)&val_string_90
#define VAL_STRING_91 (cst_val *)&val_string_91
#define VAL_STRING_92 (cst_val *)&val_string_92
#define VAL_STRING_93 (cst_val *)&val_string_93
#define VAL_STRING_94 (cst_val *)&val_string_94
#define VAL_STRING_95 (cst_val *)&val_string_95
#define VAL_STRING_96 (cst_val *)&val_string_96
#define VAL_STRING_97 (cst_val *)&val_string_97
#define VAL_STRING_98 (cst_val *)&val_string_98
#define VAL_STRING_99 (cst_val *)&val_string_99

const cst_val *val_string_n(int n);


#endif
