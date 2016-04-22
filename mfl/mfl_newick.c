/*
 *  mfl_newick.c
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


#include "morphy.h"

/*!
 Checks if newick string is valid.
 @param newick_input a pointer to a character string.
 @returns int whether the newick tree is invalid (0) or valid (1).
 */
int mfl_is_valid_newick(char *newick_input)
{
    int i = 0;
    int is_valid = 1;
    
    int n_open_brackets = 0;
    int n_closed_brackets = 0;
    int n_commas = 0;
    bool terminal_semicolon = false;
    bool rooting_specifier = false;
    
    for (i = 0; *(newick_input + i) != '\0'; ++i) {
        if (*(newick_input + i) == '(') {
            ++n_open_brackets;
        }
        else if (*(newick_input + i) == ')') {
            ++n_closed_brackets;
        }
        else if (*(newick_input + i) == ',') {
            ++n_commas;
        }
        if (*(newick_input + i) == ';') {
            terminal_semicolon = true;
        }
        if (*(newick_input + i) == '&') {
            rooting_specifier = true;
        }
    }

    if (!terminal_semicolon) {
        dbg_printf("Error in mfl_check_valid_newick(): no terminal semicolon\n");
        return is_valid = 0;
    }
    
    if (!rooting_specifier) {
        dbg_printf("Error in mfl_check_valid_newick(): no rooting specifier\n");
        return is_valid = 0;
    }
    
    if (n_open_brackets != n_closed_brackets) {
        dbg_printf("Error in mfl_check_valid_newick(): bracket number mismatch: ");
        if (n_open_brackets > n_closed_brackets) {
            dbg_printf("open brackets outnumber closed brackets\n");
        }
        else {
            dbg_printf("closed brackets outnumber opening brackets\n");
        }
        return is_valid = 0;
    }
    
    return is_valid;
}

/*!
 Counting the number of internal nodes in a newick string.
 @param newick_string a pointer to a character string.
 @returns int the number of internal nodes.
 */
int mfl_count_internal_nodes_in_newick(char *newick_string)
{
    
    int num_open = 0;
    int num_closed = 0;
    int num_comma = 0;
    
    if (*newick_string != '(') {
        dbg_printf("ERROR in function calling mfl_count_internal_nodes_in_newick(): pointer is not at start of valid Newick string\n");
        return 0;
    }
    
    while (*newick_string != ';') {
        if (*newick_string == '(') {
            ++num_open;
        }
        if (*newick_string == ')') {
            ++num_closed;
        }
        if (*newick_string == ',') {
            ++num_comma;
        }
        ++newick_string;
    }
    
    return num_comma;
    
}

/*!
 Checks if a newick tree is rooted or unrooted.
 @param newick_string a pointer to a character string.
 @returns bool whether the tree is rooted (true) or unrooted (false).
 */
bool mfl_newick_tree_is_rooted(char *newick_string)
{
    bool is_rooted = false;
    
    while (*newick_string != '&') {
        ++newick_string;
    }
    
    ++newick_string;
    
    if (*newick_string != 'R' && *newick_string != 'U' && *newick_string != 'r' && *newick_string != 'u') {
        is_rooted = false;
        dbg_printf("Error in mfl_newick_string_is_rooted(): Newick data has no rooting specification\n");
    }
    else if (*newick_string == 'R' || *newick_string == 'r') {
        is_rooted = true;
        dbg_printf("Newick string is rooted\n\n");
    }
    else {
        dbg_printf("Newick string is unrooted\n\n");
    }
    
    return is_rooted;
}

/*!
 Reads a newick tip number.
 @param newick_position a pointer to a tip position in a newick string.
 @returns int the tip number.
 */
int mfl_read_newick_int(char **newick_position)
{
    int tip_number = 0;
    
    // Read char until ',' or ')' character, converting an int.
    do {
        tip_number = 10 * tip_number;
        tip_number = tip_number + (**(newick_position) - '0'); // This should convert the value at the current newick position to an int.
        ++(*newick_position);
    } while (**newick_position != ',' && **newick_position != ')');
    
    return tip_number;
}

/*!
 Locates the next opening bracket in a newick string.
 @param newick_tree a pointer to a character string.
 @returns char a pointer to the next opening bracket.
 */
char *mfl_find_next_opening_bracket_in_newick(char *newick_tree)
{
    if (*newick_tree == '(') {
        return newick_tree;
    }
    
    do {
        ++newick_tree;
    } while (*newick_tree != '(');
    
    return newick_tree;
}


/*!
 Finds the largest tip number in a newick string.
 @param newick_string a pointer to a character string.
 @returns int the number of the largest tip in the newick string.
 */
int mfl_seek_largest_tip_number_newick(char *newick_string)
{
    int retrieved_tip = 0;
    int largest_tip_number = 0;
    
    dbg_printf("The string to analyse: %s\n", newick_string);
    if (*newick_string != '(') {
        newick_string = mfl_find_next_opening_bracket_in_newick(newick_string);
    }
    
    while (*newick_string) {
        if (isdigit(*newick_string)) {
            retrieved_tip = mfl_read_newick_int(&newick_string);
            if (retrieved_tip > largest_tip_number) {
                largest_tip_number = retrieved_tip;
            }
        }
        ++newick_string;
    }
    
    return largest_tip_number;
}

/*!
 Generates a mfl_node_t object from a newick position.
 @param newick_position a pointer to a tip position in a newick string.
 @param nodearray the mfl_nodearray_t to where to store the node
 @param num_taxa an integer that is the number of taxa.
 @returns mfl_node_t a node of the tree in the mfl_tree_t structure.
 */
mfl_node_t * mfl_traverse_newick_recursively(char **newick_position, mfl_nodearray_t nodearray, int num_taxa)
{
    
    mfl_node_t *new_parent = NULL;  // A new parent node that will be returned from this function
    mfl_node_t *new_child = NULL;   // Child nodes returned from calls on this function
    mfl_node_t *node_ptr = NULL;    // For new node bases
    int tip_number = 0;
    
    if (**newick_position == ';') {
        dbg_printf("ERROR in mfl_traverse_newick_recursively(): no defined operation for terminal semicolon\n");
        return NULL;
    }
    if (**newick_position != '(') {
        dbg_printf("ERROR in mfl_traverse_newick_recursively(): called on invalid symbol\n");
        return NULL;
    }
    
    ++(*newick_position);
    
    new_parent = mfl_get_next_available_node(nodearray);
    new_parent->nodet_next = new_parent;
    
    if (new_parent->nodet_index == 7) {
        dbg_printf("Just wait\n");
    }
    
    do {
        if (**newick_position == '(') {
            node_ptr = mfl_get_node_from_nodestack(nodearray[0]->nodet_ndstack);//mfl_get_next_available_node(nodearray);
            mfl_insert_node_in_ring(new_parent, node_ptr);
            new_child = mfl_traverse_newick_recursively(newick_position, nodearray, num_taxa);
            mfl_join_node_edges(node_ptr, new_child);
            
        }
        if (isdigit(**newick_position)) {
            node_ptr = mfl_get_next_available_node(nodearray);
            tip_number = mfl_read_newick_int(newick_position);
            mfl_insert_node_in_ring(new_parent, node_ptr);
            new_child = nodearray[tip_number-1];
            mfl_join_node_edges(node_ptr, new_child);
        }
        if (**newick_position == ',') {
            ++(*newick_position);
        }
        if (isspace(**newick_position)) {
            ++(*newick_position);
        }
        
    } while (**newick_position != ')');
    
    ++(*newick_position);
    
    return new_parent;
}

/*!
 Converts a newick string into a mfl_tree_t tree.
 @param newick_tree a pointer to a character string.
 @param num_taxa an integer that is the number of taxa.
 @returns mfl_tree_t the mfl_tree_t structure.
 */
mfl_tree_t *mfl_convert_newick_to_mfl_tree_t(char *newick_tree, int num_taxa)
{
    int num_taxa_local = 0; // If the number of taxa is not supplied by the user, it is easy to calculate it from the Newick string.
    mfl_tree_t* tree_from_newick = NULL;
    char *newicktr_copy = NULL;
    char **newick_position = NULL; // A pointer to the Newick string that can be incremented during the recursion on the string.
    
    /* Do safety checks on the Newick string. */
    if (!mfl_is_valid_newick(newick_tree)) {
        dbg_printf("ERROR in function calling mfl_convert_newick_to_mfl_tree_t(): string passed to function is not in valid Newick format\n");
        return NULL;
    }
    
    newicktr_copy = newick_tree;
    
    /* Ensure that the recursion begins on the opening bracket */
    if (*newicktr_copy != '(') {
        newicktr_copy = mfl_find_next_opening_bracket_in_newick(newicktr_copy);
    }
    
    /* Ideally, Newick trees will never be supplied without an NTAX value.
     * However, if that fails, the safest operation I can come up with is
     * to find the largest tip number in the string and allocate at least
     * that many nodes. Then the tip array will always be large enough to
     * accommodate at leastmany terminals. MDB. */
    if (!num_taxa) {
        num_taxa_local = mfl_seek_largest_tip_number_newick(newicktr_copy);
    }
    else {
        num_taxa_local = num_taxa;
    }
    
    newick_position = &newicktr_copy;
    
    tree_from_newick = mfl_alloctree_with_nodes(num_taxa_local);
    
    tree_from_newick->treet_root = mfl_traverse_newick_recursively(newick_position, tree_from_newick->treet_treenodes, num_taxa_local);
    
    dbg_printf("The newick string processed: %s\n", newick_tree);
    
    /* Process the rooting options*/
    if (!mfl_newick_tree_is_rooted(newick_tree)) {
        mfl_unroot_tree(tree_from_newick);
    }
    
    return tree_from_newick;
}


/*!
 Counts the number of digits in an integer.
 @param n an integer.
 @returns int the number of digits in n.
 */
int mfl_number_of_digits_in_integer(int n)
{
    int count=0;

    //Counting the digits
    while(n!=0)
    {
        n/=10;
        ++count;
    }
  return(count);
}


/*!
 Traverse a mfl_tree_t to get the number of digits in the tips.
 @param start a mfl_node_t pointer to the starting node in the tree.
 @param tips_length an integer counter for the number of digits (ideally set to 
 0).
 @returns int the updated tips_length counter.
 */
void mfl_traverse_tree_to_get_tip_char_length(mfl_node_t *start, int *tips_length)
{
    // Set the node to start
    mfl_node_t *node = start;
    
    if (node->nodet_tip != 0) {
        // convert the tip into a character
        *tips_length = *tips_length + mfl_number_of_digits_in_integer(node->nodet_tip);
        return;
    }
        
    // Move to the next node
    node = node->nodet_next;
    
    do {
        // Go through the next nodes and print the next tip
        mfl_traverse_tree_to_get_tip_char_length(node->nodet_edge, tips_length);
        
        // Move to the next node
        node = node->nodet_next;
        
        // Stop once reached back the start tree
    } while (node != start);
    
    return;

}

/*!
 Traverse a mfl_tree_t to get the number of tips in the tree.
 @param start a mfl_node_t pointer to the starting node in the tree.
 @param num_taxa an integer counter for the number of tips (ideally set to 0).
 @returns the updated num_taxa counter.
 */
void mfl_traverse_mfl_tree_t_number_of_taxa(mfl_node_t *start, int* num_taxa)
{
    // Set the node to start
    mfl_node_t *node = start;
    
    if (node->nodet_tip) {
        // add to counter
        ++(*num_taxa);
        return;
    }
    
    // Move to the next node
    node = node->nodet_next;
    
    do {
        // Go through the next nodes and print the next tip
        mfl_traverse_mfl_tree_t_number_of_taxa(node->nodet_edge, num_taxa);
            
        // Move to the next node
        node = node->nodet_next;
            
        // Stop once reached back the start tree
    } while (node != start);
}


/*!
 Get the maximum number of characters in a newick string
 @param num_taxa an integer being the number of taxa present in the newick 
 string.
 @param start a mfl_node_t pointer to the starting node in the tree.
 @returns the maximum number of characters in the newick string.
 */
int mfl_number_of_characters_in_newick(int num_taxa, mfl_node_t *start)
{
    int num_brackets = 2 * (num_taxa - 1);
    int num_commas = num_taxa - 1;
    int newick_size = 0;
    int tips_length = 0;

    //Get the number of digits in the tips
    mfl_traverse_tree_to_get_tip_char_length(start, &tips_length);
    
    //Get the newick string size
    newick_size = tips_length + num_brackets + num_commas + 1; // The final includes the closing semi-colon (1)
    
    return newick_size;
}


/*!
 Traverse a mfl_tree_t to print each newick characters.
 @param start a mfl_node_t pointer to the starting node in the tree.
 @param newick_tree_out a character pointer to fill in with the newick 
 characters.
 @param count a counter for the newick characters position (ideally set to 0).
 */
void mfl_traverse_tree_to_print_newick_char_recursive(mfl_node_t *start, char *newick_tree_out, int* count)
{
    int i = 0;
    int tip_length;
    
    // Set the node to start
    mfl_node_t *node = start;
    
    if (node->nodet_tip != 0) {
        // convert the tip into a character
        tip_length = mfl_number_of_digits_in_integer(node->nodet_tip);
        
        char tip_num[tip_length+1];
        
        sprintf(tip_num, "%i", node->nodet_tip);
        // print a tip (depending on the number of integer in there)
        
        //i = mfl_number_of_digits_in_integer(node->nodet_tip);
        
        for (i = 0; i < tip_length; ++i) {
            newick_tree_out[*count] = tip_num[i];
            ++(*count);
        }
        
        return;
        
    } else {
        // open a clade
        newick_tree_out[*count] = '(';
        ++(*count);
    }
    
    // Move to the next node
    node = node->nodet_next;
    
    do {
        if (node != start->nodet_next) {
            newick_tree_out[*count] = ',';
            ++(*count);
        }
        // Go through the next nodes and print the next tip
        mfl_traverse_tree_to_print_newick_char_recursive(node->nodet_edge, newick_tree_out, count);
        
        // Move to the next node
        node = node->nodet_next;
    
    // Stop once reached back the start tree
    } while (node != start);
    
    newick_tree_out[*count] = ')';
    ++(*count);
}


char* mfl_alloc_empty_newick(int newick_string_length)
{
    char * newick_tree_out = (char*)malloc(newick_string_length * sizeof(char));
    
    if (!newick_tree_out) {
        dbg_printf("ERROR in mfl_convert_mfl_tree_t_to_newick(): unable to allocate memory for Newick string\n");
        return NULL;
    }
    else {
        memset(newick_tree_out, 0, newick_string_length * sizeof(char));
    }
    
    return newick_tree_out;
}


int mfl_get_num_active_taxa_in_tree(mfl_tree_t* input_tree)
{
    int num_taxa_active = 0;
    
    //Infer number of taxa from mfl_tree_t
    if (input_tree->treet_root) {
        mfl_traverse_mfl_tree_t_number_of_taxa(input_tree->treet_root, &num_taxa_active);
    }
    else {
        mfl_traverse_mfl_tree_t_number_of_taxa(input_tree->treet_start, &num_taxa_active);
        ++num_taxa_active;
    }
    
    return num_taxa_active;
}


int mfl_calculate_newick_length(mfl_tree_t* input_tree, int num_taxa)
{
    int newick_string_length = 0;
    
    if (input_tree->treet_root) {
        //Get the newick string length
        newick_string_length = mfl_number_of_characters_in_newick(num_taxa, input_tree->treet_root);
    }
    else {
        newick_string_length = mfl_number_of_characters_in_newick(num_taxa, input_tree->treet_start);
    }
    
    return newick_string_length;
}


void mfl_concatenate_newick_elements(char* destination, char* newick_substring, char* root_header)
{
    int i = 0;
    int j = 0;
    
    for (i = 0; root_header[i]; ++i) {
        destination[i] = root_header[i];
    }
    
    j = i;
    
    for (i = 0; newick_substring[i]; ++i, ++j) {
        destination[j] = newick_substring[i];
    }
    
    destination[j] = '\0';
    
    free(newick_substring);
}


char* mfl_get_newick_root_header(bool isrooted)
{
    if (isrooted) {
        return (char*)"[&R] ";
    }
    else {
        return (char*)"[&U] ";
    }
}

/*!
 Converts an input mfl_tree_t into a newick character string
 @param input_tree (mfl_tree_t*) a pointer to the mfl_tree_t object to convert.
 @param num_taxa (int) the active number of taxa in the tree. Can be set to 0 to
 be infered.
 @param root_polytomy (bool) if the tree is unrooted, whether arbitrarily root 
 the first node (polytomy = true) or the first edge (polytomy = false).
 @returns a character Newick string.
 */
char* mfl_convert_mfl_tree_t_to_newick(mfl_tree_t *input_tree, int num_taxa, bool root_polytomy)
{
    char* newicktr_substr = NULL;
    char *newick_tree_out = NULL;
    char* root_command = NULL;
    int newick_string_length = 0;
    int num_taxa_local = 0;
    int count = 0;
    
    // Adding starting with the root command
    
    if (input_tree->treet_root) {
        root_command = mfl_get_newick_root_header(true);
    } else {
        root_command = mfl_get_newick_root_header(false);
        
        //Add an arbitrary polytomy with a root at the starting node of the tree.
        if(root_polytomy == true) {
            mfl_root_target_node(input_tree, input_tree->treet_start);
        } else {
            mfl_root_target_edge(input_tree, input_tree->treet_start);
        }
    }
    
    
    if(!num_taxa) {
        num_taxa_local = mfl_get_num_active_taxa_in_tree(input_tree);
    }
    else {
        num_taxa_local = num_taxa;
    }
    
  
    newick_string_length = mfl_calculate_newick_length(input_tree, num_taxa_local);
    
    //Allocating memory to the newick
    newicktr_substr = mfl_alloc_empty_newick(newick_string_length+1);

    //Creating the newick string out
    mfl_traverse_tree_to_print_newick_char_recursive(input_tree->treet_root, newicktr_substr, &count);
    newicktr_substr[count] = ';';
    newicktr_substr[count + 1] = '\0';
    
    //Closing the tree
    newick_tree_out = (char*)malloc( (strlen(root_command) + strlen(newicktr_substr) + 1) * sizeof(char));
    if (!newick_tree_out) {
        dbg_eprintf("unable to allocate memory for Newick string");
        free(newicktr_substr);
        return NULL;
    } else {
        memset(newick_tree_out, 0, (strlen(root_command) + strlen(newicktr_substr) + 1 ) *sizeof(char));
    }

    mfl_concatenate_newick_elements(newick_tree_out, newicktr_substr, root_command);

    
    mfl_unroot_tree(input_tree);
    
    return newick_tree_out;

}

///*!
// Stores a newick string in an array
// @param storing_array (mfl_tree_t**) an array of pointers to pointers to mfl_tree_t trees
// @param newick_string (char*) a pointer to the newick string.
// @param count (int*) where to store the newick_string in the array
// @returns a pointer to mfl_tree_t trees.
// */
//mfl_tree_t** mfl_store_newick_strings(mfl_tree_t **storing_array, char *newick_string, int *count)
//{
     // MDB: Why does count need to be a pointer?
     // MDB: Why are you returning storing array? You've passed it in as a pointer, so
     // there's no need to return it. In fact, this could lead to some
     // unpredictable effects.
//    storing_array[*count] = mfl_convert_newick_to_mfl_tree_t(newick_string, 0);;
//    return storing_array;
//}
//
///*!
// Allocates memory for the storing array
// @param array_size (int) the size of the array to allocate
// @returns an empty storing array (mfl_tree_t **).
// */
//mfl_tree_t** mfl_allocate_storing_array(int array_size)
//{
//    MDB: This malloc will lead to an error. You're returning a pointer of pointers
//         to mfl_tree_t, but sizing it according to the size of a mfl_tree_t struct.
//         the size should be of an mfl_tree_t*.
//    mfl_tree_t **storing_array = (mfl_tree_t**)malloc(array_size * sizeof(mfl_tree_t));
//    
//    if (!storing_array) {
//        dbg_printf("ERROR in mfl_allocate_storing_array(): unable to allocate memory for the storing array\n");
//        return NULL;
//    }
//    else {
//        memset(storing_array, 0, array_size * sizeof(mfl_tree_t));
//    }
//    
//    return storing_array;
//}
//
///*!
// Stores some trees from an input file into an aray of character pointers
// @returns a storing array (char **) containing multiple newick trees.
// */
//// TODO: input the newicks from a i/o file
//    MDB: Don't handle any i/o operations in the library. Just deal with
//          handling the strings. However, simple file i/o for a list of
//          trees can be dealt with in the TUI.
//char** mfl_store_newick_in_array()
//{
//    
//}
//
///*!
// Frees an array of newick strings
// @param newick_storing_array (mfl_tree_t **) an array of characters pointers mfl_tree_t trees.
// */
// MDB: Why is the newick storing array of type mfl_tree_t**? Newick trees are char*
// and an array of them would be of type char**
//void mfl_free_newick_in_array(mfl_tree_t **newick_storing_array)
//{
//    int i = 0;
//    int array_length = 0;
//    
//    //Free the newick trees in the array
//    do {
//        free(newick_storing_array[i]);
//        ++i;
//    } while (newick_storing_array[i] != '\0');
//    
//    //Free the array
//    free(newick_storing_array);
//}