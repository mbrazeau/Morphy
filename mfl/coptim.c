/*
 *  coptim.c
 *  Morphy
 *
 *  Created by Martin Brazeau on 2/15/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#include "morphy.h"

charstate * mfl_convert_tipdata(char *txtsrc, int ntax, int nchar, bool na_as_missing)
{
    
    /* One of Morphy's most important and unique features will be distinguishing
     * between a missing data entry "?" and a character-inapplicability entry "-".
     * Most existing programs just treat "?" and "-" as identical values (-1).
     * In Morphy, they can be treated differently, such that ancestral states
     * are not reconstructed for taxa which cannot logically have them */
    
    int i, j;
    
    charstate *tipdata = (charstate*) malloc(ntax * nchar * sizeof(charstate));
    
    if (!na_as_missing) {
        dbg_printf("Gap symbol ('-') treated as character inapplicability\n");
    }
    else {
        dbg_printf("Gap symbol ('-') treated as missing data\n");
    }

    
    for (i = 0, j = 0; txtsrc[i]; ++i, ++j) {
        if ((txtsrc[i] - '0') >= 0 && (txtsrc[i] - '0') <= 9) {
            tipdata[j] = 1 << (txtsrc[i] - '0' + 1);
            
        }
        else if (txtsrc[i] == '{' || txtsrc[i] == '(') {
            ++i;
            tipdata[j] = 0;
            while (txtsrc[i] != '}' && txtsrc[i] != ')') {
                if ((txtsrc[i] - '0') >= 0 && (txtsrc[i] - '0') <= 9) {
                    tipdata[j] = tipdata[j] | (1 << (txtsrc[i] - '0' + 1));
                    ++i;
                }
            }
        }
        else if (txtsrc[i] == '?') {
            tipdata[j] = IS_APPLIC;
        }
        else if (txtsrc[i] == '-') {
            if (na_as_missing) {
                tipdata[j] = IS_APPLIC;;
            }
            else {
                tipdata[j] = 1;
            }

        }
        else if (txtsrc[i] == '\n') {
            --j;
        }
        else {
            --j;
        }
    }
    
    return tipdata;
}

void mfl_apply_tipdata(tree *currenttree, charstate *tipdata, int ntax, int nchar)
{   
    int i, j;
    
    for (i = 0; i < ntax; ++i) {
        //currenttree->trnodes[i]->apomorphies = &tipdata[i * nchar];
        currenttree->trnodes[i]->tempapos = &tipdata[i * nchar];
        if (!currenttree->trnodes[i]->apomorphies) {
            currenttree->trnodes[i]->apomorphies = (charstate*)malloc(nchar * sizeof(charstate));
        }
        for (j = 0; j < nchar; ++j) {
            currenttree->trnodes[i]->apomorphies[j] = currenttree->trnodes[i]->tempapos[j];
        }
    }
}

int mfl_locreopt_cost(node *src, node *tgt1, node *tgt2, int nchar, int diff)
{
    /* Returns cost of inserting subtree src between tgt1 and tgt2 following
     * the algorithms described by Ronquist (1998. Cladistics) and Goloboff 
     * (1993, 1996. Cladistics).*/
    
    int i;
    int cost = 0;
    charstate *srctemps = src->apomorphies;
    charstate *tgt1apos = tgt1->apomorphies;
    charstate *tgt2apos = tgt2->apomorphies;
   
    for (i = 0; i < nchar; ++i) {
        if ( !(*(srctemps++) & (*(tgt1apos++) | *(tgt2apos++))) ) {
            ++cost;
            if (cost > diff) {
                return cost;
            }
        }
    }
    return cost;
}

int mfl_subtr_reinsertion(node *src, node *tgt1, node *tgt2, int nchar)
{
    /* Returns cost of reinserting the subtree src at the original place from
     * which it was clipped (tgt1 and tgt2). This score is used to compute the 
     * difference in length between the two subtrees so that neither the total 
     * tree length nor the length of the individual subtrees needs to be 
     * calculated (Ronquist 1998). */
    
    int i;
    int cost = 0;
    charstate *srctemps = src->apomorphies;
    charstate *tgt1apos = tgt1->apomorphies;
    charstate *tgt2apos = tgt2->apomorphies;
    
    for (i = 0; i < nchar; ++i) {
        if ( !(srctemps[i] & (tgt1apos[i] | tgt2apos[i])) ) {
            ++cost;
        }
    }
    return cost;
}

void mfl_subtree_count_ii(node *leftdesc, node *rightdesc, node *ancestor, int nchar)
{
    int i;
    charstate *lft_chars = leftdesc->apomorphies;
    charstate *rt_chars = rightdesc->apomorphies;
    charstate *anc_chars = ancestor->apomorphies;

    
    for (i = 0; i < nchar; ++i) {
        anc_chars[i] = lft_chars[i] | rt_chars[i];
    }
}

void mfl_reopt_subtr_root(node *n, int nchar)
{
    if (!n->tip) {
        mfl_subtree_count_ii(n->next->outedge, n->next->next->outedge, n, nchar);
    }
}

void mfl_subtree_count(node *leftdesc, node *rightdesc, node *ancestor, int nchar, int *trlength)
{
    int i;
    charstate lft_chars, rt_chars;
    
    for (i = 0; i < nchar; ++i) {
        if (leftdesc->tempapos[i] & rightdesc->tempapos[i]) 
        {
            ancestor->tempapos[i] = leftdesc->tempapos[i] & rightdesc->tempapos[i];
        }
        else
        {
            lft_chars = leftdesc->tempapos[i];
            rt_chars = rightdesc->tempapos[i];
            ancestor->tempapos[i] = lft_chars | rt_chars;

            if (lft_chars & IS_APPLIC && rt_chars & IS_APPLIC) {
                ancestor->tempapos[i] = ancestor->tempapos[i] & IS_APPLIC;
                if (trlength) {
                    *trlength = *trlength + 1;
                }
            }
        }
    }
}

int mfl_get_sttreelen(tree *testtree, charstate *tipdata, int ntax, int nchar, int *besttreelen)
{
    int treelen = 0;
    int *treelen_p = &treelen;
    
    mfl_apply_tipdata(testtree, tipdata, ntax, nchar);
    mfl_subtree_postorder(testtree->root, treelen_p, nchar);
    mfl_fitch_preorder(testtree->root, nchar);
    
    return *treelen_p;
}

int mfl_wagner_count(charstate lchar, charstate rchar)
{
    int length = 0;
    
    if (lchar > rchar) {
        while (!(lchar & rchar)) {
            lchar = lchar >> 1;
            ++length;
        }
    }
    else {
        while (!(lchar & rchar)) {
            rchar = rchar >> 1;
            ++length;
        }
    }
    return length;
}

void mfl_countsteps(node *leftdesc, node *rightdesc, node *ancestor, int nchar, int *trlength, int *besttreelen)
{
    int i;
    charstate lft_chars, rt_chars;
    
    for (i = 0; i < nchar; ++i) {
        if (leftdesc->tempapos[i] & rightdesc->tempapos[i]) 
        {
            ancestor->tempapos[i] = leftdesc->tempapos[i] & rightdesc->tempapos[i];
        }
        else
        {            
            lft_chars = leftdesc->tempapos[i];
            rt_chars = rightdesc->tempapos[i];
            
            ancestor->tempapos[i] = lft_chars | rt_chars;

            if (lft_chars & IS_APPLIC && rt_chars & IS_APPLIC) {
                ancestor->tempapos[i] = ancestor->tempapos[i] & IS_APPLIC;
                *trlength = *trlength + 1;
            }
        }
    }
}

void mfl_combine_up(node *n, node *anc, int nchar)
{
    int i;
    charstate lft_chars, rt_chars;
    
    for (i = 0; i < nchar; ++i) {
        
        if ((n->tempapos[i] & anc->apomorphies[i]) == anc->apomorphies[i]) 
        {
            n->apomorphies[i] = n->tempapos[i] & anc->apomorphies[i];            
        }
        else {
            
            lft_chars = n->next->outedge->tempapos[i];
            rt_chars = n->next->next->outedge->tempapos[i];
            
            if ( lft_chars & rt_chars ) { //III
                //V
                n->apomorphies[i] = (n->tempapos[i] | (anc->apomorphies[i] & (lft_chars | rt_chars))); //& IS_APPLIC;
            }
            else {
                //IV
                if ( (anc->apomorphies[i] & IS_APPLIC) && (n->tempapos[i] & IS_APPLIC)) {
                    n->apomorphies[i] = (n->tempapos[i] | anc->apomorphies[i]) & IS_APPLIC;
                }
                else {
                    n->apomorphies[i] = n->tempapos[i] | anc->apomorphies[i];
                }

            }
        }
    }
}

void mfl_subtree_postorder(node *n, int *trlength, int nchar)
{
    node *p;
    
    if (!n->apomorphies) {
        n->apomorphies = (charstate*)malloc(nchar * sizeof(charstate));
        memset(n->apomorphies, 0, nchar * sizeof(charstate));
        if (n->next) {
            mfl_join_apomorphies(n);
        }
    }
    
    if (n->tip) {
        return;
    }
    
    p = n->next;
    while (p != n) {
        mfl_subtree_postorder(p->outedge, trlength, nchar);
        p = p->next;
    }
    if (!n->tempapos) {
        n->tempapos = (charstate*)malloc(nchar * sizeof(charstate));
    }
    mfl_subtree_count(n->next->outedge, n->next->next->outedge, n, nchar, trlength);
}

void mfl_fitch_postorder(node *n, int *trlength, int nchar, int *besttreelen)
{
    node *p;
    
    if (!n->apomorphies) {
        n->apomorphies = (charstate*)malloc(nchar * sizeof(charstate));
        memset(n->apomorphies, 0, nchar * sizeof(charstate));
        if (n->next) {
            mfl_join_apomorphies(n);
        }
    }
    if (!n->tempapos) {
        n->tempapos = (charstate*)malloc(nchar * sizeof(charstate));
        memset(n->tempapos, 0, nchar * sizeof(charstate));
    }
    
    if (n->tip) {
        return;
    }
    
    p = n->next;
    while (p != n) {
        mfl_fitch_postorder(p->outedge, trlength, nchar, besttreelen);
        p = p->next;
    }

    mfl_countsteps(n->next->outedge, n->next->next->outedge, n, nchar, trlength, besttreelen);
    n->nodelen = *trlength;
}

void mfl_reopt_fitch(node *leftdesc, node *rightdesc, node *ancestor, int nchar, int *changing)
{
    int i, c;
    charstate lft_chars, rt_chars, temp;
    bool allsame = false;
    
    charstate *ldtemps = leftdesc->tempapos;
    charstate *rdtemps = rightdesc->tempapos;
    charstate *antemps = ancestor->tempapos;
    
    for (c = 0; changing[c]; ++c) {
        i = changing[c]-1;
        if (ldtemps[i] & rdtemps[i]) 
        {
            temp = ldtemps[i] & rdtemps[i];
            if (temp != antemps[i]) {
                antemps[i] = temp;
                allsame = false;
            }
        }
        else
        {
            lft_chars = ldtemps[i];
            rt_chars = rdtemps[i];

            temp = lft_chars | rt_chars;

            if (lft_chars & IS_APPLIC && rt_chars & IS_APPLIC) {
                temp = temp & IS_APPLIC;
            }
            
            if (temp != antemps[i]) {
                antemps[i] = temp;
                allsame = false;
            }
        }
    }
    if (allsame) {
        ancestor->success = true;
    }
}

void mfl_partial_downpass(node *n, tree *t, int numnodes, int ntax, int nchar, int *changing)
{
    
    node *p;
    p = n;
    
    mfl_erase_clippath(t, numnodes);
    mfl_definish_tree(t, numnodes);
    mfl_temproot(t, 0, ntax);
    
    
    while (p->outedge && !p->tip) {
        
        p = p->next;
        
        if (p->tocalcroot || !p->outedge) {
            
            p->clippath = true;
            
            mfl_reopt_fitch(p->next->outedge, p->next->next->outedge, p, nchar, changing);
            
            if (p->success && p->outedge) {
                
                /* POTENTIAL PROBLEM AREA: Never gets tested by datasets*/
                
                /*if (p->next->outedge->tocalcroot) {
                    mfl_reopt_preorder(p->next->outedge, nchar, changing);
                    break;
                }
                else if (p->next->next->outedge->tocalcroot) {
                    mfl_reopt_preorder(p->next->next->outedge, nchar, changing);
                    break;
                }*/
                mfl_reopt_preorder(p, nchar, changing);
                return;
            }
            if (!p->outedge) {
                mfl_reopt_preorder(p, nchar, changing);
                break;
            }
            p = p->outedge;
        }
    
    }
    
    mfl_undo_temproot(ntax, t);
    
}

bool mfl_reopt_postorder(node *n, int nchar, int *changing)
{
    
    node *p;
    bool fromclip = false;
    bool allsame = true;
     
    if (n->clip) {
        n->clippath = true;
        return true;
    }
    
    if (n->tip) {
        return false;
    }
    
    p = n->next;
    while (p != n) {
        if (mfl_reopt_postorder(p->outedge, nchar, changing)) {
            fromclip = true;
        }
        if (!p->outedge->success) {
            allsame = false;
        }
        p = p->next;
    }
    
    if (fromclip) {
        if (!allsame) {
            mfl_reopt_fitch(n->next->outedge, n->next->next->outedge, n, nchar, changing);
            n->changed = true;
        }
        n->clippath = true;
        return true;
    }
    else {
        return false;
    }

}

void mfl_fitch_allviews(node *n, int *trlength, int nchar, int *besttreelen)
{
    node *p;
    int weight = 0;
    
    if (!n->apomorphies) {
        n->apomorphies = (charstate*)malloc(nchar * sizeof(charstate));
        memset(n->apomorphies, 0, nchar * sizeof(charstate));
        if (n->next) {
            mfl_join_apomorphies(n);
        }
    }
    
    if (n->tip) {// || n->visited) {
        return;
    }
    
    p = n->next;
    while (p != n) {
        mfl_fitch_allviews(p->outedge, trlength, nchar, besttreelen);
        weight = weight + p->outedge->vweight;
        p = p->next;
    }
    
    if (!n->tempapos) {
        n->tempapos = (charstate*)malloc(nchar * sizeof(charstate));
        memset(n->tempapos, 0, nchar * sizeof(charstate));
    }

    mfl_countsteps(n->next->outedge, n->next->next->outedge, n, nchar, trlength, besttreelen);
    n->visited = 1;
    n->vweight = weight;
}

void mfl_tip_apomorphies(node *tip, node *anc, int nchar, int *changing)
{
    /* Reconstructs the tip set if it is polymorphic */
    
    int i, c;
    charstate *tiptemp = tip->tempapos;
    charstate *tipapos = tip->apomorphies;
    charstate *ancapos = anc->apomorphies;
    
    if (changing != NULL) {
        for (c = 0; changing[c]; ++c) {
            i = changing[c]-1;
            if (tiptemp[i] != 1) {
                if (tiptemp[i] & ancapos[i]) {
                    tipapos[i] = tiptemp[i] & ancapos[i];
                }
            }
        }
    }
    else {
        for (i = 0; i < nchar; ++i) {
            
            if (tiptemp[i] != 1) {
                if (tiptemp[i] & ancapos[i]) {
                    tipapos[i] = tiptemp[i] & ancapos[i];
                }
            }
        }
    }

}

void mfl_tip_reopt(tree *t, int ntax, int nchar)
{
    int i;
    for (i = 0; i < ntax; ++i) {
        mfl_tip_apomorphies(t->trnodes[i], t->trnodes[i]->outedge, nchar, NULL);
    }
}

void mfl_reopt_comb(node *n, node *anc, int nchar, int *changing)
{
    int i, c;
    charstate lft_chars, rt_chars;
    charstate temp = NULL;
    charstate *ntemps = n->tempapos;
    charstate *napos = n->apomorphies;
    charstate *ancapos = anc->apomorphies;
    
    bool allsame = true;
    
    for (c = 0; changing[c]; ++c) {
        
        i = changing[c]-1;
        
        if ((ntemps[i] & ancapos[i]) == ancapos[i]) 
        {
            temp = ntemps[i] & ancapos[i]; 
            if (temp != napos[i]) {
                napos[i] = temp;
                allsame = false;
            }
        }
        else {
            lft_chars = n->next->outedge->tempapos[i];
            rt_chars = n->next->next->outedge->tempapos[i];
            
            if ( lft_chars & rt_chars ) { //III
                //V
                temp = (ntemps[i] |(ancapos[i] &(lft_chars | rt_chars)));// & IS_APPLIC;
                if (temp != napos[i]) {
                    napos[i] = temp;
                    allsame = false;
                }
            }
            else {
                //IV
                if ( (ancapos[i] & IS_APPLIC) && (ntemps[i] & IS_APPLIC)) {
                    temp = (ntemps[i] | ancapos[i]) & IS_APPLIC;
                }
                else {
                    temp = ntemps[i] | ancapos[i];
                }
                
                if (temp != napos[i]) {
                    napos[i] = temp;
                    allsame = false;
                }
            }
        }
    }
    if (allsame) {
        n->finished = true;
    }
}


void mfl_set_rootstates(node *n, int nchar)
{
    int i;
    
    for (i = 0; i < nchar; ++i) {
        if (n->next->outedge->tempapos[i] & n->next->next->outedge->tempapos[i]) {
            n->apomorphies[i] = (n->next->outedge->tempapos[i] & n->next->next->outedge->tempapos[i]);
        }
        else {
            n->apomorphies[i] = (n->next->outedge->tempapos[i] | n->next->next->outedge->tempapos[i]);
            if (n->next->outedge->tempapos[i] & IS_APPLIC && n->next->next->outedge->tempapos[i] & IS_APPLIC) {
                n->apomorphies[i] = n->apomorphies[i] & IS_APPLIC;
            }
        }
    }
}

void mfl_reopt_rootstates(node *n, int nchar, int *changing)
{
    int i, c;
    charstate lft_chars, rt_chars, temp;
    bool allsame = false;
    
    charstate *ldtemps = n->next->outedge->tempapos;
    charstate *rdtemps = n->next->next->outedge->tempapos;
    charstate *antemps = n->apomorphies;
    
    for (c = 0; changing[c]; ++c) {
        i = changing[c]-1;
        if (ldtemps[i] & rdtemps[i]) 
        {
            temp = ldtemps[i] & rdtemps[i];
            if (temp != antemps[i]) {
                antemps[i] = temp;
                allsame = false;
            }
        }
        else
        {
            lft_chars = ldtemps[i];
            rt_chars = rdtemps[i];
            
            temp = lft_chars | rt_chars;
            
            if (lft_chars & IS_APPLIC && rt_chars & IS_APPLIC) {
                temp = temp & IS_APPLIC;
            }
            
            if (temp != antemps[i]) {
                antemps[i] = temp;
                allsame = false;
            }
        }
    }
    if (allsame) {
        n->finished = true;
    }
}

void mfl_reopt_preorder(node *n, int nchar, int *changing)
{
    
    node *dl, *dr;
    
    if (n->tip) {
        return;
    }
    
    if (n->finished && !n->clippath) {
        return;
    }
    
    if (!n->outedge || n->isroot) {
        mfl_reopt_rootstates(n, nchar, changing);
        if (n->finished) {
            return;
        }
    }
    
    dl = n->next->outedge;
    dr = n->next->next->outedge;
    
    if (!dl->tip) {
        mfl_reopt_comb(dl, n, nchar, changing);
    }
    else if (!n->finished) {
        mfl_tip_apomorphies(dl, n, nchar, changing);
    }
    
    if (!dr->tip) {
        mfl_reopt_comb(dr, n, nchar, changing);
    }
    else if (!n->finished) {
        mfl_tip_apomorphies(dr, n, nchar, changing);
    }
    
    mfl_reopt_preorder(n->next->outedge, nchar, changing);
    mfl_reopt_preorder(n->next->next->outedge, nchar, changing);
    
}

void mfl_reopt_comb_ii(node *n, node *anc, int nchar, int *changing)
{
    int i, c;
    charstate lft_chars, rt_chars;
    charstate temp = NULL;
    charstate *ntemps = n->tempapos;
    charstate *napos = n->apomorphies;
    charstate *ancapos = anc->apomorphies;
    
    bool allsame = true;
    
    for (c = 0; changing[c]; ++c) {
        
        i = changing[c] - 1;
        
        if ((ntemps[i] & ancapos[i]) == ancapos[i]) 
        {
            temp = ntemps[i] & ancapos[i]; 
            if (temp != napos[i]) {
                //dbg_printf("%i ", i);
                napos[i] = temp;
                allsame = false;
            }
        }
        else {
            lft_chars = n->next->outedge->tempapos[i];
            rt_chars = n->next->next->outedge->tempapos[i];
            
            if ( lft_chars & rt_chars ) { //III
                //V
                temp = (ntemps[i] |(ancapos[i] &(lft_chars | rt_chars)));// & IS_APPLIC;
                if (temp != napos[i]) {
                    //dbg_printf("%i ", i);
                    napos[i] = temp;
                    allsame = false;
                }
            }
            else {
                //IV
                if ( (ancapos[i] & IS_APPLIC) && (ntemps[i] & IS_APPLIC)) {
                    temp = (ntemps[i] | ancapos[i]) & IS_APPLIC;
                }
                else {
                    temp = ntemps[i] | ancapos[i];
                }
                
                if (temp != napos[i]) {
                    //dbg_printf("%i ", i);
                    napos[i] = temp;
                    allsame = false;
                }
            }
        }
    }
    //dbg_printf("\n");
    
    if (allsame) {
        n->success = true;
    }
}

void mfl_reopt_preorder_ii(node *n, int nchar, int *changing)
{
    
    node *dl, *dr;
    
    if (n->tip) {
        return;
    }
    
    if (!n->outedge || n->isroot) {
        mfl_set_rootstates(n, nchar);
    }
    
    if (n->success) {
        return;
    }
    
    dl = n->next->outedge;
    dr = n->next->next->outedge;
    
    if (!dl->tip) {
        mfl_reopt_comb_ii(dl, n, nchar, changing);
    }
    else if (!dl->success) {
        mfl_tip_apomorphies(dl, n, nchar, changing);
    }
    
    if (!dr->tip) {
        mfl_reopt_comb_ii(dr, n, nchar, changing);
    }
    else if (!dr->success) {
        mfl_tip_apomorphies(dr, n, nchar, changing);
    }
    
    mfl_reopt_preorder_ii(n->next->outedge, nchar, changing);
    mfl_reopt_preorder_ii(n->next->next->outedge, nchar, changing);
    
}

void mfl_fitch_preorder(node *n, int nchar)
{
    node *p, *dl, *dr;
    
    if (n->tip) {// || n->clip) {
        return;
    }
    
    if (!n->outedge || n->isroot) {
        mfl_set_rootstates(n, nchar);
    }
    
    dl = n->next->outedge;
    dr = n->next->next->outedge;
    
    if (!dl->tip) {
        mfl_combine_up(dl, n, nchar);
    }
    else {
        mfl_tip_apomorphies(dl, n, nchar, NULL);
    }
    
    if (!dr->tip) {
        mfl_combine_up(dr, n, nchar);
    }
    else {
        mfl_tip_apomorphies(dr, n, nchar, NULL);
    }

    p = n->next;
    while (p != n) {
        mfl_fitch_preorder(p->outedge, nchar);
        p = p->next;
    }
}

int mfl_get_subtreelen(node *n, int ntax, int nchar, int *besttreelen)
{
    int treelen = 0;
    int *treelen_p = &treelen;
    
    mfl_fitch_postorder(n, treelen_p, nchar, besttreelen);
    mfl_fitch_preorder(n, nchar);
    
    return *treelen_p;
}


int mfl_get_treelen(tree *t, int ntax, int nchar, int *besttreelen)
{
    int treelen = 0;
    int *treelen_p = &treelen;
    
    if (!t->root) {
        mfl_temproot(t, 0, ntax);
        mfl_fitch_postorder(t->root, treelen_p, nchar, besttreelen);
        mfl_fitch_preorder(t->root, nchar);
        mfl_undo_temproot(ntax, t);
    }
    else {
        mfl_fitch_postorder(t->root, treelen_p, nchar, besttreelen);
        mfl_fitch_preorder(t->root, nchar);
    }

    return *treelen_p;
}

charstate *mfl_get_subtr_changing(node *n, node *up, node *dn, int nchar)
{
    /* Compares preliminary and final states in the source tree in heuristic 
     * searches and lists characters that are expected to require reoptimization*/
    
    int i, j;
    
    charstate *prelims = n->tempapos;
    charstate *finals = n->apomorphies;
    int *changing = (int*)malloc((nchar + 1) * sizeof(int));
    
    j = 0;
    
    for (i = 0; i < nchar; ++i) {
        if (prelims[i] != finals[i]) {
            changing[j] = i + 1;
            ++j;
        }
    }
    
    changing[j] = '\0';
    
    return changing;
}

int *mfl_get_tgt_changing(node *n, node *crownprt, node *rootprt, int nchar)
{
    int i, j;
    
    int *changing = (int*)malloc((nchar + 1) * sizeof(int));
    charstate *crownprelims = crownprt->tempapos;
    charstate *rootfinals = rootprt->apomorphies;
    charstate *nprelims = n->tempapos;
    charstate *nfinals = n->apomorphies;
    
    j = 0;
    for (i = 0; i < nchar; ++i) {
        if (crownprelims[i] != nprelims[i] || rootfinals[i] != nfinals[i]) {
            changing[j] = i + 1;
            ++j;
        }
    }
    
    changing[j] = '\0';
    
    return changing;
}

mfl_changing *mfl_get_changing(node *base, node *subtr, node *crownprt, node *rootprt, int nchar)
{
    int i, j, k;
    
    mfl_changing *changing = (mfl_changing*)malloc(sizeof(mfl_changing));
    changing->srcchanging = (int*)malloc((nchar + 1) * sizeof(int));
    changing->tgtchanging = (int*)malloc((nchar + 1) * sizeof(int));
    
    charstate *prelims = base->tempapos;
    charstate *finals = base->apomorphies;
    
    charstate *crownprelims = crownprt->tempapos;
    charstate *rootfinals = rootprt->apomorphies;
    charstate *nprelims = subtr->tempapos;
    charstate *nfinals = subtr->apomorphies;
    
    j = 0;
    k = 0;
    for (i = 0; i < nchar; ++i) {
        if (prelims[i] != finals[i]) {
            *(changing->srcchanging + j) = i + 1;
            ++j;
        }
        if (crownprelims[i] != nprelims[i] || rootfinals[i] != nfinals[i]) {
            *(changing->tgtchanging + k) = i + 1;
            ++k;
        }
        
    }
    
    changing->srcchanging[j] = '\0';
    changing->tgtchanging[k] = '\0';
    
    return changing;
}

void mfl_wipe_states(node *n, int nchar)
{
    memset(n->apomorphies, 0, nchar * sizeof(charstate));
    memset(n->tempapos, 0, nchar * sizeof(charstate));
}

void mfl_set_calcroot(node *n)
{
    
    node *p;
    
    if (n->tip) {
        return;
    }
    
    p = n->next;
    
    while (p != n) {
        p->tocalcroot = false;
        mfl_set_calcroot(p->outedge);
        p = p->next;
    }
    
    n->tocalcroot = true;
    
}

void mfl_copy_originals(node *n, charstate *originals, int nchar)
{
    if (!n->origfinals) {
        n->origfinals = (charstate*)malloc(nchar * sizeof(charstate));
    }
    if (!n->origtemps) {
        n->origtemps = (charstate*)malloc(nchar * sizeof(charstate));
    }
    
    memcpy(n->origfinals, n->apomorphies, nchar * sizeof(charstate));
    memcpy(n->origtemps, n->tempapos, nchar * sizeof(charstate));
}

void mfl_restore_originals(node *n, charstate *originals, int nchar)
{
    memcpy(n->tempapos, n->origtemps, nchar * sizeof(charstate));
    memcpy(n->apomorphies, n->origfinals, nchar * sizeof(charstate));
}

void mfl_save_origstates(tree *t, int ntax, int numnodes, int nchar)
{
    int i;
    node *p;
    nodearray tns = t->trnodes;
    
    for (i = 0; i < numnodes; ++i) {
        
        p = tns[i];
        
        if (p->next) {
            while (!p->tocalcroot) {
                p = p->next;
                if (p == tns[i]) {
                    break;
                    dbg_printf("Error: node has no apomorphies array\n");
                }
            }
        }
        
        mfl_copy_originals(p, p->tempapos, nchar);
    }
}

void mfl_restore_origstates(tree *t, int ntax, int numnodes, int nchar)
{
    int i;
    node *p;
    nodearray tns = t->trnodes;
    
    for (i = 0; i < numnodes; ++i) {
        p = tns[i];
        
        if (p->next) {
            while (!p->tocalcroot) {
                p = p->next;
                if (p == tns[i]) {
                    break;
                    dbg_printf("Error: node has no apomorphies array\n");
                }
            }
        }
        mfl_restore_originals(p, p->origfinals, nchar);
    }
}

bool mfl_find_lastchanged(node *n, int nchar, int *changing)
{
    node *p;
    
    bool found = false;
    
    if (n->tip) {
        return false;
    }
    
    if (n->changed) {
        mfl_reopt_preorder(n, nchar, changing);
        return true;
    }
    
    p = n->next;
    while (p != n) {
        if (mfl_find_lastchanged(p->outedge, nchar, changing)) {
            found = true;
            break;
        }
        p = p->next;
    }
    
    return false;
}

void mfl_reopt_subtr(node *base, tree *t, int nchar, int numnodes, int *changing)
{
    
    /* For subtree reoptimization only. */
    
    base->isroot = true;
    mfl_desuccess_tree(t, numnodes);
    mfl_reopt_preorder_ii(base, nchar, changing);
    base->isroot = false;
    
}

void mfl_trav_allviews(node *n, tree *t, int ntax, int nchar, int *changing)
{
    
    /* For subtree reoptimization only. */
    
    mfl_definish_tree(t, 2 * ntax - 1);
    mfl_temproot(t, 0, ntax);
    mfl_erase_clippath(t, 2 * ntax - 1);
    mfl_reopt_postorder(t->root, nchar, changing);
    mfl_find_lastchanged(t->root, nchar, changing);
    mfl_undo_temproot(ntax, t);
    
}

int mfl_all_views(tree *t, int ntax, int nchar, int *besttreelen)
{
    int i=0;
    int treelen = 0, fptreelen;
    int *treelen_p = &treelen;
    
    mfl_devisit_tree(t->trnodes, 2 * ntax - 1);
    mfl_definish_tree(t, 2 * ntax - 1);

    *treelen_p = 0;
    mfl_temproot(t, 0, ntax);
    mfl_fitch_allviews(t->root, treelen_p, nchar, besttreelen);
    t->root->visited = 0;
    
    fptreelen = *treelen_p;
    mfl_set_calcroot(t->root);

    mfl_undo_temproot(ntax, t);
    mfl_temproot(t, 0, ntax);
    t->root->visited = 0;
    mfl_fitch_preorder(t->root, nchar);
    //mfl_union_construction(t->root, nchar);
    mfl_undo_temproot(ntax, t);
    
    return fptreelen;
}
