#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "scrabble.h"

#define max(a,b) ((a)>(b)?(a):(b))

/* Load in a dawg and return a handle 
Return NULL if none found
*/
DAWG_T *dawg_init(char *name)
{
   FILE *fid=fopen(name,"rb");
   DAWG_T *dawg;
   int index;
   
   if (fid==NULL) return NULL;
   dawg=(DAWG_T*)malloc(sizeof(DAWG_T));
   if (dawg == NULL)
      return NULL;
   
   fseek(fid,0,SEEK_END);
   dawg->len=ftell(fid);
   fseek(fid,0,SEEK_SET);
   
   dawg->nedges=dawg->len/4; /* 4 bytes in an int */
   dawg->data=(int*)malloc(4*dawg->nedges);
   if (dawg->data == NULL)
      return NULL;
   
   /* Data contains name plus data */
   fread(&dawg->skip,4,1,fid);
   dawg->skip>>=2;
   index=dawg->skip<<2;
   fseek(fid,index,SEEK_SET);
   fread(dawg->data,4,dawg->nedges-dawg->skip,fid); /* Read all data */
   fclose(fid);
   dawg->root=dawg->data[1]; /* Not clear why? */
   return dawg;
}

/* Test function to generate a text string from the dawg */
void dawg_generate(DAWG_T *dawg,int edge,char *text,int num)
{
   int count;
   unsigned int r=0;
   int index=GETINDEX(edge);
   
   if ((index==0) || (num==0))
   {
      text[0]=0;
      return;
   }
   
   for(count=0;!(dawg->data[index+count]&DAWG_LAST);count++)
      ;
   /* Now pick a random number between 0 and count */
   if (count>0)
      r=rand()%count;
   edge=dawg->data[index+r];
   
   text[0]=GETCHAR(edge);
   dawg_generate(dawg,edge,text+1,num-1); 
}

/* Test function to check a text string from the dawg */
int dawg_check(DAWG_T *dawg,int edge,char *text,int num)
{
   int index=GETINDEX(edge);
   
   if ((index==0) || (num==0))
      return 0;
   assert(index);
   assert(num);
   
   do
   {
      edge=dawg->data[index];
      if (text[0]==GETCHAR(edge))
      {
         /* Match */
         if (num==1)
         {
            return dawg_ends_word(edge, LEVEL_JUSTCHECK);
         }
         return dawg_check(dawg,edge,text+1,num-1); 
      }
   } while (!(dawg->data[index++]&DAWG_LAST));
   
   return 0; 
}

/* Test function to check a text string from the dawg */
int dawg_check_blank(DAWG_T *dawg, int edge, char *text, int num, int *mask, LEVEL_T level)
{
   int index=GETINDEX(edge);

   if ((index==0) || (num==0))
      return 0;
   assert(index);
   assert(num);
   
   if (text[0]) {
      do
      {
         edge=dawg->data[index];
         if (text[0]==GETCHAR(edge))
         {
            /* Match */
            if (num==1)
            {
               return dawg_ends_word(edge, level);
            }
            return dawg_check_blank(dawg, edge, text+1, num-1, mask, level); 
         }
      } while (!(dawg->data[index++]&DAWG_LAST));
   
      return 0;    
   } else {
      int flag = 0;
      do
      {
         edge=dawg->data[index];
         {
            /* Match */
            if (num==1)
            {
               if (dawg_ends_word(edge, level)) {
                  assert(GETCHAR(edge));
                  assert(GETCHAR(edge)<=32);
                  *mask |= 1<<(GETCHAR(edge));
                  flag = 1;
               }
               continue;
            }
            if (dawg_check_blank(dawg, edge, text+1, num-1, mask, level)) {
               assert(GETCHAR(edge));
               assert(GETCHAR(edge)<=32);
               *mask |= 1<<(GETCHAR(edge));
               flag = 1;
            }
         }
      } while (!(dawg->data[index++]&DAWG_LAST));
   
      return flag;   
   }
}

 /**
 * Traverse from edge e along nodes already on the board,
 * starting at column 'c'.  Return final edge.
 **/
int dawg_traverse(DAWG_T *dawg, int edge, char *text, int num)
{
   int node = GETINDEX(edge);

   if (num == 0)
      return edge;
   if (node == 0)
      return 0;

   do {
      edge = dawg->data[node++];
      if (GETCHAR(edge) == text[0])
         return dawg_traverse(dawg,edge,text+1,num-1);
   } while ((edge & DAWG_LAST) == 0);

   return 0;
}

 /**
 * Traverse from edge e along nodes already on the board,
 * starting at column 'c'.  Return final edge.
 **/
static char whole[32];
static char *global_block;
int dawg_set(DAWG_T *dawg,int *edge, int depth)
{
   int node = GETINDEX(*edge);

   if ((*edge) & DAWG_WORD) {
      *edge |= DAWG_SUBWORD;
   }

   if (node == 0)
      return 0;

   do {
      edge = &dawg->data[node++];
      whole[depth] = 'a'-1+GETCHAR(*edge);
      dawg_set(dawg,edge,depth+1);
   } while (((*edge) & DAWG_LAST) == 0);

   return 0;
}
int dawg_print(DAWG_T *dawg,int *edge, int depth, LEVEL_T level)
{
   int node = GETINDEX(*edge);
   //assert(!((*edge) & DAWG_SUBWORD));

   if (dawg_ends_word(*edge, level)) {
      assert(depth);
      printf("%.*s\n", depth, whole);
   }

   if (node == 0)
      return 0;

   do {
      edge = &dawg->data[node++];
      whole[depth] = 'a'-1+GETCHAR(*edge);
      dawg_print(dawg, edge, depth+1, level);
   } while (((*edge) & DAWG_LAST) == 0);

   return 0;
}
int dawg_whole(DAWG_T *dawg,int *edge, int depth)
{
   int node = GETINDEX(*edge);
   assert(global_block);
   //assert(!((*edge) & DAWG_SUBWORD));

   if ((*edge) & DAWG_WORD) {
      int cmp;
      assert(depth);
compare_point:      
      cmp = strncmp(global_block, whole, depth);
      if (cmp == 0 && global_block[depth] != 0)
         cmp = 1;
      if (cmp == 0) {
         //*edge |= DAWG_SUBWORD;
         global_block += depth;
         assert(global_block[0] == 0);
         global_block++;      
      } else if (cmp < 0) { // big dawg word after small word
         global_block += strlen(global_block);
         assert(global_block[0] == 0);
         global_block++;      
         goto compare_point;
      } else { // big dawg word before small word
         // just skip
         *edge &= ~DAWG_SUBWORD;
      }
   }

   if (node == 0)
      return 0;

   do {
      edge = &dawg->data[node++];
      whole[depth] = 'a'-1+GETCHAR(*edge);
      dawg_whole(dawg,edge,depth+1);
   } while (((*edge) & DAWG_LAST) == 0);

   return 0;
}

int dawg_make_subdawg(DAWG_T *dawg, char *subfile)
{
   FILE *fid = fopen(subfile, "rt");
   char *block;
   int i, len;
   
   if (!fid) {
      assert(0);
      return 0;
   }
   
   fseek(fid,0,SEEK_END);
   len=ftell(fid);
   fseek(fid,0,SEEK_SET);
   block = (char*)malloc(len);
   if (!block) {
      assert(0);
      return 0;
   }
   fread(block,1,len,fid); /* Read all data */
   fclose(fid);
   
   for (i=0; i<len; i++)
      if (block[i]=='\n') block[i] = '\0';
   global_block = block;
   dawg_set(dawg, &dawg->root, 0);
   dawg_whole(dawg, &dawg->root, 0);
   //dawg_print(dawg, &dawg->root, 0, LEVEL_SUBDICTIONARY);
   free(block);
   return 1;
}

/* count words in subtree */
int dawg_countsubtree(DAWG_T *dawg, int edge, int *words, int len)
{
   int count = 0;
   int node=GETINDEX(edge);
   
   if (edge & DAWG_WORD) {
      if (len==0)
         words[0] = -1;
      count ++;
   }
   
   if (node==0)
      return count;
         
   do {
      int c;
      edge = dawg->data[node++];
      c = dawg_countsubtree(dawg, edge, words, len+1);
      if (len==0) {
         if (0) //(edge & DAWG_WORD)
            words[GETCHAR(edge)] = -1;
         else
            words[GETCHAR(edge)] = c;
      }
      count += c;
   } while ((edge & DAWG_LAST) == 0);

   return count;
}

int max_count(int *words)
{
   int i, max = 1;
   for (i=0; i<MAXLETTER; i++)
      max = max(max, words[i]);   
   return max;
}

/* count words in subtree */
int dawg_countsuffixes(DAWG_T *dawg, int edge, char *prefix, int num, int *words)
{
   int addition, count, i;
   edge = dawg_traverse(dawg, edge, prefix, num);
   for (i=0; i<MAXLETTER; i++)
      words[i] = 0;
   count = dawg_countsubtree(dawg, edge, words, 0);

   // check if end of word possible
   for (i=0; i<MAXLETTER; i++)
      if (words[i] < 0) 
         words[i] = max_count(words);
   
   addition = max(1, count / 50);
   for (i=0; i<MAXLETTER; i++)
      words[i] = (words[i] + addition) * (256 * 256) /(count+addition*MAXLETTER);
   return count;
}
