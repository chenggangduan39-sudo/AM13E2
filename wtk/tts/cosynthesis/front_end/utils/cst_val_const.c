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
/*  Without a real garbage collector we need some basic constant val     */
/*  to avoid having to create them on the fly                            */
/*                                                                       */
/*************************************************************************/
#include "cst_val.h"
#include "cst_features.h"
#include "cst_string.h"

/* modified by Tomki Toda on 25 Mar. 2008 */

DEF_CONST_VAL_STRING(val_string_0,"0");
DEF_CONST_VAL_STRING(val_string_1,"1");
DEF_CONST_VAL_STRING(val_string_2,"2");
DEF_CONST_VAL_STRING(val_string_3,"3");
DEF_CONST_VAL_STRING(val_string_4,"4");
DEF_CONST_VAL_STRING(val_string_5,"5");
DEF_CONST_VAL_STRING(val_string_6,"6");
DEF_CONST_VAL_STRING(val_string_7,"7");
DEF_CONST_VAL_STRING(val_string_8,"8");
DEF_CONST_VAL_STRING(val_string_9,"9");
DEF_CONST_VAL_STRING(val_string_10,"10");
DEF_CONST_VAL_STRING(val_string_11,"11");
DEF_CONST_VAL_STRING(val_string_12,"12");
DEF_CONST_VAL_STRING(val_string_13,"13");
DEF_CONST_VAL_STRING(val_string_14,"14");
DEF_CONST_VAL_STRING(val_string_15,"15");
DEF_CONST_VAL_STRING(val_string_16,"16");
DEF_CONST_VAL_STRING(val_string_17,"17");
DEF_CONST_VAL_STRING(val_string_18,"18");
DEF_CONST_VAL_STRING(val_string_19,"19");
DEF_CONST_VAL_STRING(val_string_20,"20");
DEF_CONST_VAL_STRING(val_string_21,"21");
DEF_CONST_VAL_STRING(val_string_22,"22");
DEF_CONST_VAL_STRING(val_string_23,"23");
DEF_CONST_VAL_STRING(val_string_24,"24");
DEF_CONST_VAL_STRING(val_string_25,"25");
DEF_CONST_VAL_STRING(val_string_26,"26");
DEF_CONST_VAL_STRING(val_string_27,"27");
DEF_CONST_VAL_STRING(val_string_28,"28");
DEF_CONST_VAL_STRING(val_string_29,"29");
DEF_CONST_VAL_STRING(val_string_30,"30");
DEF_CONST_VAL_STRING(val_string_31,"31");
DEF_CONST_VAL_STRING(val_string_32,"32");
DEF_CONST_VAL_STRING(val_string_33,"33");
DEF_CONST_VAL_STRING(val_string_34,"34");
DEF_CONST_VAL_STRING(val_string_35,"35");
DEF_CONST_VAL_STRING(val_string_36,"36");
DEF_CONST_VAL_STRING(val_string_37,"37");
DEF_CONST_VAL_STRING(val_string_38,"38");
DEF_CONST_VAL_STRING(val_string_39,"39");
DEF_CONST_VAL_STRING(val_string_40,"40");
DEF_CONST_VAL_STRING(val_string_41,"41");
DEF_CONST_VAL_STRING(val_string_42,"42");
DEF_CONST_VAL_STRING(val_string_43,"43");
DEF_CONST_VAL_STRING(val_string_44,"44");
DEF_CONST_VAL_STRING(val_string_45,"45");
DEF_CONST_VAL_STRING(val_string_46,"46");
DEF_CONST_VAL_STRING(val_string_47,"47");
DEF_CONST_VAL_STRING(val_string_48,"48");
DEF_CONST_VAL_STRING(val_string_49,"49");
DEF_CONST_VAL_STRING(val_string_50,"50");
DEF_CONST_VAL_STRING(val_string_51,"51");
DEF_CONST_VAL_STRING(val_string_52,"52");
DEF_CONST_VAL_STRING(val_string_53,"53");
DEF_CONST_VAL_STRING(val_string_54,"54");
DEF_CONST_VAL_STRING(val_string_55,"55");
DEF_CONST_VAL_STRING(val_string_56,"56");
DEF_CONST_VAL_STRING(val_string_57,"57");
DEF_CONST_VAL_STRING(val_string_58,"58");
DEF_CONST_VAL_STRING(val_string_59,"59");
DEF_CONST_VAL_STRING(val_string_60,"60");
DEF_CONST_VAL_STRING(val_string_61,"61");
DEF_CONST_VAL_STRING(val_string_62,"62");
DEF_CONST_VAL_STRING(val_string_63,"63");
DEF_CONST_VAL_STRING(val_string_64,"64");
DEF_CONST_VAL_STRING(val_string_65,"65");
DEF_CONST_VAL_STRING(val_string_66,"66");
DEF_CONST_VAL_STRING(val_string_67,"67");
DEF_CONST_VAL_STRING(val_string_68,"68");
DEF_CONST_VAL_STRING(val_string_69,"69");
DEF_CONST_VAL_STRING(val_string_70,"70");
DEF_CONST_VAL_STRING(val_string_71,"71");
DEF_CONST_VAL_STRING(val_string_72,"72");
DEF_CONST_VAL_STRING(val_string_73,"73");
DEF_CONST_VAL_STRING(val_string_74,"74");
DEF_CONST_VAL_STRING(val_string_75,"75");
DEF_CONST_VAL_STRING(val_string_76,"76");
DEF_CONST_VAL_STRING(val_string_77,"77");
DEF_CONST_VAL_STRING(val_string_78,"78");
DEF_CONST_VAL_STRING(val_string_79,"79");
DEF_CONST_VAL_STRING(val_string_80,"80");
DEF_CONST_VAL_STRING(val_string_81,"81");
DEF_CONST_VAL_STRING(val_string_82,"82");
DEF_CONST_VAL_STRING(val_string_83,"83");
DEF_CONST_VAL_STRING(val_string_84,"84");
DEF_CONST_VAL_STRING(val_string_85,"85");
DEF_CONST_VAL_STRING(val_string_86,"86");
DEF_CONST_VAL_STRING(val_string_87,"87");
DEF_CONST_VAL_STRING(val_string_88,"88");
DEF_CONST_VAL_STRING(val_string_89,"89");
DEF_CONST_VAL_STRING(val_string_90,"90");
DEF_CONST_VAL_STRING(val_string_91,"91");
DEF_CONST_VAL_STRING(val_string_92,"92");
DEF_CONST_VAL_STRING(val_string_93,"93");
DEF_CONST_VAL_STRING(val_string_94,"94");
DEF_CONST_VAL_STRING(val_string_95,"95");
DEF_CONST_VAL_STRING(val_string_96,"96");
DEF_CONST_VAL_STRING(val_string_97,"97");
DEF_CONST_VAL_STRING(val_string_98,"98");
DEF_CONST_VAL_STRING(val_string_99,"99");

DEF_CONST_VAL_INT(val_int_0,0);
DEF_CONST_VAL_INT(val_int_1,1);
DEF_CONST_VAL_INT(val_int_2,2);
DEF_CONST_VAL_INT(val_int_3,3);
DEF_CONST_VAL_INT(val_int_4,4);
DEF_CONST_VAL_INT(val_int_5,5);
DEF_CONST_VAL_INT(val_int_6,6);
DEF_CONST_VAL_INT(val_int_7,7);
DEF_CONST_VAL_INT(val_int_8,8);
DEF_CONST_VAL_INT(val_int_9,9);
DEF_CONST_VAL_INT(val_int_10,10);
DEF_CONST_VAL_INT(val_int_11,11);
DEF_CONST_VAL_INT(val_int_12,12);
DEF_CONST_VAL_INT(val_int_13,13);
DEF_CONST_VAL_INT(val_int_14,14);
DEF_CONST_VAL_INT(val_int_15,15);
DEF_CONST_VAL_INT(val_int_16,16);
DEF_CONST_VAL_INT(val_int_17,17);
DEF_CONST_VAL_INT(val_int_18,18);
DEF_CONST_VAL_INT(val_int_19,19);
DEF_CONST_VAL_INT(val_int_20,20);
DEF_CONST_VAL_INT(val_int_21,21);
DEF_CONST_VAL_INT(val_int_22,22);
DEF_CONST_VAL_INT(val_int_23,23);
DEF_CONST_VAL_INT(val_int_24,24);
DEF_CONST_VAL_INT(val_int_25,25);
DEF_CONST_VAL_INT(val_int_26,26);
DEF_CONST_VAL_INT(val_int_27,27);
DEF_CONST_VAL_INT(val_int_28,28);
DEF_CONST_VAL_INT(val_int_29,29);
DEF_CONST_VAL_INT(val_int_30,30);
DEF_CONST_VAL_INT(val_int_31,31);
DEF_CONST_VAL_INT(val_int_32,32);
DEF_CONST_VAL_INT(val_int_33,33);
DEF_CONST_VAL_INT(val_int_34,34);
DEF_CONST_VAL_INT(val_int_35,35);
DEF_CONST_VAL_INT(val_int_36,36);
DEF_CONST_VAL_INT(val_int_37,37);
DEF_CONST_VAL_INT(val_int_38,38);
DEF_CONST_VAL_INT(val_int_39,39);
DEF_CONST_VAL_INT(val_int_40,40);
DEF_CONST_VAL_INT(val_int_41,41);
DEF_CONST_VAL_INT(val_int_42,42);
DEF_CONST_VAL_INT(val_int_43,43);
DEF_CONST_VAL_INT(val_int_44,44);
DEF_CONST_VAL_INT(val_int_45,45);
DEF_CONST_VAL_INT(val_int_46,46);
DEF_CONST_VAL_INT(val_int_47,47);
DEF_CONST_VAL_INT(val_int_48,48);
DEF_CONST_VAL_INT(val_int_49,49);
DEF_CONST_VAL_INT(val_int_50,50);
DEF_CONST_VAL_INT(val_int_51,51);
DEF_CONST_VAL_INT(val_int_52,52);
DEF_CONST_VAL_INT(val_int_53,53);
DEF_CONST_VAL_INT(val_int_54,54);
DEF_CONST_VAL_INT(val_int_55,55);
DEF_CONST_VAL_INT(val_int_56,56);
DEF_CONST_VAL_INT(val_int_57,57);
DEF_CONST_VAL_INT(val_int_58,58);
DEF_CONST_VAL_INT(val_int_59,59);
DEF_CONST_VAL_INT(val_int_60,60);
DEF_CONST_VAL_INT(val_int_61,61);
DEF_CONST_VAL_INT(val_int_62,62);
DEF_CONST_VAL_INT(val_int_63,63);
DEF_CONST_VAL_INT(val_int_64,64);
DEF_CONST_VAL_INT(val_int_65,65);
DEF_CONST_VAL_INT(val_int_66,66);
DEF_CONST_VAL_INT(val_int_67,67);
DEF_CONST_VAL_INT(val_int_68,68);
DEF_CONST_VAL_INT(val_int_69,69);
DEF_CONST_VAL_INT(val_int_70,70);
DEF_CONST_VAL_INT(val_int_71,71);
DEF_CONST_VAL_INT(val_int_72,72);
DEF_CONST_VAL_INT(val_int_73,73);
DEF_CONST_VAL_INT(val_int_74,74);
DEF_CONST_VAL_INT(val_int_75,75);
DEF_CONST_VAL_INT(val_int_76,76);
DEF_CONST_VAL_INT(val_int_77,77);
DEF_CONST_VAL_INT(val_int_78,78);
DEF_CONST_VAL_INT(val_int_79,79);
DEF_CONST_VAL_INT(val_int_80,80);
DEF_CONST_VAL_INT(val_int_81,81);
DEF_CONST_VAL_INT(val_int_82,82);
DEF_CONST_VAL_INT(val_int_83,83);
DEF_CONST_VAL_INT(val_int_84,84);
DEF_CONST_VAL_INT(val_int_85,85);
DEF_CONST_VAL_INT(val_int_86,86);
DEF_CONST_VAL_INT(val_int_87,87);
DEF_CONST_VAL_INT(val_int_88,88);
DEF_CONST_VAL_INT(val_int_89,89);
DEF_CONST_VAL_INT(val_int_90,90);
DEF_CONST_VAL_INT(val_int_91,91);
DEF_CONST_VAL_INT(val_int_92,92);
DEF_CONST_VAL_INT(val_int_93,93);
DEF_CONST_VAL_INT(val_int_94,94);
DEF_CONST_VAL_INT(val_int_95,95);
DEF_CONST_VAL_INT(val_int_96,96);
DEF_CONST_VAL_INT(val_int_97,97);
DEF_CONST_VAL_INT(val_int_98,98);
DEF_CONST_VAL_INT(val_int_99,99);

static const int val_int_const_max = 100;
static const cst_val * const val_int_const [] = {
    VAL_INT_0,
    VAL_INT_1,
    VAL_INT_2,
    VAL_INT_3,
    VAL_INT_4,
    VAL_INT_5,
    VAL_INT_6,
    VAL_INT_7,
    VAL_INT_8,
    VAL_INT_9,
    VAL_INT_10,
    VAL_INT_11,
    VAL_INT_12,
    VAL_INT_13,
    VAL_INT_14,
    VAL_INT_15,
    VAL_INT_16,
    VAL_INT_17,
    VAL_INT_18,
    VAL_INT_19,
    VAL_INT_20,
    VAL_INT_21,
    VAL_INT_22,
    VAL_INT_23,
    VAL_INT_24,
    VAL_INT_25,
    VAL_INT_26,
    VAL_INT_27,
    VAL_INT_28,
    VAL_INT_29,
    VAL_INT_30,
    VAL_INT_31,
    VAL_INT_32,
    VAL_INT_33,
    VAL_INT_34,
    VAL_INT_35,
    VAL_INT_36,
    VAL_INT_37,
    VAL_INT_38,
    VAL_INT_39,
    VAL_INT_40,
    VAL_INT_41,
    VAL_INT_42,
    VAL_INT_43,
    VAL_INT_44,
    VAL_INT_45,
    VAL_INT_46,
    VAL_INT_47,
    VAL_INT_48,
    VAL_INT_49,
    VAL_INT_50,
    VAL_INT_51,
    VAL_INT_52,
    VAL_INT_53,
    VAL_INT_54,
    VAL_INT_55,
    VAL_INT_56,
    VAL_INT_57,
    VAL_INT_58,
    VAL_INT_59,
    VAL_INT_60,
    VAL_INT_61,
    VAL_INT_62,
    VAL_INT_63,
    VAL_INT_64,
    VAL_INT_65,
    VAL_INT_66,
    VAL_INT_67,
    VAL_INT_68,
    VAL_INT_69,
    VAL_INT_70,
    VAL_INT_71,
    VAL_INT_72,
    VAL_INT_73,
    VAL_INT_74,
    VAL_INT_75,
    VAL_INT_76,
    VAL_INT_77,
    VAL_INT_78,
    VAL_INT_79,
    VAL_INT_80,
    VAL_INT_81,
    VAL_INT_82,
    VAL_INT_83,
    VAL_INT_84,
    VAL_INT_85,
    VAL_INT_86,
    VAL_INT_87,
    VAL_INT_88,
    VAL_INT_89,
    VAL_INT_90,
    VAL_INT_91,
    VAL_INT_92,
    VAL_INT_93,
    VAL_INT_94,
    VAL_INT_95,
    VAL_INT_96,
    VAL_INT_97,
    VAL_INT_98,
    VAL_INT_99 };

static const cst_val * const val_string_const [] = {
    VAL_STRING_0,
    VAL_STRING_1,
    VAL_STRING_2,
    VAL_STRING_3,
    VAL_STRING_4,
    VAL_STRING_5,
    VAL_STRING_6,
    VAL_STRING_7,
    VAL_STRING_8,
    VAL_STRING_9,
    VAL_STRING_10,
    VAL_STRING_11,
    VAL_STRING_12,
    VAL_STRING_13,
    VAL_STRING_14,
    VAL_STRING_15,
    VAL_STRING_16,
    VAL_STRING_17,
    VAL_STRING_18,
    VAL_STRING_19,
    VAL_STRING_20,
    VAL_STRING_21,
    VAL_STRING_22,
    VAL_STRING_23,
    VAL_STRING_24,
    VAL_STRING_25,
    VAL_STRING_26,
    VAL_STRING_27,
    VAL_STRING_28,
    VAL_STRING_29,
    VAL_STRING_30,
    VAL_STRING_31,
    VAL_STRING_32,
    VAL_STRING_33,
    VAL_STRING_34,
    VAL_STRING_35,
    VAL_STRING_36,
    VAL_STRING_37,
    VAL_STRING_38,
    VAL_STRING_39,
    VAL_STRING_40,
    VAL_STRING_41,
    VAL_STRING_42,
    VAL_STRING_43,
    VAL_STRING_44,
    VAL_STRING_45,
    VAL_STRING_46,
    VAL_STRING_47,
    VAL_STRING_48,
    VAL_STRING_49,
    VAL_STRING_50,
    VAL_STRING_51,
    VAL_STRING_52,
    VAL_STRING_53,
    VAL_STRING_54,
    VAL_STRING_55,
    VAL_STRING_56,
    VAL_STRING_57,
    VAL_STRING_58,
    VAL_STRING_59,
    VAL_STRING_60,
    VAL_STRING_61,
    VAL_STRING_62,
    VAL_STRING_63,
    VAL_STRING_64,
    VAL_STRING_65,
    VAL_STRING_66,
    VAL_STRING_67,
    VAL_STRING_68,
    VAL_STRING_69,
    VAL_STRING_70,
    VAL_STRING_71,
    VAL_STRING_72,
    VAL_STRING_73,
    VAL_STRING_74,
    VAL_STRING_75,
    VAL_STRING_76,
    VAL_STRING_77,
    VAL_STRING_78,
    VAL_STRING_79,
    VAL_STRING_80,
    VAL_STRING_81,
    VAL_STRING_82,
    VAL_STRING_83,
    VAL_STRING_84,
    VAL_STRING_85,
    VAL_STRING_86,
    VAL_STRING_87,
    VAL_STRING_88,
    VAL_STRING_89,
    VAL_STRING_90,
    VAL_STRING_91,
    VAL_STRING_92,
    VAL_STRING_93,
    VAL_STRING_94,
    VAL_STRING_95,
    VAL_STRING_96,
    VAL_STRING_97,
    VAL_STRING_98,
    VAL_STRING_99 };
  
const cst_val *val_int_n(int n)
{
    if (n < val_int_const_max)
	return val_int_const[n];
    else
	return val_int_const[val_int_const_max-1];
}

/* carts are pretty confused about strings/ints, and some features */
/* are actually used as floats and as int/strings                  */
const cst_val *val_string_n(int n)
{
    if (n < val_int_const_max) /* yes I mean *int*, its the table size */
	return val_string_const[n];
    else
	return val_string_const[val_int_const_max-1];
}

#if 0
/* This technique isn't thread safe, so I replaced it with val_consts */
static cst_features *val_string_consts = NULL;

const cst_val *val_string_x(const char *n)
{
    const cst_val *v;

    /* *BUG* This will have to be fixed soon */
    if (val_string_consts == NULL)
	val_string_consts = new_features();
    
    v = feat_val(val_string_consts,n);
    if (v)
	return v;
    else
    {
	feat_set_string(val_string_consts,n,n);
	return feat_val(val_string_consts,n);
    }
}
#endif
