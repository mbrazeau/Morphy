/*
 *  tree.c
 *  Morphy
 *
 *  Miscellaneous functions for trees
 *
 */

#include "morphy.h"

struct node * seekInternal(int ntax, node **nds)
{
    /* Searches for an unused internal node */
    /* NB: This function needs to return some kind of error msg
     * if no unused nodes are found. */
    
    int i;
    node *unused = NULL;
    node *p;
    bool isUsed = false;
    
    for (i = ntax + 1; nds[i]; ++i) {
        if (!nds[i]->next && !nds[i]->initialized && !nds[i]->outedge) {
            unused = nds[i];
            i = 2 * ntax;
        }
    }
    
    if (!unused) 
    {
        for (i = ntax + 1; nds[i]; ++i) 
        {
            isUsed = false;
            if (nds[i]->next) 
            {
                p = nds[i];                
                do
                {
                    if (p->outedge) 
                    {
                        isUsed = true;
                        p = nds[i];
                    }
                    else 
                    {
                        p = p->next;
                    }                
                } while (p != nds[i]);
                
                if (!isUsed) {
                    unused = nds[i];
                }
            }
        }
    }
    
    if (!unused) {
        printf("Error in tree memory allocation\n");
        return unused;
    }
    else {
        unused->initialized = 1;
        return unused;
    }
}

void closeRing(node *n)
{
    /* Makes sure there isn't a dangling next pointer*/
    node *p;
    
    p = n;
    
    do {
        if (p->next) {
            p = p->next;
        }
        if (!p->next) {
            p->next = n;
            p = p->next; 
        }
    }
    while (p != n);
}

void asRing(node *n)
{
    
    if (n->next) {
        closeRing(n);
        deletering(n);
    }
    
    newring(n);
}

void asNoring(node *n)
{
    if (n->next) {
        closeRing(n);
        deletering(n);
    }
}

void mfl_collapse(node *n, nodearray nds)
{
    /*collapses a branch*/
    int oldindex;
    node *an1, *an2, *p;
    
    oldindex = n->index;
    
    an1 = n->outedge;
    an2 = an1->next;
    p = an2;
    
    while (p->next != an1) 
    {
        p = p->next;
    }
    
    p->next = n->next;
    
    p = p->next;
    while (p->next != n) 
    {
        p = p->next;
    }
    
    an1->next = NULL;
    p->next = an2;
    n->next = NULL;
    n->outedge->outedge = NULL;
    free(n->outedge);
    n->outedge = NULL;
    mfl_set_index(an1);
    nds[oldindex] = n; // Ensures n can be found in the trnodes array
}

int mfl_determ_order(node *n)
{
    /*determines the number of branchings in a node*/
    int i = 0;
    node *p;
    
    if (n->outedge) {
        i = 1;
    }
    
    if (n->tip) {
        return i;
    }
    
    p = n->next;
    while (p != n) 
    {
        if (p->outedge) {
            ++i;
        }
        p = p->next;
    }
    
    return i;
    printf("node order: %i\n", i);
    
}

void mfl_set_order(node *n)
{
    int ord;
    node *p;
    ord = mfl_determ_order(n);
    
    n->order = ord;
    if (n->next) {
        p = n->next;
        while (p != n) {
            p->order = ord;
            p = p->next;
        }
    }
}

void mfl_clear_order(node *n)
{
    node *p;
    
    p = n->next;
    while (p != n) {
        p->order = 0;
        p = p->next;
    }
    
    n->order = 0;
}

void mfl_set_index(node *n)
{
    node *p;
    
    printf("clear the buffer\n");
    
    if (n->next) {
        p = n->next;
        while (p != n) 
        {
            p->index = n->index;
            p = p->next;
        }
    }   
}

void mfl_deinit_tree(tree *t)
{
    int i;
    node *p;
    
    for (i = 0; t->trnodes[i]; ++i) 
    {
        t->trnodes[i]->initialized = 0;
        
        if (t->trnodes[i]->next) 
        {
            p = t->trnodes[i]->next;
            
            do {
                p->initialized = 0;
                if (p->next) 
                {
                    p = p->next;
                }
                else 
                {
                    p = t->trnodes[i]; /* Should prevent a crash in the event of a dangling next pointer*/
                }

            } while (p != t->trnodes[i]);
            
        }
    }
}

void putBranchInRing(node *n, node *rnode)
{
    /* Given a branch (two nodes joined by their outedge pointers), places the 
     * branch in a ring */
    
    node *rnode2 = NULL;
    
    if (rnode->next) {
        rnode2 = rnode->next;
    }
    
    rnode->next = n;
    
    if (rnode2) {
        n->next = rnode2;
    }
    else {
        n->next = rnode;
    }
    
    rnode->order = rnode->order + 1;
    mfl_set_order(rnode);
}

void mfl_insert_branch(node *n, node *tgt1)
{
    node *p, *q, *tgt2;
    
    tgt2 = tgt1->outedge;
    
    if (n->outedge) 
    {
        p = n->next;
        while (p->outedge) 
        {
            p = p->next;
        }
    }
    
    q = p->next;
    if (q->outedge) 
    {
        while (q->outedge)
        {
            q = q->next;
        }
    }
    
    joinNodes(p, tgt1);
    joinNodes(q, tgt2);    
}

void mfl_arb_resolve(node *n, node **nds, int ntax, int numnodes)
{
    /* Arbitrarily resolves a non-binary node and leaves it binary*/
    
    int c, i, j, ord;
    node *p, *q, *in, *in2;
    
    // Make sure node there is a polytomy, otherwise exit resolve()
    mfl_set_order(n);
    ord = n->order;
    if (ord <= 3) {
        printf("mfl_arb_resolve() called in error on node %i\n", n->index);
        return;
    }
    
    // Find available internal node(s) in nds
    in = seekInternal(ntax, nds);
    in2 = in;
    if (in->next) {
        asNoring(in);
    }
    if (in->outedge) {
        if (in->outedge->outedge) {
            in->outedge->outedge = NULL;
        }
        in->outedge = NULL;
    }
    
    /* Randomly select branches to join to it. Cycles through the ring (skipping 
     * n itself) ntax % 10 times. This doesn't use a true random number 
     * generator, but should be both sufficiently arbitrary with respect to the
     * tree's topology, but sufficiently deterministic to be repeatable. */
    
    c = ntax % 10;
    
    for (i = 0; i < (ord - 3); ++i)
    {
        p = n->next;
        for (j = 0; j <= c; ++j) 
        {
            q = p;
            p = p->next;
            if (p = n) 
            {
                q = p;
                p = p->next;
            }
        }
        in2->next = p;
        in2 = in2->next;
        q->next = p->next;
        p->next = NULL;
    }
    
    p->next = allocnode();
    if (p->next->outedge) {
        p->next->outedge = NULL;
    }
    p = p->next;
    p->next = in;
    
    if (ntax % 2) 
    {
        n = n->next;
    }
    else 
    {
        n = n->next->next;
    }

    
    mfl_insert_branch(in, n);
    
}