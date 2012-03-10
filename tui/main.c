/*
 *  morphy.h
 *  Morphy
 *
 *  Created by Martin Brazeau on 11-04-26.
 *  Copyright 2011. All rights reserved.
 *  Morphy is provided as is with no warranty of any kind.
 *
 */


#include "morphy.h"

/*temporary defs for testing*/
#define MORPHY_NUM_ITERATIONS 15
//#define MAXSTATES 5
/**/

void call_index(node *n)
{
    node *p;
    
    if (n->next) {
        printf("index: %i\n", n->index);
        p = n->next;
        while (p != n) 
        {
            printf("index: %i\n", p->index);
            p = p->next;
        }
    }   
}

void dump_nodearray(nodearray nds, int ntax, int numnodes)
{
    int i;
    node *p; 
    
    for (i = 0; i < numnodes; ++i) {
        mfl_set_ring_to_n(nds[i]);
        printf("Index: %i, tip: %i, order: %i, address: %p, outedge %p", nds[i]->index, nds[i]->tip, nds[i]->order, nds[i], nds[i]->outedge);
        if (i >= ntax && nds[i]->next) {
            printf(", next: %p\n", nds[i]->next);
            p = nds[i]->next;
            while (p != nds[i]) {
                printf("In ring: index: %i, tip: %i, order: %i, address: %p, outedge: %p, next: %p\n", p->index, p->tip, p->order, p, p->outedge, p->next);
                p = p->next;
            }
        }
        else 
        {
            printf("\n");
        }

    }
}

void dump_connections(nodearray nds, int ntax, int numnodes)
{
    int i, nbi;
    node *p; 
    
    
    for (i = 0; i < numnodes; ++i) {
        //mfl_set_ring_to_n(nds[i]);
        if (nds[i]->outedge) {
            nbi = nds[i]->outedge->index;
        }
        else {
            nbi = 999;
        }

        printf("Index: %i, tip: %i, OE index: %i\n", nds[i]->index, nds[i]->tip, nbi);
        if (i >= ntax && nds[i]->next) {
            p = nds[i]->next;
            while (p != nds[i]) {
                if (p->outedge) {
                    nbi = p->outedge->index;
                }
                else {
                    nbi = 999;
                }
                printf("In ring: index: %i, tip: %i, OE index %i\n", p->index, p->tip, nbi);
                p = p->next;
            }
        }
    }
}


void dump_tree(tree *t, int ntax, int numnodes)
{
    printf("Tree %i:\n", t->index);
    printf("Root: %p\nLength: %i\n", t->root, t->length);
    dump_nodearray(t->trnodes, ntax, numnodes);
    printf("\n");
}

void dump_tree_connections(tree *t, int ntax, int numnodes)
{
    printf("Tree %i:\n", t->index);
    printf("Root: %p\nLength: %i\n", t->root, t->length);
    dump_connections(t->trnodes, ntax, numnodes);
    printf("\n");
}

void print_bipartition(taxbipart bipartition, int ntax)
{
    int i = 0;
    //printf("%i\n", bipartition);
    while (bipartition) {
        
        ++i;
        if (bipartition & 1) 
        {
            printf("*");
        }
        else 
        {
            printf(".");
        }
        bipartition = bipartition >> 1;
        if (bipartition == 0) {
            while (ntax - i) {
                ++i;
                printf(".");
            }
        }
    }
    printf("\n");
}

void print_hashtab(taxbipart **hashtab, int ntax)
{
    int i;
    int numbiparts = ntax - 1;
    taxbipart tempht;
    
    for (i = 0; i < numbiparts; ++i) {
        tempht = *hashtab[i];
        print_bipartition(tempht, ntax);
    }
    //printf("\n");
}

void print_f_states(node *n, int nchar)
{
    int i, j;
    charstate c;
    if (!n->tip) {
        printf("break\n");
    }
    
    for (i = 0; i < nchar; ++i) {
        printf("C%i: ", i + 1);
        //printf("as int % i, ", c);
        c = n->apomorphies[i];
        
        if (c == -1) {
            printf("?; ");
            continue;
        }
        else 
        {
            if (c & 1) {
                printf("-");
            }
            if ((c & IS_APPLIC)) {
                j = 0;
                c = c >> 1;
                while (c) 
                {
                    if (c & 1) {
                        printf("%i", j);
                    }
                    ++j;
                    c = c >> 1;
                }
            }
        }
        printf("; ");
    }
    printf("\n");
}

void print_final_allviews(tree *testtree, int ntax, int nchar, int numnodes)
{
    int i, j;
    
    for (i = ntax + 1; i < numnodes; ++i) {
        //print_f_states(testtree->trnodes[i], nchar);
        node *q = testtree->trnodes[i];
        printf("node %i\n", i);
        do {
            if (q->apomorphies) {
                for (j = 0; j < nchar; ++j) {
                    printf("%u ", q->apomorphies[j]);
                }
                printf("\n");
            }
            q = q->next;
        } while (q != testtree->trnodes[i]);
    }
}


void print_dp_states(node *n, int nchar)
{
    int i, j;
    charstate c;
    if (!n->tip) {
        printf("break\n");
    }
    
    for (i = nchar-1; i < nchar; ++i) {
        printf("C%i: ", i + 1);
        //printf("as int % i, ", c);
        c = n->tempapos[i];
        
        if (c == -1) {
            printf("?; ");
            continue;
        }
        else 
        {
            if (c & 1) {
                printf("-");
            }
            if ((c & IS_APPLIC)) {
                j = 0;
                c = c >> 1;
                while (c) 
                {
                    if (c & 1) {
                        printf("%i", j);
                    }
                    ++j;
                    c = c >> 1;
                }
            }
        }
        printf("; ");
    }
    printf("\n");
}


void print_nodedata(node *n, int nchar, int ntax)
{
    node *p;
    
    if (n->tip) {
        printf("Tip: %i\n", n->tip);
        print_f_states(n, nchar);
        printf("\n");
        return;
    }
    
    p = n->next;
    while (p != n) {
        print_nodedata(p->outedge, nchar, ntax);
        p = p->next;
    }
    
    printf("Node: %i\n", n->index);
    if (n->tipsabove) {
        print_bipartition(*n->tipsabove, ntax);
    }
    printf("nodelen %i\n", n->nodelen);
    print_dp_states(n, nchar);
    print_f_states(n, nchar);
    printf("\n");
}


void printNewick(node *n)
{   
    /* Prints the tree in Newick format (i.e. using brackets plus commas to 
     * separate equally ranked objects). In an unrooted tree, function will 
     * be called on a terminal node that has its start variable set to 1. */
    
    node *p;
    
    if (n->start) {
        printf("(%i,", n->tip);
        printNewick(n->outedge);
        printf(")");
        return;
    }   
    
    if (n->tip && n->outedge->next->outedge) {
        if (n->outedge->next->outedge->tip && 
            !n->outedge->next->outedge->start) {
            printf("%i", n->tip);
            return;
        }
    }
    
    if (n->tip && !n->start) {
        printf("%i", n->tip);
        return;
    }
    
    printf("(");
    
    p = n->next;
    while (p != n) {
        printNewick(p->outedge);
        p = p->next;
        if (p != n) {
            printf(",");
        }
    }   
    printf(")");
}

void pauseit(void)
{
    int c;
    
    do {
        printf("Enter c to continue: \n");
        c = getchar();
    } while (c != 'c');
    c = getchar();
}

void rand_tree (int ntax, int numnodes) 
{
    int i; //Loop counter
    
    /*generate a random tree*/
    
    tree **randtrees;
    randtrees = (tree **) malloc(MORPHY_NUM_ITERATIONS * sizeof(tree*));
    if (randtrees == NULL) {
        printf("Error in main(): failed malloc for randtrees\n");
        return;
    }
    
    for (i = 0; i < MORPHY_NUM_ITERATIONS; ++i) {
        randtrees[i] = randrooted(ntax, numnodes);
        printNewick(randtrees[i]->root);
        printf(";\n");
    }
    
    for (i = 0; i < MORPHY_NUM_ITERATIONS; ++i) {
        mfl_freetree(randtrees[i], numnodes);
    }
    
    free(randtrees);
}

node *mv_onetwo_to_twelve(tree *t, int ntax, int numnodes)
{
    int i;
    node *p;
    node *onetwonode = NULL;
    
    /*mfl_remove_branch(t->trnodes[0]);
    mfl_insert_branch(t->trnodes[0], t->trnodes[11], ntax);
    mfl_remove_branch(t->trnodes[1]);
    mfl_insert_branch(t->trnodes[1], t->trnodes[0], ntax);*/
    
    //Find the (1,2) node
    for (i = ntax + 1; i < numnodes; ++i) {
        p = t->trnodes[i];
        do {
            if ((p->outedge->tip == 11) || (p->outedge->tip == 12)) {
                onetwonode = t->trnodes[i];
                printf("found: %i\n", p->outedge->tip);
                break;
            }
            p = p->next;
            if (onetwonode) {
                break;
            }
        } while (p != t->trnodes[i]);
    }
    
    if (onetwonode->outedge->tip == 11 || onetwonode->outedge->tip == 12) {
        printf("Must find 'bottom' node\n");
    }

    return onetwonode;    
}

void test_char_optimization(void)
{
    char trstring[] = "(7,((8,(9,10)),(((((1,2),3),(4,(11,12))),5),6)));";
    tree *testtree = readNWK(trstring, 1);
    
    
    printf("The test tree:\n");
    printNewick(testtree->root);
    printf("\n");
    
    int ntax = 12;
    int nchar = 21;
    int trl = 0;
    int *trlength = &trl;
    int numnodes = mfl_calc_numnodes(ntax);
    charstate *tipdata;
    node *mvnode;
    
    mfl_trstring(testtree, ntax);
    
    //mfl_start_drawtree(testtree, ntax);
    
    char usrTipdata[] = "\
111111111100000000000\n\
111111111100000000002\n\
?01111111100000000002\n\
?00011111100000000000\n\
?00000111100000000001\n\
000000001100000000002\n\
100000000000000000110\n\
100000000000000011110\n\
100000000000001111111\n\
000000000000111111111\n\
-00000000011111111112\n\
-00000000011111111110";
    
    //old c21 pattern: 122012001120
    
    tipdata = mfl_convert_tipdata(usrTipdata, ntax, nchar, false);
    mfl_apply_tipdata(testtree, tipdata, ntax, nchar);
    
    mvnode = mv_onetwo_to_twelve(testtree, ntax, numnodes);
    mfl_undo_temproot(ntax, testtree);
    trl = mfl_all_views(testtree, ntax, nchar, trlength);
    //mfl_root_tree(testtree, 0, ntax);
    mfl_temproot(testtree, 5, ntax);
    printf("The altered tree (if move made):\n");
    printNewick(testtree->root);
    printf("\n");
    //mfl_undo_temproot(ntax, testtree);
    //testtree->bipartitions = mfl_tree_biparts(testtree, ntax, numnodes);
    printf("tree length: %i\n", trl);
    //mfl_undo_temproot(ntax, testtree);
    
    print_final_allviews(testtree, ntax, nchar, numnodes);
    
    int pos = 3;
    int diff = 0;
    node *up, *dn;
    up = mvnode->outedge->next->outedge;
    dn = mvnode->outedge->next->next->outedge;
    
    printf("mvnode index: %i\n", mvnode->index);
    printf("mvnode base:  %i\n", mvnode->outedge->index);
    
    mfl_join_nodes(up, dn);
    printf("The pruned tree:\n");
    printNewick(testtree->root);
    printf("\n");
    up->clip = true;
    dn->clip = true;

    mfl_undo_temproot(ntax, testtree);
    //mfl_wipe_states(testtree->trnodes[ntax], nchar);
    mfl_trav_allviews(testtree->trnodes[0], testtree, ntax, nchar, NULL, NULL);
    
    diff = mfl_get_treelen(testtree, ntax, nchar, NULL);
    printf("length of three after clipping: %i\n", diff);
    
    diff = mfl_get_subtreelen(mvnode, ntax, nchar, NULL);
    printf("length of sourcetree: %i\n", diff);
    
    diff = mfl_subtr_reinsertion(mvnode, testtree->trnodes[pos], testtree->trnodes[pos]->outedge, nchar);
    printf("cost of rejoining to original place: %i\n", diff);
    
    diff = mfl_locreopt_cost(mvnode, testtree->trnodes[9], testtree->trnodes[9]->outedge, nchar, 10000);
    printf("cost of rejoining to a new place: %i\n", diff);
    
    printf("target index: %i\n", testtree->trnodes[pos]->outedge->index);
    printf("target outedge: %i\n", testtree->trnodes[pos]->index);
    
    print_final_allviews(testtree, ntax, nchar, numnodes);
    
    mfl_join_nodes(mvnode->outedge->next, testtree->trnodes[9]->outedge);
    mfl_join_nodes(mvnode->outedge->next->next, testtree->trnodes[9]);
    //mfl_undo_temproot(ntax, testtree);
    diff = mfl_all_views(testtree, ntax, nchar, NULL);
    printf("length of the tree after placement: %i\n", diff);
    
    print_final_allviews(testtree, ntax, nchar, numnodes);
    
    mfl_temproot(testtree, 6, ntax);
    printf("The altered tree (if move made):\n");
    printNewick(testtree->root);
    printf("\n");
    
    //mfl_freetree(testtree, ntax);
}

void mini_test_analysis(void)
{    
    /* The content of this function will model a simple analysis. Obviously,
     * of the content of this function will be moved to other functions or 
     * will be stored differently. For now, it's just one place for stuff that 
     * needs to be available to the search algorithms before they can find the 
     * shortest tree. The analysis should converge towards a tree like the one
     * in the Newick string below. */
    
    int i;
    int ntax = 47, nchar = 134; // These values would be supplied by the datafile
    int besttreelen = 0;
    int numnodes;
    
    numnodes = mfl_calc_numnodes(ntax);
    
    /* A completely balanced tree topology. 
     * This is simple enough to create a 'rigged' dataset for.*/
    //char aNewickTree[] = "((((((1,2),3),4),5),6),(7,(8,(9,(10,(11,12))))));";
    
    /* A search with this dataset rigged to favour the topology of aNewickTree */

/*                                                                                                 1         1         1         1
         1         2         3         4         5         6         7         8         9         0         1         2         3
1........0.........0.........0.........0.........0.........0.........0.........0.........0.........0.........0.........0.........0....*/
    char usrTipdata[] = "\
0000-0-???01001?1?000??--00-000------0-0-----?---------000?100?0-0000000-00?0-----010-000--00-0000--------?--0----0-0??---??0-?----0??\
000100010??0110?0?10000--00-000----0-0-0-----?---------000000000-0000000-00?0-----010-000--00-?0100000-0?1?--0-1001?00-000?0?-00---00?\
000120011001001100111??-0?10001121?0-0-?0-----001110010?011?1??01?0111??0111101?011110?011-11011111---0-1{12}0101001110100001111100100011\
10012000?000?0-10?1?1??-0?0-000----0-0-?11110-0011000001001?10?000111010110?111?111?101111-01010001---0-11-001011110111001100100000000\
0100-00?--?--0?00?0-0000100-0-0----100-0?----?0100-0??0?0?0?0??????0?????0??????????????0?????????000010?0???1--001-0--110000-00---010\
000??0???????11???100??-0?1011100000-0-??-?---00??????????1?1?????????????????????????????-???????01-10-1201010??01?10?00?111011110001\
0001?0??00010???0?0-?000?10-1-?????10??????????????????0?001000000000000-0000100?0100-000-100-00?000????11-00101?01?????01??1??0??????\
0101000100?1101100100??-0?11000----0-0-011100-00??????????1?1?????????????????????????????-???????1---0-?2????-??-1?10000?111111111001\
00?1?0?10001????0?0-0011110-0-0----110-010--0?0100-0??001010100001010010-0000000?01???0012100-0010000011?10?-1?1??1?????0?0?1??000????\
000??001?010100?1?0-11-0010-1-?----110-112---?0100-0??0?0????????????0????0??????0??????0-1???????000110?1??-1-??01?1110011010000-01?0\
0?0??0???0010?1???10???-?110??111?10-0-?0-----?0??????????1?1?????????????????????????????-???????1---0-121???????1?10?00?111010100001\
00012001100100110?111??-0?1000111110-0-?0-----001110000?0?1?1???????????????????01????????-???????1---0-1211010??11?10000?111100100011\
0001201101010110110-01-110101-1100110111110-001111-?11??101?10?????11????1????0?0??????????1??????00110-?0???1????1010000?000-00---011\
110?????????????0??????-????????????????1111???0110?0001?110???000011010100011111110111111?0110000????????????????????????????????????\
11012001000??021??111??-0?11000----0-0-?11110-001100000?0?1?10?000111????1???11?11??1??111-01??0101---0-11-00101101?10000?100000?00000\
0001000??00010110?10000-0?1011100000-0-011100-0011??000?0???1?????????????????????????????-???????01-10-1200010??01?10?00?111011110001\
000??00??????0??0?0-0011110?0-0----110-0110-0101????????1?1?1????????????0????????????0???????????000011?10?-1????1?10?00?001-0001-111\
??0??001?00100110?100??-1?10000----0-0-0???---0?????????????1?????????????????????????????-???????01-10-120?010??01?1?000?111100100001\
010??0??0110111???0-01-101??1????????100110-001???????????1?1?????????????????????????????????????00110-??????????1?10000??00-00---?01\
00?110?1?0??????0?0-0011110-0-0----100-010--0-0100-0???01010100001010010-0001000?0100-0012100-00?0000010010?-1-1101????0010?1-?001-1??\
01?1?0011001001??0110??-0?12100----??0-?0---0-01????????????1?????????????????????????????-???????00110-11101000?01?100001111110100001\
1?0??0010???????0?10???-0?????0----0-0-?11110-001?????0101??10?0001010???0??111?11?011?111-01000?0???-????????????1????00???1???11????\
00010001?0011011?0100??-0?1000111110-0-?11100-00????????0???1?????????????????????????????-???????0-110-1211010???1?10000?111011100001\
0001200110010011??100??0011210100010-0-?0-----00??????1?????1?????????????????????????????-???????1---0-11001001?01?10000?111111110?01\
1101200??00010210?110??-0??-000----0-0-?11100-00?1??0?01011?11?00?0110??-???0???1??011?011-00??0?01---0-11-001011010101000100100101001\
000??00??001001100111??-0?10001121?0-0-?0-----001110010?011?1?????011????11?????01??????11-1?????11---0-1211010??11?10000?111100100011\
?????01?01101110110-01-110101-11001101111110101111-?1???1?1?10?010?111???11?????0??????010011????100110-?1-??1??101110000?000-00---?11\
000??0???0010011?0100??-0?1?00110110-0-?1110110111?1000?0???1???????????????????????????1?-???????1---0-1211011??11?10000?111100100001\
010??001????1??????????-??????0??-?0-????????-??1?????????????????????????????????????????-???????1---0-?0????-???1?10?00?11111110?001\
001??0????????????0-?1-1???????????????1???????????????0?01010101?01101?01??1?000?1?100011?11?????????????????????????????????????????\
00?110??????????1?0-?000?10-1-??????0??1???????????????00?1100?000000000-000?1-1?0100-000-100-?00000??????????????1?????????1???01????\
000??00???0?0?11??101??-0110?0111110-0-?0-----0011????0?0?1?1???????????????????0?????????-???????1---0-111101?0??1?10000?111010100011\
0011201101101110110-01-110101-1100110111110-001111-1100010111010100111110111111?00111000100110111100110-01---1-01011100001000-00---011\
0101200000?01021??110??-0?0-00?0?????0-?1?----00????????????1?????????????????????????????????????1---0-?0????-??-1?10?00?111101110001\
0011201100100?10?00-01-111101-1100?10101111010111101????101110111001001-?1111???011????01?011??1??00110-01---1-0001-100001000-00---001\
1001200??????0????1????-0?????0----0-0-?11110-00110100010010?1?000011010100011111110111111-01100001---0-11-00101?11?10100?100?00??10-1\
0?0??00100???01???100??-0?1011100000-0-011100-00????????0???1?????????????????????????????-???????01-10-1201010??01?10000?111111111001\
000120?100010?1???10???-0??????????0????11101101?1???00???????????????????????????????????-???????1---0-1211011???1?10?00?111?00100001\
0011211101101010000-01-110101-110?1101011110101111010000101110?110011011?111101?0111???010011??10100110-01---1-0001-100001000-00---?01\
0101?0011011001???101??-011000111010-0-00-----00110???0???1?1????????????????????1????????-???????1---0-12110000?11?10?00?111011100011\
001??1?????0??????0-01-?10???-?????????1111?101???????00101?10?110010011?1??1???0?11100010?11?????00?????11??1???01?????0?1?10?0100???\
010??001?0???100??0-00??100?0-0?---100-00---??0?????????0?0?0??????0????????????????????0?????????0?0010?0???1--00?-0--11??00?00---010\
?00??00??00110110?100??-0110001?000?????11100-0011?0000?0?1?10??0???0????0???0000???????????????10011-0-11000101?01?10000?111011110001\
110?????????????0??????-??????????????????????????0??????11????00001101?110?11010110101111?11?0?10??????1?????????????????????????????\
010??0???????00?1?0-11-0010-1-0----110-112---?010?-?????0????????????????????????0??????0-1???????00011???0?-1-???1?11100?101000110?00\
110??0?1-0001???0?11???-????????????????1111?0?0110??001?11??1?000011010100011111110111111-0100000????????????????????????1??1??11????\
0001200110010011??110??-0?12000----0-0-0110-0?0101-0001???1?1????????????????????0????????-???????1---0-11000100?11?10000?111000110001";

    /*"00000000000000000000000---0000000-0000000000000000000-0-0000000000000000000--00000000000000000?\n\
11-10121001120200001001---0000100-1--10001000001101010320010000010100001000--30300000000000010?\n\
??????????????????????30--0010??0-????????1-?-??100?0-??1?--????0?00??10??1112??????100111-0-1?\n\
00??0?????????????????3111100?????????????011100??0???????11?1????????10?????2??????1?1-1??????\n\
000?0???????11??01111030--???1010-???0????1---0-10010-3?00--??010?000??0??111?1-?0?111011????1?\n\
11-101201011212?0001002---0001100-01010011000001101010320010000010101001000--10310000000000000?\n\
00?100(02)???01?1(23)0???10?0---00?00010002?001100000001000-31-010????-?30?000000--01-0???00?0000100?\n\
1011023111112131???1100---000000101--200010000101000103011?01-1-01001101010?-11-10??00?0?00010?\n\
?????????????????0????3000?10?010-0100?01100-0--100011300?10????0????0010110021-1???1001101000?\n\
10110231101111311001110---0000000-1---000100001010000-3011101-1-01311001010--11-001000000000100\n\
????????????????0?????3000?1?0??0-010?1?1100100?100?11??0?00????0??0?000111002??????10011010001\n\
????????????????0001??3101?100????0??????1001100100011??0?010100??????00??1002????1?10??????001\n\
????????????2????11?????--????????????????1-?-??1?????????--?????0?00?10?????2?????110011????10\n\
11-11?301?1111(23)?0010002---000000110121011100000111001011-010?0001?011000000--?021?0000000001001\n\
11-11?30012120000000001---000000110021011100000101000-11-000000010010001000--101110?00000000000\n\
0011012?1011?12??011002---00001010002?001100000?101010??1??0??001?10?00100??-???1??0???????0000\n\
1001??30??112???001001?---0000000-1--2011100000111000-2?-010000010211001000--0021?0000000000000\n\
?????????????????11???30--0010010-???????11-?-??10010-3010--0001?0000010?????2??????10011??0-10\n\
0???0?1??1?0?1??00????311111??010-0100?011001100100011??0?01??????0??00011???21-101?1001(01)010001\n\
00000?101110113000011030000100010-0100101100100010001?30001000000000010{01}1110021-101110?101-0001\n\
00100000101011101000110---0-000010011000010000101000103011100?0?00?0?101010--11-0?1?00000000000\n\
00000?1?1??0113?00011031111101010-01?0?0??011100100?0-3000010100?0?0??10?110(01)21-?011111-1010?11\n\
?00?????1?1?213?01?1????--0011????????????1-?-??10010-??0?--??0??0000??0??????????????????????0\n\
11-11?3???1?2???000000?---000000110111011100000111001011-010000010011000000--00211??00000001000\n\
00?00000?01021100000002---000010111--1001000000110101032001000001000?001000--10010??00?00000000\n\
??????????????????????30--????????????????1-?????0????????????0????0??????????????????????????0\n\
?????????????????1?1??3000?0?1????010????1001???100011??0??000??0??0?000??11021-1??1100111-0001\n\
?????????????????????????????????????????100?????????????0?000??????0?00?????21-???1100111---1?";
    
    /*real data*/ /*"0000000000?00000100000000000000000000000000000100000100010000000000000101000101000000000010201100-0?0100--0--0000000010000000001320-?0????\
2000000000?00000-00000000000000000000000000000000000100010000000000000000000101000010000000001100-0?0100--0--00000000000001000002002?01???\
??2?????11?00000?????????????????0?10?000???????211131?1????11?1????????????????????0???????????0-0?????0????1211????10????10????2????????\
0000000000000000000000000000000000000000000000000000000000000000000000001000000000000000000101000-000000--0--00000000000000000000000??0???\
0000000000000000000000000000000000000000000000000000100010000000000000000000000000000000000000000-000100--0--00000000000000000010001001000\
1010030011200000100000001001000000000000000000101000201020210001110010000000000000000000000000000-1001----5--12110001121102000111002112111\
1010030011200000100000001001000000000000000000101000201020210001110010000000000000000000000000000-1001----5--12110001121102000011002112111\
1110040111200000110001002000000110001000010000100010100111210001210120100100001000100000000001010-010111--0--11110000201000000013012112111\
212111111012101122211211211211122111111201211111211111211111111122112110011111112021111111101011100112321110012111112201001121102112113000\
212111111012101122211211211211122111111201211111211111211111111122112110011111112021111111101011100112321110012111112201001121102112113000\
212111111122101122211211211211122101110111211211211111111111011122112010011111112020111101101001100112220010112111112201002121001102113000\
212112101012111122111211311311122111110222111111211131011111111122112021111101111130111111101111110112323111012111112310111110202102113000\
212112111012111122111211311311122111111222111211211111111111111122112021011101111030111111101001100112321021112111112300111100102212113000\
212112101011111122111211311311122111111222111211211131211111111122112021011101111130111121121111110112322132112111122300111111102122113000\
212112101012111122211211311311122111111222111211211111111111111122112021011101111030111111101111100112322031012101112310111100102212113000\
212112101011111122211211311311122111111222011211211131011111111122112021011101111130111121121111110112322243112111112310111120202202113000\
?0100?0?0??0?00??0??00???00?????0??0000?0100??0?11000000????????????????????????????00???000??????????????0??1011?000100000001003?????????";*/
       
    /*rigged data*/
    /*"11111111110000000000\
11111111110000000000\
00111111110000000000\
00001111110000000000\
00000011110000000000\
00000000110000000000\
00000000000000000011\
00000000000000001111\
00000000000000111111\
00000000000011111111\
00000000001111111111\
00000000001111111111";*/
        
        /* a noisier dataset */
        /*"11111111110000000011
00?1001???0000011011
00111111110000000000
00001111110000000000
00000011110000000000
00000000110000000000
00100000000000000000
00001100000000001110
00000001100000111110
001000001000111?1101
00000101001111111101
00000000001111111111";*/
        
        
    
    printf("User tip data:\n");
    for (i = 0; usrTipdata[i]; ++i) {
        printf("%c", usrTipdata[i]);
        /*for (j = 0; j < nchar; ++j) {
            printf("%c", usrTipdata[j + i * nchar]);
        }
        printf("\n");*/
    }
    printf("\n");
    
    //tree **savedtrees = (tree**) malloc(TREELIMIT * sizeof(tree*));
    
    mfl_handle_s *mfl_handle = mfl_t2s(mfl_create_handle());
    
    mfl_handle->n_taxa = ntax;
    mfl_handle->n_chars = nchar;
    mfl_handle->input_data = usrTipdata;
    mfl_handle->gap_as_missing = true;
    mfl_handle->addseq_type = MFL_AST_ASIS;
    mfl_handle->bswap_type = MFL_BST_SPR;
    
    mfl_heuristic_search(mfl_handle);
    
    //mfl_clear_treebuffer(savedtrees, , numnodes);   
}

int main(void)
{
    int ntax = 9;
    int numnodes;
    //bool isRooted = true;
    
    pauseit();
    
    //srandom(8);
    
    //test_tree_comparison();
    test_char_optimization();
    
    numnodes = mfl_calc_numnodes(ntax);
    
    //tree *anewtree;    
    //tree *originaltree;
    //tree *copiedtree;
    
    mini_test_analysis();
    
    //test_spr();
    
    /* This part is just for testing the collapseBiNode*/
    /*anewtree = randrooted(ntax, numnodes);
    printf("New tree: ");
    printNewick(anewtree->root);
    printf("\n");
    //dump_connections(anewtree->trnodes, ntax, numnodes);
    //dump_nodearray(anewtree->trnodes, ntax, numnodes);
    
    copiedtree = mfl_copytree(anewtree, ntax, numnodes);
    printf("A copied rooted tree: ");
    printNewick(copiedtree->root);
    printf("\n");
    
    mfl_bswap(anewtree->trnodes[1], anewtree->trnodes[2]);
    printf("Original tree w swapped br: ");
    printNewick(anewtree->root);
    printf("\n");
    
    copiedtree = mfl_copytree(anewtree, ntax, numnodes);
    printf("Copy with swapped branch: ");
    printNewick(copiedtree->root);
    printf("\n");
    
    mfl_freetree(copiedtree, numnodes);
    
    mfl_collapse(anewtree->trnodes[ntax + 2], anewtree->trnodes); // Magic number just for testing
    printf("With collapsed node: ");
    printNewick(anewtree->root);
    printf("\n");    
    //dump_connections(anewtree->trnodes, ntax, numnodes);
    
    copiedtree = mfl_copytree(anewtree, ntax, numnodes);
    //dump_nodearray(copiedtree->trnodes, ntax, numnodes);
    printf("Copying with collapsed node: ");
    printNewick(copiedtree->root);
    printf("\n");
    
    mfl_freetree(copiedtree, numnodes);
    
    mfl_arb_resolve(anewtree->trnodes[ntax + 1], anewtree->trnodes, ntax, numnodes); // Magic number just for testing
    //dump_connections(anewtree->trnodes, ntax, numnodes);
    printf("With resolved node: ");
    printNewick(anewtree->root);
    printf("\n");
    
    copiedtree = mfl_copytree(anewtree, ntax, numnodes);
    //dump_nodearray(copiedtree->trnodes, ntax, numnodes);
    printf("Copying with resolved node: ");
    printNewick(copiedtree->root);
    printf("\n");
    
    mfl_freetree(anewtree, numnodes);
    mfl_freetree(copiedtree, numnodes);
    
    originaltree = randunrooted(ntax, numnodes);
    printf("unrooted test: ");
    printNewick(originaltree->trnodes[0]);
    printf("\n");
    
    copiedtree = mfl_copytree(originaltree, ntax, numnodes);
    printf("Copying of unrooted tree: ");
    printNewick(copiedtree->trnodes[0]);
    printf("\n");
    
    mfl_freetree(originaltree, numnodes);
    mfl_freetree(copiedtree, numnodes);*/
    
    //pauseit();
    
    return 0;
}
