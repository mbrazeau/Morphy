/*
 *  rearrange.c
 *  Morphy
 *
 *  Functions used in tree rearrangements for heuristic searches.
 *
 */

#include "morphy.h"
#include <time.h>

mfl_searchrec * mfl_create_searchrec(void)
{
    mfl_searchrec *newsearchrec;
    
    newsearchrec = (mfl_searchrec*)malloc(sizeof(mfl_searchrec));
    
    newsearchrec->nextinbuffer = 0;
    newsearchrec->undertreelimit = true;
    newsearchrec->bestinrep = 0;
    newsearchrec->foundbettertr = false;
    newsearchrec->trbufstart = 0;
    newsearchrec->success = false;
    newsearchrec->niter_total = 0;
    newsearchrec->niter_ontree = 0;
    
    return newsearchrec;
}

void mfl_reset_searchrec(mfl_searchrec *searchrec)
{
    /*Partially resets the searchrec. Used when a new replicate is started. */
    
    searchrec->foundbettertr = false;
    searchrec->success = false;
    searchrec->niter_ontree = 0;
}

void mfl_destroy_searchrec(mfl_searchrec *searchrec)
{
    free(searchrec);
}

long long int mfl_rearr_num(bool reset)
{
    static long long int rn = 0;

    if (reset) {
        rn = 0;
    }
    
    return ++rn;
}


void mfl_bswap(node *p, node *q)
{
    /* Exchanges the position of two nodes */
    
    node *pbase, *qbase;
    
    pbase = p->outedge;
    qbase = q->outedge;
    
    p->outedge = qbase;
    q->outedge = pbase;
    pbase->outedge = q;
    qbase->outedge = p;
    
}

void mfl_remove_branch(node *n)
{
    node *p, *q, *nb;
    
    nb = n->outedge;
    p = nb->next->outedge;
    q = nb->next->next->outedge;
    nb->next->outedge = NULL;
    nb->next->next->outedge = NULL;
    mfl_join_nodes(p, q);
}

void mfl_insert_branch(node *br, node *target, int ntax)
{
    // Inserts a branch with a ring base into another branch
    
    node *br1, *br2, *tdesc;
    
    tdesc = target->outedge;
    
    if (br->tip) {
        br1 = br->outedge->next;
        br2 = br1->next;
    }
    else {
        br1 = mfl_seek_ringnode(br, ntax);
        br2 = mfl_seek_ringnode(br1->next, ntax);
    }

    if (br1->outedge || br2->outedge) {
        dbg_printf("Error in branch insertion\n");
        return;
    }
    
    mfl_join_nodes(br1, target);
    mfl_join_nodes(br2, tdesc);
}

/* Nearest-neighbor interchange (NNI) */

void mfl_nni_traversal(node *n, tree *swapingon, tree **savedtrees, int ntax, 
                       int nchar, int numnodes, long int *current, 
                       charstate *tipdata, bool *undertreelimit, 
                       int *currentbesttree, bool *foundbettertree)
{
    int trlength = 0;
    node *p;
    
    if (n->start) {
        mfl_nni_traversal(n->outedge, swapingon, savedtrees, ntax, nchar, numnodes, 
                          current, tipdata, undertreelimit, currentbesttree,
                          foundbettertree);
        return;
    }
    
    if (n->tip || !(*undertreelimit)) {
        return;
    }
    
    if (!n->outedge->tip)
    {
        /* Nearly identical steps are conducted twice because all nearest-
         * neighbor interchanges produce two distinct tree topologies */
        
        if (*current + 1 <= TREELIMIT) {
            mfl_bswap(n->next->outedge, n->outedge->next->outedge);
            mfl_root_tree(swapingon, 1, ntax);
            trlength = mfl_get_treelen(swapingon, ntax, nchar, currentbesttree);
            mfl_unroot(ntax, swapingon);
            if (trlength < *currentbesttree) {
                *foundbettertree = true;
                *currentbesttree = trlength;
                swapingon->length = trlength;
                mfl_reinit_treebuffer(savedtrees, swapingon, current, numnodes);
                trlength = 0;
                *current = *current + 1;
                return;
            }
            if (trlength == *currentbesttree) {
                *foundbettertree = false;
                savedtrees[*current] = mfl_copytree(swapingon, ntax, numnodes);
                savedtrees[*current]->index = *current;
                savedtrees[*current]->length = trlength;
                trlength = 0;
                *current = *current + 1;
            }
            mfl_bswap(n->next->outedge, n->outedge->next->outedge);
            
            if (*current + 1 <= TREELIMIT){ 
                mfl_bswap(n->next->next->outedge, n->outedge->next->outedge);
                mfl_root_tree(swapingon, 1, ntax);
                trlength = mfl_get_treelen(swapingon, ntax, nchar, currentbesttree);
                mfl_unroot(ntax, swapingon);
                if (trlength < *currentbesttree) {
                    *foundbettertree = true;
                    *currentbesttree = trlength;
                    swapingon->length = trlength;
                    mfl_reinit_treebuffer(savedtrees, swapingon, current, numnodes);
                    trlength = 0;
                    *current = *current + 1;
                    return;
                }
                if (trlength == *currentbesttree) {
                    *foundbettertree = false;
                    savedtrees[*current] = mfl_copytree(swapingon, ntax, numnodes);
                    savedtrees[*current]->index = *current;
                    savedtrees[*current]->length = trlength;
                    trlength = 0;
                    *current = *current + 1;
                }
                mfl_bswap(n->next->next->outedge, n->outedge->next->outedge);
            }
        }
        else {
            dbg_printf("Hit tree limit\n");
            *undertreelimit = false;
            return;
        }
    }
    
    p = n->next;
    while (p != n) {
        mfl_nni_traversal(p->outedge, swapingon, savedtrees, ntax, nchar, numnodes, 
                          current, tipdata, undertreelimit, currentbesttree,
                          foundbettertree);
        p = p->next;
    }
}


/* Quite a lot about this function will change. For now, it interfaces with the
 * test in main.c but that won't always be the case. */
void mfl_nni_search(int ntax, int nchar, int numnodes, charstate *tipdata, 
                    tree **savedtrees, int starttreelen)
{
    long int i = 0, j = 0; 
    int currentbest = starttreelen;
    long int trbufpos = 0;
    long int *nxtintrbuf = &trbufpos; 
    int *currentbest_p = &currentbest;
    long int numreps = 100;
    bool undertreelimit = true, *undertreelimit_p = &undertreelimit;
    bool foundbettertree = false, *foundbettertree_p = &foundbettertree;
    
    do {
        for (j = 0; j == 0 || j < *nxtintrbuf; ++j) {
            mfl_nni_traversal(savedtrees[j]->trnodes[0], savedtrees[j], savedtrees, 
                              ntax, nchar, numnodes, nxtintrbuf, tipdata, 
                              undertreelimit_p, currentbest_p, foundbettertree_p);
        }
        ++i;
    } while (i < numreps);
    
    dbg_printf("Next in trbuf: %li\n", *nxtintrbuf);
    
    dbg_printf("\nThe optimal tree(s) found by nearest neighbor interchange:\n");
    for (i = 0; i < *nxtintrbuf; ++i) {
        dbg_printf("Tree %li:\n", i + 1);
        mfl_root_tree(savedtrees[i], 0, ntax);
        /*printNewick(savedtrees[i]->root);*/
        dbg_printf(";\n");
        dbg_printf("Length: %i\n", savedtrees[i]->length);
    }
    dbg_printf("\n\n");
    mfl_clear_treebuffer(savedtrees, nxtintrbuf, numnodes);
    
    free(savedtrees);
}

long int mfl_spr_leftotry(int ntax)
{
    long int rleft = 0;
    rleft = 4 * (ntax - 3) * (ntax - 2);
    return rleft;
}

void print_visited(nodearray nds, int numnodes)
{
	int i;
	node *p;
	for (i = 0; i < numnodes; ++i) {
		if (nds[i]->visited) {
			dbg_printf("*");
		}
		else {
			dbg_printf(".");
		}
		if (nds[i]->next) {
			dbg_printf("/");
			p = nds[i]->next;
			while (p != nds[i]) {
				if (p->visited) {
					dbg_printf("*");
				}
				else {
					dbg_printf(".");
				}
				p = p->next;
			}
			dbg_printf("/");
		}
		
	}
	dbg_printf("\n\n");
}

void mfl_regrafting_traversal(node *n, node *subtr, tree *swapingon, 
                              tree **savedtrees, int ntax, int nchar, int numnodes, mfl_searchrec *searchrec, int diff)
{
    /* Called from within any subtree pruning algorithm used in either SPR or
     * TBR branch swapping. Traverses a binary tree in preorder, inserting the 
     * subtree at each node and (hopefully) skipping a reinsertion at the original
     * site of pruning (flagged by the "visited" boolean values in those nodes) */
    
    //dbg_printf("visiting site\n");
    
    if (searchrec->success) {
        return;
    }
    
    if (n->start) {
        mfl_regrafting_traversal(n->outedge, subtr, swapingon, savedtrees, ntax,
                                 nchar, numnodes, searchrec, diff);
        return;
    }
    
    static long int counter = 0;
    int trlength = 0;
    int al = 0;
    node *up;
    
    
    if (!(n->visited) && !(n->outedge->visited)) {
        up = n->outedge;
        al = mfl_locreopt_cost(subtr->next->outedge, n, up, nchar, diff);
        trlength = searchrec->bestinrep - diff + al;
        
        searchrec->niter_total = searchrec->niter_total + 1;
        if (trlength < searchrec->bestinrep) {
            counter = 0;
            mfl_insert_branch(subtr, up, ntax);
            searchrec->foundbettertr = true;
            searchrec->success = true;
            swapingon->length = trlength;
            searchrec->bestinrep = trlength;
            
            if (searchrec->bestinrep < searchrec->bestlength) {
                searchrec->bestlength = searchrec->bestinrep;
                mfl_reinit_treebuffer(savedtrees, swapingon, &searchrec->nextinbuffer, numnodes);
            }
            else {
                mfl_reinit_tbinrange(savedtrees, swapingon, searchrec->trbufstart, &searchrec->nextinbuffer, numnodes);
            }
            searchrec->nextinbuffer = searchrec->nextinbuffer + 1;
            return;
        }
        
        searchrec->foundbettertr = false;
        searchrec->success = false;
        if (trlength == searchrec->bestinrep) 
        {
            ++counter;
            mfl_insert_branch(subtr, up, ntax);
            if (!mfl_compare_alltrees(swapingon, savedtrees, ntax, numnodes, (long*)&searchrec->trbufstart, &searchrec->nextinbuffer)) 
            {
                savedtrees[searchrec->nextinbuffer] = mfl_copytree(swapingon, ntax, numnodes);
                savedtrees[searchrec->nextinbuffer]->index = searchrec->nextinbuffer;
                savedtrees[searchrec->nextinbuffer]->length = trlength;
                savedtrees[searchrec->nextinbuffer]->swapped = false;
                searchrec->nextinbuffer = searchrec->nextinbuffer + 1;
            }
            trlength = 0;
        }
        mfl_join_nodes(n, up);
    }
    
    if (n->tip && !n->start) {
        return;
    }

    mfl_regrafting_traversal(n->next->outedge, subtr, swapingon, savedtrees, ntax,
                                 nchar, numnodes, searchrec, diff);
    if (searchrec->foundbettertr) {
        return;
    }
    mfl_regrafting_traversal(n->next->next->outedge, subtr, swapingon, savedtrees, ntax,
                             nchar, numnodes, searchrec, diff);
    if (searchrec->foundbettertr) {
        return;
    }
}

void mfl_pruning_traversal(node *n, tree *swapingon, tree **savedtrees, int ntax, 
                           int nchar, int numnodes, mfl_searchrec *searchrec)
{

    /* Traverses a binary tree clipping out a subtree in postorder and passing 
     * a pointer to the subtree to mfl_regrafting_traversal. */
    
    node *p, *clipnode, *up, *dn, *subtr;
    int diff = 0;
    
    if (n->start) {
        mfl_pruning_traversal(n->outedge, swapingon, savedtrees, ntax,
                              nchar, numnodes, searchrec);
        return;
    }
    
    if (n->tip || searchrec->success) {
        return;
    }
    
    
    clipnode = n->next->next->outedge;
    
    p = n->next;
    while (p != n) {
        mfl_pruning_traversal(p->outedge, swapingon, savedtrees, ntax,
                              nchar, numnodes, searchrec);
        if (searchrec->success) {
            return;
        }
        up = p->next->outedge;
        dn = p->next->next->outedge;
        
        subtr = p->next->next;
        
        if (!subtr->next->outedge->skip) {
            
            subtr->next->outedge->skip = true;
            
            if (!(up->tip && dn->tip)) {
                up->visited = 1;
                dn->visited = 1;
                
                clipnode->clip = true;
                mfl_join_nodes(up, dn);

                // Reoptimize the clipped tree
                mfl_trav_allviews(swapingon->trnodes[0], swapingon, ntax, nchar, NULL, NULL);
                
                // Reoptimize the subtree
                mfl_reopt_subtr_root(subtr->next->outedge, nchar);
                
                diff = 0;
                
                // Determine the cost of local reinsertion
                diff = mfl_subtr_reinsertion(subtr->next->outedge, up, dn, nchar);
                mfl_regrafting_traversal(swapingon->trnodes[0]->outedge, subtr, swapingon, 
                                         savedtrees, ntax, nchar, numnodes, searchrec, diff);
                
                up->visited = 0;
                dn->visited = 0;
                clipnode->clip = false;
                if (searchrec->success) {
                    return;
                }
                subtr->next->outedge->skip = false;
                mfl_join_nodes(up, p->next);
                mfl_join_nodes(dn, p->next->next);
            }
        }
        else {
            /* Optimization: this is certainly a bit excessive and can be improved
             * by finding some way to call this less often--or not at all!*/
            mfl_trav_allviews(swapingon->trnodes[0], swapingon, ntax, nchar, NULL, NULL);   
        }
        p = p->next;
        clipnode = n->next->outedge;
    }   
}

node *mfl_find_atip(node *n)
{
    node *tip = NULL;
    
    if (n->tip) {
        tip = n;
        return tip;
    }
 
    tip = mfl_find_atip(n->next->outedge);
    
    return tip;
}

void mfl_reroot_subtree(node *n, node *subtr, node *base/*, node *up, node *dn, tree swapingon, tree *savedtrees, int ntax, int nchar, int numnodes, mfl_searchrec *searchrec*/)
{
    
    /* Traverses the subtree and re-roots it at each branch in preorder and then
     * calls regrafting traversal (same as used in SPR) */
    
    mfl_join_nodes(base->next->next, n->outedge);
    mfl_join_nodes(base->next, n);
    
    printNewick(base);
    dbg_printf(";\n");
    
    /*
     // Reoptimize the clipped tree
     mfl_trav_allviews(swapingon->trnodes[0], swapingon, ntax, nchar, NULL, NULL);
     
     // Reoptimize the subtree
     mfl_reopt_subtr_root(subtr->next->outedge, nchar);
     
     diff = 0;
     
     // Determine the cost of local reinsertion
     diff = mfl_subtr_reinsertion(subtr->next->outedge, up, dn, nchar);
     mfl_regrafting_traversal(swapingon->trnodes[0]->outedge, subtr, swapingon, 
     savedtrees, ntax, nchar, numnodes, searchrec, diff);*/
    
    // Remove the base
    mfl_join_nodes(base->next->outedge, base->next->next->outedge);
    
    if (n->tip) {
        return;
    }
    
    mfl_reroot_subtree(n->next->outedge, subtr, base/*, up, dn, swapingon, savedtrees, ntax, nchar, numnodes, searchrec*/);
    mfl_reroot_subtree(n->next->next->outedge, subtr, base/*, up, dn, swapingon, savedtrees, ntax, nchar, numnodes, searchrec*/);
    
    // Insert the base at n->outedge
    
}

void mfl_bisection_traversal(node *n, tree *swapingon, tree **savedtrees, int ntax, 
                           int nchar, int numnodes, mfl_searchrec *searchrec)
{
    
    /* Traverses a binary tree clipping out a subtree in postorder and passing 
     * a pointer to the subtree to mfl_reroot_subtree. */
    
    node *p, *clipnode, *up, *dn, *subtr;
    int diff = 0;
    
    if (n->start) {
        mfl_pruning_traversal(n->outedge, swapingon, savedtrees, ntax,
                              nchar, numnodes, searchrec);
        return;
    }
    
    if (n->tip || searchrec->success) {
        return;
    }
    
    
    clipnode = n->next->next->outedge;
    
    p = n->next;
    while (p != n) {
        mfl_pruning_traversal(p->outedge, swapingon, savedtrees, ntax,
                              nchar, numnodes, searchrec);
        if (searchrec->success) {
            return;
        }
        up = p->next->outedge;
        dn = p->next->next->outedge;
        
        subtr = p->next->next;
        
        if (!subtr->next->outedge->skip) {
            
            subtr->next->outedge->skip = true;
            
            if (!(up->tip && dn->tip)) {
                up->visited = 1;
                dn->visited = 1;
                
                clipnode->clip = true;
                mfl_join_nodes(up, dn);
                
                // Find a tip in the subtree to act as a starting point.
                // Call rerooting and reoptimization function on tip->outedge
                
                up->visited = 0;
                dn->visited = 0;
                clipnode->clip = false;
                if (searchrec->success) {
                    return;
                }
                subtr->next->outedge->skip = false;
                mfl_join_nodes(up, p->next);
                mfl_join_nodes(dn, p->next->next);
            }
        }
        else {
            /* Optimization: this is certainly a bit excessive and can be improved
             * by finding some way to call this less often--or not at all!*/
            mfl_trav_allviews(swapingon->trnodes[0], swapingon, ntax, nchar, NULL, NULL);
        }
        p = p->next;
        clipnode = n->next->outedge;
    }   
}


void (*mfl_swap_controller(mfl_handle_s *mfl_handle)) (node*, tree*, tree**, int, int , int, mfl_searchrec*)
{
    switch (mfl_handle->bswap_type) {
        case MFL_BST_TBR:
            dbg_printf("Not implemented\n");
            //return; // Temporary as would fail if tried
            break;
        case MFL_BST_SPR:
            return &mfl_pruning_traversal;
        case MFL_BST_NNI:
            dbg_printf("Not implemented\n");
            //return; // Temporary as would fail if tried
            break;
        default:
            dbg_printf("Not implemented\n");
            break;
    }
    return NULL;
}

bool mfl_heuristic_search(mfl_handle_s *mfl_handle)
{
    int ntax = mfl_handle->n_taxa, nchar = mfl_handle->n_chars;
    int numnodes = mfl_calc_numnodes(ntax);
    long int i = 0, j = 0;
    long int nreps = 0;
    double timein = 0;
    double timeout = 0;
    bool quit = false;
    
    void (*branch_swapper)(node*, tree*, tree**, int, int, int, mfl_searchrec*) = NULL;
    
    tree **savedtrees = (tree**) malloc(TREELIMIT * sizeof(tree*));
    
    tree *newreptree;
    
    
    charstate *tipdata = mfl_convert_tipdata(mfl_handle->input_data, mfl_handle->n_taxa, mfl_handle->n_chars, mfl_handle->gap_as_missing);
    
    mfl_searchrec *searchrec = mfl_create_searchrec();
        
    timein = (double)(clock() / (double)CLOCKS_PER_SEC);
    
    /* This outer loop makes it possible to do multiple replicates of random
     * addition sequence. The variable nreps is the number of times an initial
     * tree is generated using random addition sequence. */
    
    /*testing only*/
    nreps = 1;
    /* end testing only*/
    
    branch_swapper = mfl_swap_controller(mfl_handle);
    
    for (i = 0; i < nreps; ++i) {
        
        newreptree = mfl_addseq_randasis(ntax, nchar, numnodes, tipdata, mfl_handle->addseq_type, savedtrees);
        
        if (i == 0) {
            /* The particular addition sequence will have to be selected by the user */
            savedtrees[0] = newreptree;
            searchrec->bestinrep = mfl_all_views(savedtrees[0], ntax, nchar, &searchrec->bestinrep);
            searchrec->bestlength = searchrec->bestinrep;
            
            j = 0;
            
            /* TESTING ONLY */
            dbg_printf("The starting tree:\n");
            /*printNewick(savedtrees[0]->trnodes[0]);*/
            dbg_printf("\n");
            dbg_printf("The length of the starting tree: %i steps\n\n", searchrec->bestinrep);
            /* END TESTING ONLY */
        }
        else {
            
            savedtrees[searchrec->nextinbuffer] = newreptree;
            searchrec->bestinrep = mfl_all_views(savedtrees[searchrec->nextinbuffer], ntax, nchar, &searchrec->bestinrep);
            dbg_printf("Best length in replicate: %i\n", searchrec->bestinrep);
            j = searchrec->nextinbuffer;
            searchrec->trbufstart = searchrec->nextinbuffer;
            searchrec->foundbettertr = false;
            dbg_printf("j = %li\n", searchrec->nextinbuffer);
            quit = false;
            //break;
        }
        
        do {
            //if (i > 0) {
                //dbg_printf("swapping on tree: %li\n", j);
            //}
            mfl_reset_searchrec(searchrec);
            //searchrec->foundbettertr = false;
            //searchrec->success = false;
            mfl_apply_tipdata(savedtrees[j], tipdata, ntax, nchar);
            mfl_all_views(savedtrees[j], ntax, nchar, &searchrec->bestinrep);
            mfl_devisit_tree(savedtrees[j]->trnodes, numnodes);
            branch_swapper(savedtrees[j]->trnodes[0], savedtrees[j], savedtrees, 
                                  ntax, nchar, numnodes, searchrec);
            if (searchrec->foundbettertr) {
                if (i > 0) {
                    dbg_printf("trbuf start %i\n", searchrec->trbufstart);
                }
                j = searchrec->trbufstart;
            }
            else {
                savedtrees[j]->swapped = true;
                //mfl_devisit_tree(savedtrees[j]->trnodes, numnodes);
                ++j;
            }
            
            if (j >= searchrec->nextinbuffer || !searchrec->undertreelimit) {
                dbg_printf("number of rearrangements tried: %li\n", searchrec->niter_total);
                quit = true;
            }
            
            //dbg_printf("saved trees: %li\n", searchrec->nextinbuffer);
            
        } while (!quit);
        
        dbg_printf("Next in buffer at end of rep: %li\n", searchrec->nextinbuffer);
        
        //dbg_printf("best in rep: %i\n", searchrec->bestinrep);
        //dbg_printf("best overall: %i\n", searchrec->bestlength);
        
        if (i != 0) {
            if (searchrec->bestinrep > searchrec->bestlength) {
                mfl_reinit_tbinrange(savedtrees, savedtrees[0], searchrec->trbufstart, &searchrec->nextinbuffer, numnodes);
            }
        }
    }
    
    timeout = (double)(clock() / (double)CLOCKS_PER_SEC);
    
    mfl_handle->resultant_data = (mfl_resultant_data_s*) malloc(sizeof(mfl_resultant_data_s));
    
    mfl_handle->resultant_data->bestlength = searchrec->bestlength;
    mfl_handle->resultant_data->n_rearrangements = searchrec->niter_total;
    mfl_handle->resultant_data->n_savetrees = searchrec->nextinbuffer;
    mfl_handle->resultant_data->searcht = (timeout - timein);
    
    /* TESTING ONLY. This is just for checking output as I build up the heuristic
     * search procedure. Eventually, all this stuff will be written to a string
     * and handed over to the interface for outputting to screen. */
    
    dbg_printf("Total search time: %g\n", timeout - timein);
    
    dbg_printf("Number of saved trees: %li\n", searchrec->nextinbuffer);
    
    dbg_printf("\nThe optimal tree(s) found by subtree pruning and regrafting:\n");
    for (j = 0; j < searchrec->nextinbuffer; ++j) {
        dbg_printf("TREE str_%li = [&U] ", j+1);
        mfl_root_tree(savedtrees[j], 0, ntax);
        //printNewick(savedtrees[j]->root);
        dbg_printf(";\n");
    }
    dbg_printf("\n");
    
    mfl_clear_treebuffer(savedtrees, &searchrec->nextinbuffer, numnodes);
    free(savedtrees);
    
    /* END OF TESTING-ONLY SECTION */
    
    mfl_destroy_searchrec(searchrec);
    
    return true;
}
