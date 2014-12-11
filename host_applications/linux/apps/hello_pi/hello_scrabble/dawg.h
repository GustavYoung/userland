
    /*
            Description of the dawg data structure

    The dawg is stored as an array of edges, each edge
    stored in an int.  Each node is represented by the
    index into this array of the first edge leaving
    that node; subsequent edges leaving the same
    node occupy successive locations in the array.
    The last edge leaving a node is flagged by a
    bit. Edges leading to terminal nodes (those which
    are completed words) are marked with another bit.
    The edges are labelled with character numbers from
    1:26 (1=a) and occupy one 32 bit word each.  The
    node with index 0 is the special node with no
    edges leaving it.

    Bits in format of edges in unsigned 32-bit word:

        | . . . : . . . | . . . : . . . | . . . : . . . | . . . : . . . |
         L W S <-------> * <------------------------------------------->
                 char            Index of edge pointed to in array       

        L: this is the LAST edge out of this node
        W: this edge points to a node that completes a WORD
        S: this edge points to a node that completes a SUBWORD (word from smaller dictionary)
        *: currently spare

    Be careful with the distinction between an edge and an edge position.
    */
#define DAWG_LAST      0x80000000
#define DAWG_WORD      0x40000000
#define DAWG_SUBWORD   0x20000000
#define DAWG_CHAR_MASK  0x1F
#define DAWG_CHAR_SHIFT  22
#define DAWG_INDEX_MASK  0x001fffff
#define DAWG_INDEX_SHIFT  0


DAWG_T *dawg_init(char *name);

/* Test function to generate a text string from the dawg */
void dawg_generate(DAWG_T *dawg,int node,char *text,int num);

/* Test function to check a text string against the dawg */
int dawg_check(DAWG_T *dawg,int node,char *text,int num);
int dawg_check_blank(DAWG_T *dawg,int node,char *text,int num,int *mask, LEVEL_T level);
int dawg_traverse(DAWG_T *dawg,int node,char *text,int num);
int dawg_countsubtree(DAWG_T *dawg, int edge, int *words, int len);
int dawg_countsuffixes(DAWG_T *dawg, int edge, char *prefix, int num, int *words);

#define GETCHAR(edge)  (((edge)>>DAWG_CHAR_SHIFT)&DAWG_CHAR_MASK)
#define GETMASK(edge)  (((edge)>>DAWG_INDEX_SHIFT)&DAWG_INDEX_MASK)
#define GETINDEX(edge) (((edge)>>DAWG_INDEX_SHIFT)&DAWG_INDEX_MASK)


static int dawg_ends_word(int edge, LEVEL_T level)
{
   if (level == LEVEL_SUBDICTIONARY)
      return (edge & DAWG_SUBWORD) != 0;
   return (edge & DAWG_WORD) != 0;
}
