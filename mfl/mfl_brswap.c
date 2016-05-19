/*
 *  mfl_brswap.c
 *
 *  THE MORPHY FUNCTION LIBRARY
 *  A library for phylogenetic analysis with emphasis on parsimony and
 *  morphology (but someday other methods)
 *
 *  Copyright (C) 2016  by Martin D. Brazeau, Thomas Guillerme,
 *  and Chris Desjardins
 *
 *  Created by Martin Brazeau and Chris Desjardins on 1/26/12.
 *  Some data structs, routines and ideas derived from:
 *      - The PHYLIP package by Joe Felsenstein
 *          <http://evolution.genetics.washington.edu/phylip.html>
 *      - MrBayes by John Huelsenbeck and Fredrik Ronquist
 *          <http://mrbayes.sourceforge.net/>
 *
 *  Any bugs, errors, inefficiences and general amateurish handling are our own
 *  and most likely the responsibility of MDB. We make no guarantees.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include "morphy.h"

void mfl_temp_nodeweight_set(mfl_node_t* n)
{
    mfl_node_t *p = NULL;
    int weightcount = 0;
    
    if (n->nodet_tip) {
        n->nodet_isbottom = true;
        return;
    }
    
    p = n->nodet_next;
    do {
        mfl_temp_nodeweight_set(p->nodet_edge);
        weightcount += p->nodet_edge->nodet_weight;
        p = p->nodet_next;
    } while (p != n);
    
    n->nodet_weight = weightcount;
    n->nodet_isbottom = true;
    
    return;
}


/*!
 @discussion function for initialising the neigbhour rule counter
 @param num_nodes (int) the number of nodes to get the clade sizes from.
 */
bool mfl_neighbour_rule_counter_init(int num_nodes)
{
    bool clade_counter[num_nodes];
    int i = 0;
    
    // Set all clades to 0 (not visited yet)
    for (i=1; i <= num_nodes; ++i) {
        clade_counter[i] = 0;
    }
    
    return clade_counter;
}

/*!
 @discussion function for appending the clade_counter
 @param clade_counter (bool) an array of booleans.
 @param clade_size_visited (int) the size of a clade visited.
 */
void mfl_neighbour_rule_counter_append(bool* clade_counter, int clade_size_visited)
{
    // Set the visited clade to 1
    if(clade_counter[clade_size_visited-1] != 1) { //TG: the -1 is to start from origin (0)
        clade_counter[clade_size_visited-1] = 1;
    }
}


void mfl_save_topology(mfl_tree_t* t, mfl_treebuffer_t* trbuf, mfl_searchrec_t* searchrec)
{
    mfl_tree_t* topolcopy = mfl_copy_tree_topology(t);
    mfl_append_tree_to_treebuffer(topolcopy, trbuf, searchrec->sr_handle_ptr);
}


void mfl_temp_rebranching(mfl_node_t* src, mfl_node_t* tgt, mfl_cliprec_t* regraft)
{
    regraft->tgt1 = tgt;
    regraft->tgt2 = tgt->nodet_edge;
    regraft->src1 = src;
    
    if (src->nodet_next->nodet_edge) {
        regraft->src2 = src->nodet_next->nodet_next;
    }
    else {
        regraft->src2 = src->nodet_next;
        assert(!regraft->src2->nodet_edge);
    }
    
    mfl_join_node_edges(regraft->src1, regraft->tgt1);
    mfl_join_node_edges(regraft->src2, regraft->tgt2);
}


void mfl_undo_temp_rebranching(mfl_cliprec_t* regraft)
{
    regraft->src1->nodet_edge = NULL;
    regraft->src2->nodet_edge = NULL;
    mfl_join_node_edges(regraft->tgt1, regraft->tgt2);
}


void mfl_regrafting_traversal(mfl_node_t* n, mfl_node_t* src, mfl_searchrec_t* searchrec)
{
    // Traverse the target tree, attempting the reinsertion
    
    mfl_node_t* p = n->nodet_next;
    mfl_cliprec_t regraft;
    
    // Insert branch
    mfl_temp_rebranching(src, n, &regraft);
    
    // Copy the tree, append it to the buffer
    mfl_save_topology(searchrec->sr_swaping_on, searchrec->sr_treebuffer, searchrec);
    
    // Count the number of rearrangements
    mfl_undo_temp_rebranching(&regraft);
    
    
    ++searchrec->sr_rearrangement_counter;
    
    if (n->nodet_tip) {
        return;
    }
    
    if (p->nodet_edge) {
        mfl_regrafting_traversal(p->nodet_edge, src, searchrec);
    }
    p = p->nodet_next;
    
    if (p->nodet_edge) {
        mfl_regrafting_traversal(p->nodet_edge, src, searchrec);
    }
    
    
    return;
}

void mfl_regraft_subtree(mfl_node_t* src, mfl_node_t* tgt, mfl_searchrec_t* searchrec)
{
    int weight = 0;
    
    mfl_node_t* tgt_opp = tgt->nodet_edge;
    
    // This might be where we will insert a condition to enforce the neighborhood rule
    //if (!tgt->nodet_tip) {
        mfl_regrafting_traversal(tgt, src, searchrec);
    //}
    
    //if (!tgt_opp->nodet_tip) {
        mfl_regrafting_traversal(tgt_opp, src, searchrec);
    //}
}

mfl_node_t* mfl_clip_branch(mfl_node_t* n, mfl_cliprec_t* cliprec)
{
    mfl_node_t* retnode = NULL;
    
    cliprec->src1 = n->nodet_edge->nodet_next;
    cliprec->src2 = cliprec->src1->nodet_next;
    cliprec->tgt1 = cliprec->src1->nodet_edge;
    cliprec->tgt2 = cliprec->src2->nodet_edge;
    
    if (cliprec->src1->nodet_isbottom) {
        retnode = cliprec->src1;
    } else {
        retnode = cliprec->src2;
    }
    
    mfl_disconnect_node(cliprec->src1);
    mfl_disconnect_node(cliprec->src2);
    
    mfl_join_node_edges(cliprec->tgt1, cliprec->tgt2);
    
    return retnode;
}


void mfl_restore_branching(mfl_cliprec_t* cliprec)
{
    mfl_join_node_edges(cliprec->src1, cliprec->tgt1);
    mfl_join_node_edges(cliprec->src2, cliprec->tgt2);
}


void mfl_pruning_traversal(mfl_node_t* n, mfl_searchrec_t* searchrec)
{
    int i = 0;
    int weightcount = 0;
    mfl_node_t* p = NULL;
    mfl_node_t* q = NULL;
    mfl_cliprec_t cliprec;
    
    if (n->nodet_tip) {
        return;
    }
    
    q = n->nodet_next;
    do {
        if (i < 2) {
            mfl_pruning_traversal(q->nodet_edge, searchrec);
            p = mfl_clip_branch(q->nodet_edge, &cliprec);
            n->nodet_weight += q->nodet_edge->nodet_weight;
        }
        else {
            p = mfl_clip_branch(n->nodet_edge , &cliprec);
        }
        
        mfl_regraft_subtree(p, cliprec.tgt1, searchrec);
        mfl_restore_branching(&cliprec);

        dbg_printf("src weight: %i\n", q->nodet_edge->nodet_weight);
        q = q->nodet_next;
        ++i;
    } while (i < 3);
    
    //n->nodet_weight = weightcount;
    dbg_printf("Weight: %i\n", n->nodet_weight);
    
    return;
}

bool mfl_check_node_wt(int weight, mfl_nodeweights_t* ndweightlist)
{
    assert(!(weight > ndweightlist->nwl_max));
    
    if (ndweightlist->nwl_weights_list[weight-1]) {
        dbg_printf("This weight has been seen before\n");
        return true;
    }
    else {
        return false;
    }
}

void mfl_set_node_wt(bool value, int weight, mfl_nodeweights_t* ndweightlist)
{
    assert(!(weight > ndweightlist->nwl_max));
    
    ndweightlist->nwl_weights_list[weight-1] = value;
}

mfl_nodeweights_t* mfl_create_node_wt_list(int num_taxa)
{
    
    int i = 0;
    int num_weights = 0;
    mfl_nodeweights_t* newnwl = (mfl_nodeweights_t*)mfl_malloc(sizeof(mfl_nodeweights_t), 0, __FXN_NAME__);
    
    num_weights = num_taxa - 3; // The number of useful weights in branch breaking
    
    newnwl->nwl_max = num_weights;
    
    newnwl->nwl_weights_list = (bool*)mfl_malloc(num_weights * sizeof(bool), 0, __FXN_NAME__);
    
    return newnwl;
}


void mfl_destroy_node_wt_list(mfl_nodeweights_t* nodewtlst)
{
    free(nodewtlst->nwl_weights_list);
    free(nodewtlst);
}

void tui_spr_test_environment(void)
{
    // NOTE: This is a tui function and should be moved there.
    
    int temp_max_trees = 1000;
    
    mfl_handle_s* testhandle = mfl_t2s(mfl_create_handle());
    
    //char* cliptesttree = "temp_examp6=[&R] (((((1,4),5),3),2),(6,(7,((8,9),(10,(11,12))))));";
    //char* cliptesttree = "temp_examp6=[&R] ((1,2),3);";
    //char* cliptesttree = "temp_examp6=[&R] ((1,2),(3,4));";
    char* cliptesttree = "temp_examp6=[&R] ((1,2),(3,(4,5)));";
    //char* cliptesttree = "temp_examp6=[&R] ((1,(2,(6,7))),(3,(4,5)));";
    //char* cliptesttree = "temp_examp6=[&R] ((1,((2,8),(6,7))),(3,(4,5)));";
    
    mfl_tree_t* testree = mfl_convert_newick_to_mfl_tree_t(cliptesttree, 0);
    
    char* treedraw = mfl_drawtree(testree);
    dbg_printf("%s", treedraw);
    
    testhandle->n_taxa = testree->treet_num_taxa;
    
    mfl_searchrec_t* searchrec = mfl_create_searchrec(testhandle);
    
    mfl_unroot_tree(testree);
    
    if (testree->treet_root) {
        searchrec->sr_swap_entry = testree->treet_root;
    } else {
        searchrec->sr_swap_entry = testree->treet_start;
    }
    
    //mfl_temp_nodeweight_set(searchrec->sr_swap_entry);
    
    searchrec->sr_treebuffer = mfl_alloc_treebuffer(temp_max_trees);
    searchrec->sr_swaping_on = testree;
    searchrec->sr_handle_ptr = testhandle;
    
    mfl_append_tree_to_treebuffer(testree, searchrec->sr_treebuffer, testhandle);
    
    //mfl_subtree_pruning_and_regrafting(testree, searchrec);
    mfl_pruning_traversal(searchrec->sr_swap_entry, searchrec);
    dbg_printf("\nNumber of rearrangements attempted for %i taxa: %lli\n", testree->treet_num_taxa, searchrec->sr_rearrangement_counter);
    dbg_printf("\nAnd here are the trees:\n\n");
    char* newicktr = NULL;
    int i = 0;
    for (i = 0; i < searchrec->sr_treebuffer->tb_num_trees; ++i) {
        newicktr = mfl_convert_mfl_tree_t_to_newick(searchrec->sr_treebuffer->tb_savedtrees[i], 0);
        dbg_printf("TREE tree_%i = ", (i+1));
        dbg_printf("%s\n", newicktr);
        free(newicktr);
    }
    
    mfl_free_tree(testree);

}



bool mfl_heuristic_search(mfl_handle_s *mfl_handle)
{
    /* Eventually, this will parse the information in the handle and set up the 
     * heuristic search. For now, it just returns zero because we're not even 
     * close to that yet. */
    return 0;
}