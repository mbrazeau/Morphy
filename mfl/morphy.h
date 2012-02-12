/*
 *  morphy.h
 *  Morphy
 *
 *  Created by Martin Brazeau on 11-04-26.
 *  Copyright 2011. All rights reserved.
 *  Morphy is provided as is with no warranty of any kind.
 *
 *  
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>

//#include <gsl/gsl_rng.h>

#define MORPHY_MAX_STATES 31
#define MAX_OG_SIZE 20
#define MAX_IG_SIZE 500

#define IS_APPLIC (-1^1)

#define TREELIMIT 200 //A temporary tree limit for testing.


/* For node and tree structures, this program follows the format recommended by
 * Felsenstein (2004. Inferring Phylogenies. Sinauer, Mass.) and implemented in 
 * Felsenstein et al.'s 2004. Phylip package. This includes representing 
 * internal nodes as a ring of (minmally) three nodes joined by the next 
 * pointer. The outedge pointer joins the node to either a leaf or the nearest 
 * internal node. */

typedef int32_t charstate;
typedef int64_t taxbipart;

typedef struct node {
    struct node *outedge, *next;
    char *tipname;
    int tip;
    int index;
    int compindex;
    int vweight;
    int initialized;
    int order;
    int nodelen;
    bool start;
    bool skip;
    int minsteps;
    int maxsteps;
    int charstates;
    taxbipart *tipsabove;
    charstate *apomorphies;
} node;

typedef node **nodearray;

typedef struct tree {
    nodearray trnodes;
    /* An array of pointers to nodes. Elements from 0 to ntax - 1 are the terminals. Element
     * ntax is generally reserved for the root, but this is just a convention. All other nodes
     * are internal nodes and not the root node. */
    node *root;
    int length;
    int templen;
    bool swapped;
    taxbipart **bipartitions;
    int index;
} tree;

typedef struct treeset {
    tree **savedtrees;
    int nsaved;
    int bestlen;
} treeset;

typedef struct chardata {
    int charnum;
    charstate *transeries;
    int optim_type;
    int maxvalue;
    int minvalue;
    bool included;
    bool informative;
    double cIndex;
    double rcIndex;
    double retIndex;
    double hIndex;
    int numstates;
    int *stepmatrix[MORPHY_MAX_STATES][MORPHY_MAX_STATES];
    void (*optimzation_algo)(node *n, int *trlength);
} chardata;

/*Function prototypes*/

/*in main.c*/

void call_index(node *n);
void dump_nodearray(nodearray nds, int ntax, int numnodes);
void dump_connections(nodearray nds, int ntax, int numnodes);
void dump_tree(tree *t, int ntax, int numnodes);
void init_taxarray(int *taxarray, int ntax);
void joinNodes(node *n, node *p);
struct tree *alloctree(int ntax, int numnodes);
void freetree(tree *newtree, int numnodes);
struct tree *alloc_noring(int ntax, int numnodes);
struct node * allocnode(void);
void printNewick(node *n);
void treelen(node *n, int *stepcount); // The traversal algorithm that calls fitchdown
void mfl_countsteps(node *leftdesc, node *rightdesc, node *ancestor, int nchar, int *trlength, int *besttreelen);
void mfl_fitch_postorder(node *n, int *trlength, int nchar, int *besttreelen);
void mfl_fitch_preorder(node *n, int *trlength, int nchar, int *besttreelen);
struct tree * copytree(tree *origtree, int ntax, int numnodes); // Calls growcopy to copy a template tree
struct tree * copytree_II(tree *origtree, int ntax, int numnodes);
void growcopy(node *templ, node *target, tree *newtree, int *iter); // Called by copytree. Copies tree in preorder
void newring(node *r1, int ntax);
void deletering(node *r1);
void detree(node *n);
void detree2(nodearray trnptr);
void mfl_point_bottom(node *n, node **nodes, int ntax, int *iteration);
void mfl_root_tree(tree *trtoroot, int root, int ntax);
void unroot(int ntax, tree *rootedtree);
void mfl_apply_tipdata(tree *currenttree, charstate *tipdata, int ntax, int nchar);
void mfl_countsteps(node *leftdesc, node *rightdesc, node *ancestor, int nchar, int *trlength, int *besttreelen);
int mfl_get_treelen(tree *testtree, charstate *tipdata, int ntax, int nchar, int *bestreelen);
int mfl_get_subtreelen(node *n, charstate *tipdata, int ntax, int nchar, int *besttreelen);

/*in compare.c*/
int mfl_count_fields(int ntax);
bool mfl_comp_bipartition(taxbipart *bp1, taxbipart *bp2, int numfields);
bool mfl_compare_alltrees(tree *newtopol, tree **savedtrees, int ntax, int numnodes, long int *current);
void mfl_set_bipartition(node *n, node *d);
void mfl_set_tipsabove(node *n, int numfields, taxbipart **hashtab, int *bpcounter);
void mfl_free_hashtab(taxbipart **hashtab, int numbiparts);
taxbipart **mfl_tree_biparts(tree *t,int ntax, int numnodes);
bool mfl_compare_trees(taxbipart **t1, taxbipart **t2, int ntax, int numfields);
void test_tree_comparison(void);

/*in exhaustive.c*/ 
void allunrooted(void /*tree *treearray, int ntaxa*/);
void insert_allp(node *n, tree *origtree, int taxon, int calln, int *counter);
long long int factorial(long long int n);
long long int numtrees(int ntaxa);

/*in randtree.c*/
void allunrooted(void /*tree *treearray, int ntaxa*/);
void insert_allp(node *n, tree *origtree, int taxon, int calln, int *counter);
long long int factorial(long long int n);
long long int numtrees(int ntaxa);
int mfl_get_sttreelen(tree *testtree, charstate *tipdata, int ntax, int nchar, int *besttreelen);
void mfl_subtree_postorder(node *n, int *trlength, int nchar, int *besttreelen);
struct tree *randrooted (int ntax, int numnodes);
struct tree *randunrooted (int ntax, int numnodes);
void mfl_addseq_randasis(int ntax, int nchar, int numnodes, 
                                 charstate *tipdata, bool addRandom, tree** savedtrees);
/*in taxpart*/
int strToInt (char string[]);
void wipe_Og(int outtaxa[], nodearray outgroup);
void wipe_Ig(int intaxa[], nodearray ingroup);
void defOutgroup(int ntax, int outtaxa[], nodearray outgroup, int intaxa[], nodearray ingroup, bool *OGdefined);

/*in tree.c*/
struct node * mfl_seek_internal(int ntax,int numnodes, node **nds);
struct node * mfl_seek_ringnode(node *n, int ntax);
void mfl_set_vweight(node *n);
void mfl_close_ring(node *n);
void mfl_as_ring(node *n, int ntax);
void mfl_as_noring(node *n);
void mfl_reindex_tree(nodearray nds, int ntax, int numnodes);
void mfl_set_ring_to_n(node *n);
void mfl_reset_ring(node *n);
void mfl_collapse(node *n, nodearray nds);
int mfl_determ_order(node *n);
void mfl_set_order(node *n);
void mfl_clear_order(node *n);
void mfl_set_index(node *n);
void mfl_put_branch_in_ring(node *n, node *rnode);
void mfl_insert_branch(node *br, node *target, int ntax);
void mfl_arb_resolve(node *n, node **nds, int ntax, int numnodes);
void mfl_deinit_tree(tree *t, int numnodes);
void mfl_temproot(tree *trtoroot, int root, int ntax);
void mfl_undo_temproot(int ntax, tree *trtounroot);
int mfl_tree_enumerator(void);
void mfl_resize_treebuffer(tree **treebuffer, int *treelimit, int sizeincrease);
void mfl_clear_treebuffer(tree **treebuffer, long int *numsavedtrees, int numnodes);
void mfl_reinit_treebuffer(tree **treebuffer, tree *newbest, long int *numsavedtrees, int numnodes);


/*in readnewick.c*/
struct node * cpyfromNWK(char *nwktr, int nwklen, int ntax, int numnodes, int *pos, nodearray nds, bool isRooted);
void NWK_roothandl(char *nwktr, int nwklen, int ntax, int numnodes, tree *newtree, bool isRooted);
struct tree * readNWK (char *nwktr, bool isRooted);

/*in rearrange.c*/
void mfl_bswap(node *p, node *q);
void mfl_remove_branch(node *n);
void mfl_insert_branch(node *br, node *target);
void mfl_nni_traversal(node *n, tree *swapingon, tree **savedtrees, int ntax, 
                       int nchar, int numnodes, long int *current, 
                       charstate *tipdata, bool *undertreelimitlong, 
                       int *currentbesttree, bool *foundbettertree);
void mfl_nni_search(int ntax, int nchar, int numnodes, charstate *tipdata, 
                    tree **savedtrees, int starttreelen);
void test_nni(int ntax, int numnodes);
long int mfl_spr_leftotry(int ntax);
void mfl_regrafting_traversal(node *n, node *subtr, tree *swapingon);
void mfl_regrafting_traversal(node *n, node *subtr, tree *swapingon, tree **savedtrees, int ntax, 
                              int nchar, int numnodes, long int *current, 
                              charstate *tipdata, bool *undertreelimit, 
                              int *currentbesttree, bool *foundbettertree, long int *leftotry);
void mfl_pruning_traversal(node *n, tree *swapingon, tree **savedtrees, int ntax, 
                           int nchar, int numnodes, long int *current, 
                           charstate *tipdata, bool *undertreelimit, 
                           int *currentbesttree, bool *foundbettertree, long int *leftotry);
void mfl_spr_search(int ntax, int nchar, int numnodes, charstate *tipdata, 
                    tree **savedtrees, int starttreelen);

/*End function prototypes*/
