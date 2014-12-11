/*============================================================================
Copyright (c) 2012 Broadcom Europe Ltd. All rights reserved.
Copyright (c) 2001 Alphamosaic. All rights reserved.

Project  :  C6357
Module   :  Scrabble game (scrb)
File     :  $RCSfile: board.c,v $
Revision :  $Revision: 1.12 $

FILE DESCRIPTION
Main program for scrabble game.

MODIFICATION DETAILS
$Log: board.c,v $
Revision 1.12  2006/06/23 21:03:47  dc4
Removed hardware.h

Revision 1.11  2006/02/20 15:16:23  dc4
Updated includes

Revision 1.10  2003/09/09 15:29:48  dc4
Chunky menus

Revision 1.9  2003/08/18 10:10:22  dc4
Added number of players, levels, dictionary lookup

Revision 1.8  2003/08/04 10:31:37  dc4
Added rack leave, proper human playing

Revision 1.7  2003/07/22 10:08:03  dc4
Added user move

Revision 1.6  2003/07/14 08:14:51  dc4
Does proper scoring

Revision 1.5  2003/07/10 00:26:13  dc4
Faster checking of anchors

Revision 1.4  2003/07/07 14:38:20  dc4
Fixed last_anchor, speedups

Revision 1.3  2003/07/02 21:07:10  dc4
Fixed made up words, blanks shown

Revision 1.2  2003/07/01 21:13:58  dc4
Added blank support

Revision 1.1  2003/06/30 09:50:13  dc4
Updates to make moves

Revision 1.4  2003/06/17 17:14:34  dc4
Updated mw files

Revision 1.3  2003/06/17 15:42:48  pfcd
Added some board graphics

Revision 1.2  2003/06/17 11:25:41  dc4
Added mw build file

Revision 1.1  2003/06/17 10:45:57  pfcd
Added to cvs


=============================================================================*/


/* Project level */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
Functions in other modules accessed by this file.
Specify through module public interface files.
******************************************************************************/

#include "scrabble.h"

void boardflip(BOARD_T *board)
{
   int x,y,t;
   for(y=0;y<LEN;y++)
   for(x=0;x<y;x++) {
      t = board->letter[x][y];
      board->letter[x][y] = board->letter[y][x];
      board->letter[y][x] = t;
      t = board->mask[x][y];
      board->mask[x][y] = board->mask[y][x];
      board->mask[y][x] = t;
      t = board->score[x][y];
      board->score[x][y] = board->score[y][x];
      board->score[y][x] = t;
   }
}

void boardreset(BOARD_T *board)
{
   int x,y;
   for(y=0;y<LEN;y++)
   for(x=0;x<LEN;x++)
   {
      SETBOARDLETTER(board, x-1, y-1, 0);
      SETBOARDMASK(board, x-1, y-1, 0);
      SETBOARDSCORE(board, x-1, y-1, 0);
   }
}

char *checkmove(BOARD_T *board, GUI_RACK_T *guirack, MOVE_T *move)
{
   GUI_LETTER_T *letters = guirack->letters;
   int i, numletters = 0;
   DIRECTION_T direction;
   int first=-1, last=-1, row=-1;

   move->score=0;   
   for (i=0; i<guirack->numletters; i++)
      if (!letters[i].onrack) {
         if (first < 0) first = i;
         last = i;
         numletters++;
      }

   if (numletters == 0) {
      //assert(0);
      return "NO TILES PLACED";
   }

   if (numletters == 1) {
      if (GETBOARDLETTER(board, letters[first].tilex, letters[first].tiley-1) || GETBOARDLETTER(board, letters[first].tilex, letters[first].tiley+1)) {
         direction=VERTICAL;      
         row = letters[first].tilex;
      } else {
         direction=HORIZONTAL;
         row = letters[first].tiley;     
      }
   } else if (letters[first].tiley != letters[last].tiley) {
      direction=VERTICAL;      
      row = letters[first].tilex;
   } else {
      direction=HORIZONTAL;
      row = letters[first].tiley;
   }
   
   for (i=0; i<guirack->numletters; i++) {
      if (letters[i].onrack) continue;
      // check on board
      if (letters[i].tilex < 0 || letters[i].tilex >= BOARDSIZE || letters[i].tiley < 0 || letters[i].tiley >= BOARDSIZE) {
         assert(0);
         return "TILE PLACED OFF BOARD";
      }
      // check square available
      if (GETBOARDLETTER(board, letters[i].tilex, letters[i].tiley)) {
         assert(0);
         return "SQUARE NOT EMPTY";
      }
      // check all tiles in line
      if (direction == HORIZONTAL && letters[first].tiley != letters[i].tiley) {
         assert(0);
         return "NOT IN HORIZONTAL LINE";
      }
      else if (direction == VERTICAL && letters[first].tilex != letters[i].tilex) {
         assert(0);
         return "NOT IN VERTICAL LINE";
      }
   }
   first = -1; last = -1;
   for (i=0; i<guirack->numletters; i++) {
      if (letters[i].onrack) continue;
      // find first and last tile of word
      if (direction == HORIZONTAL && (first==-1 || letters[i].tilex < first))
         first = letters[i].tilex;
      else if (direction == VERTICAL && (first==-1 || letters[i].tiley < first))
         first = letters[i].tiley;
      if (direction == HORIZONTAL && (last==-1 || letters[i].tilex > last))
         last = letters[i].tilex;
      else if (direction == VERTICAL && (last==-1 || letters[i].tiley > last))
         last = letters[i].tiley;
   }
   
   move->nletters = 0;
   while (1) {
      int gap = direction == HORIZONTAL ? GETBOARDLETTER(board, first, row) == 0 : GETBOARDLETTER(board, row, first) == 0;
      for (i=0; i<guirack->numletters; i++) {
         if (letters[i].onrack) continue;
         if (direction == HORIZONTAL && gap && letters[i].tilex == first) {
            move->letter[move->nletters++] = get_letter_name(letters[i].letter);
            gap=0;
            break;
         } else if (direction == VERTICAL && gap && letters[i].tiley == first) {
            move->letter[move->nletters++] = get_letter_name(letters[i].letter);
            gap=0;
            break;
         }
      }
      if (gap) break;
      first++;
   }
   // must be gaps in word
   if (move->nletters != numletters) {
      assert(0);
      return "GAPS IN WORD";      
   }
   move->direction = direction;
   if (direction == HORIZONTAL) {
      move->x = first; // now last+1
      move->y = row;
   } else {
      move->x = first;  // swapped ?
      move->y = row;
   }
      
   // must be okay
   return 0;
}


int boardtext(BOARD_T *board, char *string, int x, int y)
{
   int len=0;
   while (*string) {
      SETBOARDLETTER(board, x, y, get_letter_index(*string));
      x++; string++; len++;
   }
   return len;
}


int boardword(BOARD_T *board, RACK_T *rack, MOVE_T *move, int letterstoplace)
{
   int letterPos;
   int c = move->x;
   
   assert(move->nletters);

   if (move->direction == HORIZONTAL) {
      // this is space after word
      assert(GETBOARDLETTER(board, move->x, move->y) == 0);
      for (letterPos = move->nletters - 1; letterPos >= 0; letterPos--) {
         // Find the spot where we'll place this letter
         while (GETBOARDLETTER(board, --c, move->y) != 0);
         assert(GETBOARDLETTER(board, c, move->y) == 0);
         // where is this place really?
         assert(c >= 0 && c < BOARDSIZE);
         assert(move->y >= 0 && move->y < BOARDSIZE);
         SETBOARDLETTER(board, c, move->y, get_letter_index(move->letter[letterPos]));
         rack_remove_letter(rack, move->letter[letterPos]);
         if (--letterstoplace==0) break;
      }
   } else {
      // this is space after word
      assert(GETBOARDLETTER(board, move->y, move->x) == 0);
      for (letterPos = move->nletters - 1; letterPos >= 0; letterPos--) {
         // Find the spot where we'll place this letter
         while (GETBOARDLETTER(board, move->y, --c) != 0);
         assert(GETBOARDLETTER(board, move->y, c) == 0);
         // where is this place really?
         assert(c >= 0 && c < BOARDSIZE);
         assert(move->y >= 0 && move->y < BOARDSIZE);
         SETBOARDLETTER(board, move->y, c, get_letter_index(move->letter[letterPos]));
         rack_remove_letter(rack, move->letter[letterPos]);
         if (--letterstoplace==0) break;
      }
   }
   return 0;
}

char *board_getword(BOARD_T *board, char *word, int x, int y, DIRECTION_T direction)
{
   int i;
   if (direction == HORIZONTAL) {
      assert(GETBOARDLETTER(board, x, y));
      while (GETBOARDLETTER(board, x, y))
         x--;
      assert(!GETBOARDLETTER(board, x, y));
      for (i=0; i<BOARDSIZE; i++) {
         char w = GETBOARDLETTER(board, ++x, y);
         if (w) word[i] = get_letter_name(w);
         else { word[i] = 0;  break; }
      }
   } else {
      assert(GETBOARDLETTER(board, x, y));
      while (GETBOARDLETTER(board, x, y))
         y--;
      assert(!GETBOARDLETTER(board, x, y));
      for (i=0; i<BOARDSIZE; i++) {
         char w = GETBOARDLETTER(board, x, ++y);
         if (w) word[i] = get_letter_name(w);
         else { word[i] = 0;  break; }
      }
   }   
   return i > 1 ? word:0;
}


int count_letters(signed char *letters)
{
   int i, count=0;
   for (i=0; i<MAXLETTER; i++)
      count += letters[i];
   return count;
}

int sock_fill(SOCK_T *sock)
{
   int i, count=0;
   for (i=0; i<MAXLETTER; i++) {
      sock->letters[i] = letter_quantity[i];
      count += sock->letters[i];
   }
   return count;
}

int rack_init(RACK_T *rack)
{
   int i;
   for (i=0; i<MAXLETTER; i++) {
      rack->letters[i] = 0;
   }
   return 0;
}

// add up tile scores in rack
int rack_score(RACK_T *rack)
{
   int i, score=0;
   for (i=0; i<MAXLETTER; i++) {
      score += rack->letters[i] * get_letter_value(i);
   }
   return score;
}

int rack_remove_letter(RACK_T *rack, int letter)
{
   int index = get_letter_tile(letter);
   assert(rack->letters[index] > 0);
   rack->letters[index]--;
   return 0;
}

int rack_empty(RACK_T *rack, MOVE_T *move)
{
   int i;
   for(i=0; i<move->nletters; i++) {
      int index = get_letter_tile(move->letter[i]);
      assert(rack->letters[index] > 0);
      rack->letters[index]--;
   }   
   return 0;
}

int get_letter(signed char *letters, int pick)
{
   int letter = 0, letcount=0;
   while (pick >= 0) {
      if (letcount >= letters[letter]) {
         letcount = 0; letter++;
      } else {
         letcount++; pick--;
      }
   }
   return letter;   
}

int rack_fill(SOCK_T *sock, RACK_T *rack)
{
   int rack_count = count_letters(rack->letters);
   int sock_count = count_letters(sock->letters);
   while (rack_count < TILES_PER_MOVE && sock_count > 0) {
      // pick a tile from sock 
      int pick = rand() % sock_count;   
      int letter = get_letter(sock->letters, pick);
      assert(sock->letters[letter]>0);
      sock->letters[letter]--;
      rack->letters[letter]++;    
      rack_count++;
      sock_count--;
   }   
   return rack_count;
}

int rack_set(SOCK_T *sock, RACK_T *rack, RACK_T *desired)
{
   int i;
   for (i = 0; i < MAXLETTER; i++) {
      sock->letters[i] -= desired->letters[i];
      assert(sock->letters[i] >= 0);
      rack->letters[i] += desired->letters[i];
   }
   assert(count_letters(rack->letters) <= TILES_PER_MOVE);
   return 0;
}

int rack_unfill(SOCK_T *sock, RACK_T *rack)
{
   int rack_count = count_letters(rack->letters);
   int sock_count = count_letters(sock->letters);
   while (rack_count > 0) {
      // put tile back in sock 
      int letter = get_letter(rack->letters, 0);
      assert(rack->letters[letter]>0);
      rack->letters[letter]--;
      sock->letters[letter]++;    
      sock_count++;
      rack_count--;
   }   
   return rack_count;
}
