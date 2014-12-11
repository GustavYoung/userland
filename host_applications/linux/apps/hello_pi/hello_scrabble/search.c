#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "scrabble.h"
//#include "game.h"

#ifdef WIN32
#include <time.h>
#define game_time_microsecs() ((int)((float)clock() * 1000000.0f / CLOCKS_PER_SEC))
#endif

/* Globally exported variables */
static MOVE_T best_move;
static MOVE_T player_move;
static int state_endgame; // flag indicating sock has less than seven tiles in

static void clearmasks(BOARD_T *board)
{
   int i,j;
   for (j=0; j<BOARDSIZE; j++)
   for (i=0; i<BOARDSIZE; i++) {
      SETBOARDMASK(board, i, j, -1);
      SETBOARDSCORE(board, i, j, -1);
   }
}

static void setmasks(BOARD_T *board, DAWG_T *dawg, LEVEL_T level)
{
   int i,j,k,start_j,stop_j,mask,newmask,ret;
   char word[BOARDSIZE];
   for (j=0; j<BOARDSIZE; j++)
   for (i=0; i<BOARDSIZE; i++) {
      // if a square is empty but adjacent to tile, it may have a mask of anchor letters
      if (GETBOARDLETTER(board,i,j)) continue;
      if (j>0 && GETBOARDLETTER(board,i,j-1)) { // anchor below tile
         for (start_j = j-2; GETBOARDLETTER(board,i,start_j); start_j--) {}
         for (stop_j = j+1; GETBOARDLETTER(board,i,stop_j ); stop_j++) {}
      } else if (GETBOARDLETTER(board,i,j+1)) { // anchor above tile
         start_j = j-1;
         for (stop_j = j+2; GETBOARDLETTER(board,i,stop_j ); stop_j++) {}
      } else continue;

      for (k = 0; k<=(stop_j-1)-(start_j+1); k++) {
         word[k] = GETBOARDLETTER(board, i, k+(start_j+1)) &~ BLANKBIT;
         assert(word[k] || k==j-(start_j+1));
      }
      assert(k<=BOARDSIZE);
      // try each letter in 'hole' looking for forming of words
      newmask = 0;
      mask=-1; //GETBOARDMASK(board,i,j);
      ret=dawg_check_blank(dawg, dawg->root, word, k, &newmask, level);
      // if valid store cross-score
      if (ret) {
         int score = 0;
         for (k = 0; k<=(stop_j-1)-(start_j+1); k++) {
            int letter = GETBOARDLETTER(board, i, k+(start_j+1));
            score += get_letter_value(letter);
         }
         SETBOARDSCORE(board, i, j, score);
      }
      //if (ret==0) newmask=0;
      assert(j-(start_j+1)>=0);
      assert(j-(start_j+1)<BOARDSIZE-1);
      assert((!ret) ^ (!!newmask));
      mask &= newmask;            
      SETBOARDMASK(board, i, j, mask);
   }
}


/* like memcmp but treats blanks as equal */
static int wordcmp(char *s1, char *s2, int len)
{
   while (len--) {
      if (((*s1 & BLANKBIT) || (*s1 == BLANK)) && ((*s2 & BLANKBIT) || (*s2 == BLANK))) *s1 = *s2;
      if (*s1++ != *s2++)
         return 1;
   }
   return 0;
}

static int bestReplacement=-1;

/** We've added letters such that we are about to place
 * our next tile at column 'c', and have just followed 'edge'
 * in the DAWG.
 **/

static int eval(BOARD_T *board, int i, int j, PLAYER_T *player) {
   MOVE_T *move = &player->move;
   RACK_T *rack = &player->rack;
   if (move->nletters==0) return 0;
   switch (player->level) {
      case LEVEL_WORDLENGTH:
      case LEVEL_JUSTCHECK:
      case LEVEL_HIGHESTSCORE:
      case LEVEL_SUBDICTIONARY: {
         int cross_score = 0, main_score = 0, main_mult = 1;
         int this_letter_score, letter, letterPos;
         int c = i;

         // this is space after word
         assert(GETBOARDLETTER(board, i, j) == 0);
         for (letterPos = move->nletters - 1; letterPos >= 0; letterPos--) {
            // Find the spot where we'll place this letter
            while (letter = GETBOARDLETTER(board, --c, j), letter != 0) {
               main_score += get_letter_value(letter);
            }
            assert(GETBOARDLETTER(board, c, j) == 0);
            // where is this place really?
            assert(c >= 0 && c < BOARDSIZE);
            assert(j >= 0 && j < BOARDSIZE);
            this_letter_score = get_letter_mult(c,j) * get_letter_value(get_letter_index(move->letter[letterPos]));
            main_score += this_letter_score;
            cross_score += GETBOARDSCORE(board, c, j)>=0 ? (this_letter_score + GETBOARDSCORE(board, c, j)) * get_word_mult(c,j):0;
            main_mult *= get_word_mult(c,j);
         }
         // score for existing prefix letters
         while (letter = GETBOARDLETTER(board, --c, j), letter != 0) {
            main_score += get_letter_value(letter);
         }
         move->score = cross_score + main_score * main_mult + (move->nletters==7 ? 50:0);
         if (player->level == LEVEL_WORDLENGTH) 
            move->leave = 2 * (move->nletters - move->score);
         else
            move->leave = leave_eval(rack, state_endgame ? LEAVE_ENDGAME:0);

         if (player->level == LEVEL_JUSTCHECK &&
            player_move.x == i && 
            player_move.y == j &&
            player_move.direction == move->direction && 
            player_move.nletters == move->nletters && 
            wordcmp(player_move.letter, move->letter, move->nletters)==0) {
            player_move.score = move->score;
            player_move.leave = move->leave;
         }
         break;
      }
      default:
         assert(0);
         break;
   }

   // we double scores are leaves are measured in half units
   if (2*move->score+move->leave > 2*best_move.score+best_move.leave) {
#if 0
      best_move = *move;
      best_move.x = i;
      best_move.y = j;
#else
      best_move.score = move->score;
      best_move.leave = move->leave;
      best_move.x = i;
      best_move.y = j;
      best_move.direction = move->direction;
      best_move.nletters = move->nletters;
      memcpy(best_move.letter, move->letter, move->nletters);
#endif
   }
   return 0;
}

static void extend(DAWG_T *dawg, int edge, BOARD_T *board, PLAYER_T *player, int c, int j)
{
   MOVE_T *move = &player->move;
   RACK_T *rack = &player->rack;
   int node = GETINDEX(edge);

   // character at this column
   int ch = GETBOARDLETTER(board, c, j) &~ BLANKBIT;

   // Maybe we've completed a word
   if (dawg_ends_word(edge, player->level) && (ch == 0)) {
      eval(board, c, j, player);
   }

   // Go further only if we can proceed in the DAWG
   if (node == 0) return;

   if (ch == 0) {

      // There IS NOT a letter at c
      // Try all the possible edges from node,
      // making sure they don't stuff up cross-words.
      do {
         int ech;
         edge = dawg->data[node++];
         // letter on outgoing edge
         ech  = GETCHAR(edge);

         // if letter on edge is possible crossmove
         if ((GETBOARDMASK(board, c, j) & (1 << ech)) != 0) {
            // remove letter from rack, continue looking, put letter back in rack
            if (rack->letters[ech] > 0) {
               rack->letters[ech]--;
               move->letter[move->nletters++] = get_letter_name(ech);
               extend(dawg, edge, board, player, c + 1, j);
               move->nletters--;
               rack->letters[ech]++;
            // only one option for blank in quick mode
            // remove blank from rack, continue looking, put blank back in rack
            } /*else xxx dc4 */if (rack->letters[BLANK] > 0 && ((1<<ech) & bestReplacement) != 0) {
               rack->letters[BLANK]--;
               move->letter[move->nletters++] = get_letter_name(ech | BLANKBIT);
               extend(dawg, edge, board, player, c + 1, j);
               move->nletters--;
               rack->letters[BLANK]++;
			   }
         }
      } while ((edge & DAWG_LAST) == 0);
   } else {
      int ech;
      // There IS a letter at 'c'.
      // Try to match it with the edges coming
      // from this node.
      do {
         edge = dawg->data[node++];
         // letter on outgoing edge
         ech  = GETCHAR(edge);

         if (ech == ch) {
            extend(dawg, edge, board, player, c + 1, j);
         }
      } while ((ech < ch) && (edge & DAWG_LAST) == 0); /* XXX was <= */
   }
}

/** Generate all the word prefixes with length <= maxprefix,
 * starting at Dawg edge 'e'.  For every prefix, also try to
 * extend it to the right from extendCol.
 **/
static void prefix(DAWG_T *dawg, int edge, BOARD_T *board, PLAYER_T *player, int maxprefix, int extendCol, int j)
{
   MOVE_T *move = &player->move;
   RACK_T *rack = &player->rack;
   int node;
   extend(dawg, edge & ~(DAWG_SUBWORD|DAWG_WORD), board, player, extendCol, j);

   if (maxprefix == 0) return;
        
   node = GETINDEX(edge);

   // Go further only if we can proceed in the DAWG
   if (node == 0) return;

   do {
      int ch;
      // loop through all edges
      edge = dawg->data[node++];

      // find character of edge
      ch = GETCHAR(edge);

      // remove letter from rack, continue looking, put letter back in rack
      if (rack->letters[ch] > 0) {
         rack->letters[ch]--;
         move->letter[move->nletters++] = get_letter_name(ch);
         prefix(dawg, edge, board, player, maxprefix - 1, extendCol, j);
         move->nletters--;
         rack->letters[ch]++;
      // only one option for blank in quick mode
      // remove blank from rack, continue looking, put blank back in rack
      } /*else xxx dc4 */if (rack->letters[BLANK] > 0 && ((1<<ch) & bestReplacement) != 0) {
         rack->letters[BLANK]--;
         move->letter[move->nletters++] = get_letter_name(ch | BLANKBIT);
         prefix(dawg, edge, board, player, maxprefix - 1, extendCol, j);
         move->nletters--;
         rack->letters[BLANK]++;
      }
   } while ((edge & DAWG_LAST) == 0);
}

static int search_start_first_move(DAWG_T *dawg, BOARD_T *board, PLAYER_T *player)
{
   prefix(dawg, dawg->root, board, player, MIDDLEROW - 1, MIDDLEROW, MIDDLEROW);
   return 0;
}

static int search_do(DAWG_T *dawg, BOARD_T *board, PLAYER_T *player, int ticks)
{
   MOVE_T *move = &player->move;
   int mask, last_anchor;
   int i,j;

   for (; move->current_row<BOARDSIZE; move->current_row++) {
      j = move->current_row;
      for (; move->current_col<BOARDSIZE; move->current_col++) {
         i=move->current_col;
         if (ticks && (int)game_time_microsecs() - move->start_ticks > ticks) {
            move->current_col = i;
            move->current_row = j;
            return 1;
         }
         mask = GETBOARDMASK(board, i, j);
         if (mask==0) continue; // no letter playable
         if (GETBOARDLETTER(board, i, j)) continue;
         if (mask==-1 && GETBOARDLETTER(board, i-1, j) == 0 && GETBOARDLETTER(board, i+1, j) == 0) continue;
         assert(GETBOARDLETTER(board, i, j) == 0);         
         
         if (i>0 && GETBOARDLETTER(board, i-1, j)==0) {
            // The square behind us is blank.
            for (last_anchor = i-1; last_anchor>=0 && GETBOARDMASK(board, last_anchor, j)==-1; last_anchor--) {
               if (GETBOARDLETTER(board, last_anchor, j)) {
                  last_anchor++; // gone too far - hit previour letter
                  break;
               }              
            }
            assert(GETBOARDLETTER(board, last_anchor, j) == 0);

            // In fact, behind us we have 'i-anchor-1' blank squares which themselves are next to blank
            // squares.  We therefore first generate ALL the word prefixes with at most i-anchor-1 letters.
            prefix(dawg, dawg->root, board, player, i - last_anchor - 1, i, j);
         } else {        
            // square behind us is used
            if (i==0) last_anchor = -1;
            else for (last_anchor = i-2; last_anchor>=0 && GETBOARDLETTER(board, last_anchor, j)!=0; last_anchor--)
               {}
            assert(GETBOARDLETTER(board, last_anchor, j) == 0);
            // This is a square straight after a used square.
            // The _first_ used square of the word stretching
            // back behind us must be at 'anchor+1', because 'anchor'
            // was the last non-filled square
            extend(dawg, dawg->root, board, player, last_anchor + 1, j);
         }
      }
      move->current_col = 0;      
   }
   return 0;
}


// return a bit mask of playable tiles on a square
int search_square(DAWG_T *dawg, BOARD_T *board, int x, int y)
{
   int mask=-1;
   clearmasks(board);      
   setmasks(board, dawg, LEVEL_JUSTCHECK);
   mask &= GETBOARDMASK(board,x,y);

   boardflip(board);
   clearmasks(board);      
   setmasks(board, dawg, LEVEL_JUSTCHECK);
   mask &= GETBOARDMASK(board,y,x);
   boardflip(board);
   
   return mask;
}

int search_start(DAWG_T *dawg, BOARD_T *board, PLAYER_T *player, SOCK_T *sock, int ticks)
{
   MOVE_T *move = &player->move;
   int firstmove = !GETBOARDLETTER(board, MIDDLEROW, MIDDLEROW);
   /* First generate the matrices based on the board */
   // set up board square masks (for playable tiles)
   if (!(move->flags & MOVE_INCOMPLETE)) {
      best_move.score = 0, best_move.nletters = 0, best_move.leave = -9999, move->flags = MOVE_INCOMPLETE;
      move->current_row = move->current_col = 0;
      move->direction = HORIZONTAL;
      clearmasks(board);      
      setmasks(board, dawg, player->level);
      state_endgame = count_letters(sock->letters) < TILES_PER_MOVE;
   } else {
      if (move->direction == VERTICAL) // put back board
         boardflip(board);   
   }
   move->nletters = 0;
   move->start_ticks = game_time_microsecs();

   while (1) {
      int timedout = 0;
      // must be first move is center square not taken
      if (firstmove) {
         timedout = search_start_first_move(dawg, board, player);
      } else {  
         // returns one when hasn't completed move (taking too long)
         timedout = search_do(dawg, board, player, ticks);
      }
      if (timedout) break;
      
      if (/*!firstmove && */move->direction == HORIZONTAL) {
         boardflip(board);
         move->current_row = move->current_col = 0;
         move->direction = VERTICAL;
         clearmasks(board);      
         setmasks(board, dawg, player->level);
      } else {
         *move = best_move;
         move->flags &= ~MOVE_INCOMPLETE;
         boardflip(board);
         return 0; // finished
      }
   }
   if (move->direction == VERTICAL) // put back board
      boardflip(board);
   return 1;
}

static RACK_T best_rack;
static int search_change(RACK_T *rack, int best_leave)
{
   int i, leave;
   
   leave = leave_eval(rack, state_endgame ? LEAVE_CHANGE|LEAVE_ENDGAME:LEAVE_CHANGE);
   if (leave > best_leave) {
      best_leave = leave;
      best_rack = *rack;
   }
   for (i = 0; i < MAXLETTER; i++) {
      int count = rack->letters[i];
      if (!count) continue;
      rack->letters[i]--;
      best_leave = search_change (rack, best_leave);
      rack->letters[i]++;
   }
   return best_leave;
}

int search_change_start(SOCK_T *sock, PLAYER_T *player)
{
   MOVE_T *move = &player->move;
   RACK_T *rack = &player->rack;
   int leave;
   
   if (player->move.score > 0 && player->level == LEVEL_WORDLENGTH)
      return 0;

   state_endgame = count_letters(sock->letters) < TILES_PER_MOVE;

   leave = search_change(rack, move->leave + 2*move->score);

   if (leave + 2*0 > move->leave + 2*move->score) {

      move->score = 0;
      move->nletters = 0;
      // put back all letters
      rack_unfill(sock, rack);
      // get back best ones
      rack_set(sock, rack, &best_rack);
      return 1;
   }
   return 0;
}

int checkword(DAWG_T *dawg, BOARD_T *board, PLAYER_T *player, SOCK_T *sock)
{
   MOVE_T *move = &player->move;
   int ret, goodmove = 0;
   int oldlevel = player->level;
   player_move = *move;
   player_move.score = 0;
   player_move.leave = 0;

   state_endgame = count_letters(sock->letters) < TILES_PER_MOVE;

   player->level = LEVEL_JUSTCHECK;
   ret = search_start(dawg, board, player, sock, 0);
   player->level = (LEVEL_T)oldlevel;

   if (10 * (2*player_move.score+player_move.leave) >= 7*(2*best_move.score+best_move.leave))
      goodmove = 1; // if 70% of best move, it is considered good

   *move = player_move;
   return move->score > 0 ? goodmove ? 2:1:0;
}
