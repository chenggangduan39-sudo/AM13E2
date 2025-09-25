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
/*    tree.h: decision tree definition                               */
/*  ---------------------------------------------------------------  */

/* slightly modified - Tomoki Toda 12/05/08 */
/* pattern trees are accepted */

#ifndef __HTS_TREE_H
#define __HTS_TREE_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

/* Pattern: structure for pattern */
typedef class Pattern_CLASS {
  public:
    char *pat;			/* pattern */
    class Pattern_CLASS *next;	/* link to next pattern */

    Pattern_CLASS();
    ~Pattern_CLASS();
    HTS_Boolean DPMatch(char *, char *, const int, const int);
    HTS_Boolean PMatch(char *);
} *Pattern;


/* Question: structure for questions */
typedef class Question_CLASS {
  private:
    char *qName;		/* name of this question */
    Pattern phead;		/* link to head of pattern list */
    Pattern ptail;		/* link to tail of pattern list */
    
  public:
    class Question_CLASS *next;/* link to next question */

    Question_CLASS();
    ~Question_CLASS();
    void LoadQuestions(FILE *);
    HTS_Boolean IsqName(char *);
    HTS_Boolean QMatch(char *);

#ifdef RESOURSE_FILE_ENCRYPTED
    void LoadQuestions(char **);
#endif
} *Question;


/* Node: structure for node of decision tree */
typedef class Node_CLASS {
  public:
    int idx;		/* index of this node */
    int pdf;		/* index of pdf for this node (leaf node only) */
    class Node_CLASS *yes;	/* link to child node (yes) */
    class Node_CLASS *no;	/* link to child node (no)  */
    class Node_CLASS *next;	/* link to next node  */  
    Question quest;		/* question applied at this node */

    Node_CLASS();
    ~Node_CLASS();
} *Node;


/* Tree: structure for each decision tree */
typedef class Tree_CLASS {
  private:
    int state;		/* state position of this tree */
    Node root;		/* root node of this decision tree */
    Node leaf;		/* leaf nodes of this decision tree */
    Pattern phead;	/* link to head of pattern list for this tree */
    Pattern ptail;	/* link to tail of pattern list for this tree */

    void ParseTreePat(char *);
    int name2num(char *);
    Node FindNode(Node, const int);
    Question FindQuestion(char *, Question, Question);
    int SearchTree(char *, Node);
    int SearchTree(char *);
    HTS_Boolean IsNum(char *);

  public:
    class Tree_CLASS *next;	/* link to next tree */

    Tree_CLASS();
    ~Tree_CLASS();
    HTS_Boolean IsTree(char *);
    HTS_Boolean IsState(int);
    void LoadTree(FILE *, Question, Question);
    int SearchPTree(char *);

#ifdef RESOURSE_FILE_ENCRYPTED
    void LoadTree(char **, Question, Question);
#endif
} *Tree;


/* TreeSet: structure for decision tree set */
typedef class TreeSet_CLASS {
  private:
    Question qhead[HTS_NUMMTYPE]; /* question lists for mcp, lf0, and dur */
    Question qtail[HTS_NUMMTYPE];
    Tree thead[HTS_NUMMTYPE];     /* tree lists for mcp, lf0, and dur */
    Tree ttail[HTS_NUMMTYPE];
    int nTrees[HTS_NUMMTYPE];      /* # of trees for mcp, lf0, and dur */

    void LoadTreeSet(const HTS_Mtype, FILE *fp);
    void ClearTreeSet(const HTS_Mtype);

  public:
    TreeSet_CLASS(char *, char *, char *, char *, char *, char *, char *);
    ~TreeSet_CLASS();
    int NumTree(int, const HTS_Mtype);
    int* SearchPTree(char *, int, const HTS_Mtype);

#ifdef RESOURSE_FILE_ENCRYPTED
    TreeSet_CLASS(char *, char *, char *, char *, char *, char *, char *, simp_hash_file_t *);
    void LoadTreeSet(const HTS_Mtype, simp_hash_table_item_t *);
#endif
} *TreeSet;

#endif /* __HTS_TREE_H */
/* -------------------- End of "tree.h" -------------------- */

