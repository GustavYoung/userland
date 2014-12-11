/*============================================================================
Copyright (c) 2012 Broadcom Europe Ltd. All rights reserved.
Copyright (c) 2001 Alphamosaic. All rights reserved.

Project  :  C6357
Module   :  Scrabble game (scrb)
File     :  $RCSfile: leave.c,v $
Revision :  $Revision: 1.4 $

FILE DESCRIPTION
Rack leave for scrabble game.

MODIFICATION DETAILS
$Log: leave.c,v $
Revision 1.4  2006/06/23 21:03:48  dc4
Removed hardware.h

Revision 1.3  2003/08/20 21:44:03  dc4
Fixed leave problem

Revision 1.2  2003/08/13 21:45:51  dc4
Generate blanks on the fly

Revision 1.1  2003/08/04 10:31:37  dc4
Added rack leave, proper human playing

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

typedef enum {false_t, true_t} boolean_t;

 static const boolean_t isvowel[]     = {
     false_t, true_t, false_t, false_t, false_t, true_t, false_t, false_t,     // DUMMY abcdefg
     false_t, true_t, false_t, false_t, false_t, false_t, false_t, true_t,    // hijklmno
     false_t, false_t, false_t, false_t, false_t, true_t, false_t, false_t,    // pqrstuvw
     false_t, false_t, false_t, false_t, false_t, false_t, false_t, false_t     // xyz BLANK
 };
 static const boolean_t isconsonant[] = {
     false_t, false_t, true_t, true_t, true_t, false_t, true_t, true_t,       // DUMMY abcdefg
     true_t, false_t, true_t, true_t, true_t, true_t, true_t, false_t,         // hijklmno
     true_t, true_t, true_t, true_t, true_t, false_t, true_t, true_t,         // pqrstuvw
     true_t, true_t, true_t, false_t, false_t, false_t, false_t, false_t		// xyz BLANK
 };
 static const signed char VCMix[]       = {    /* vowel/consonant mix heuristics */
     0, 0, -1, -2, -3, -4, -5, 0, -1, 1, 1, 0, -1, -2, 0, 0, -2, 0, 2, 2,
     1, 0, 0, 0, -3, -1, 1, 3, 0, 0, 0, 0, -4, -2, 0, 0, 0, 0, 0, 0, -5,
     -3, 0, 0, 0, 0, 0, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
 };
 /* This contains some heuristics for values of letters remaining in rack after move
  Letter  Heuristic1  Heuristic2    Letter  Heuristic1  Heuristic2
     A       +0.5    +1.0    -3.0      B       -3.5    -3.5    -3.0
     C       -0.5    -0.5    -3.5      D       -1.0     0.0    -2.5
     E       +4.0    +4.0    -2.5      F       -3.0    -2.0    -2.0
     G       -3.5    -2.0    -2.5      H       +0.5    +0.5    -3.5
     I       -1.5    -0.5    -4.0      J       -2.5    -3.0
     K       -1.5    -2.5              L       -1.5    -1.0    -2.0
     M       -0.5    -1.0    -2.0      N        0.0    +0.5    -2.5
     O       -2.5    -1.5    -3.5      P       -1.5    -1.5    -2.5
     Q      -11.5   -11.5              R       +1.0    +1.5    -3.5
     S       +7.5    +7.5    -4.0      T       -1.0     0.0    -2.5
     U       -4.5    -3.0    -3.0      V       -6.5    -5.5    -3.5
     W       -4.0    -4.0    -4.5      X       +3.5    +3.5
     Y       -2.5    -2.0    -4.5      Z       +3.0    +2.0
     BLANK  +24.5   +24.5   -15.0
 */
 static const signed char basicRackScore[]     = {
     +49, +2, -7, -1, 0, +8, -4, -4,    // BLANK abcdefg
      +1, -1, -6, -5, -2, -2, +1, -3,    // hijklmno
      -3, -23, +3, +15, 0, -6, -11, -8,   // pqrstuvw
      +7, -4, +4,                      // xyz 
 };
 static const signed char duplicateRackScore[] = {
     -30, -6, -6, -7, -5, -5, -4, -5,    // BLANK abcdefg
      -7, -8, 0, 0, -4, -4, -5, -7,      // hijklmno
      -5, 0, -7, -8, -5, -6, -7, -9,      // pqrstuvw
     0, -9, 0,                           // xyz 
 };
 static const signed char endBasicRackScore[]     = {
     +5, -5, -5, -5,-5, -5, -5, -5,    // BLANK abcdefg
      -5, -5, -5, -5, -5, -5, -5, -5,    // hijklmno
      -5, -5 , -5, +2 ,-5, -5, -5 , -5,    // pqrstuvw
      -5, -5, -5,                        // xyz 
 };
 static const signed char endDuplicateRackScore[] = {
     0, -6, -6, -7, -5, -5, -4, -5,    // BLANK abcdefg
      -7, -8, 0, 0, -4, -4, -5, -7,      // hijklmno
      -5, 0, -7, -8, -5, -6, -7, -9,      // pqrstuvw
      0, -9, 0,                           // xyz 
 };

int leave_eval(RACK_T *rack, int flags) {

   int score      = 0;
   int vowels     = 0;
   int consonants = 0;
   int i;

   assert(!((flags & LEAVE_ENDGAME) && (flags & LEAVE_CHANGE)));

   if (!(flags & LEAVE_ENDGAME)) {
      for (i = 0; i < MAXLETTER; i++) {
         int count = rack->letters[i]; 
         assert(count >=0 && count <=TILES_PER_MOVE);
         if (!count) continue;
         
         vowels     += isvowel[i] ? count : 0;
         consonants += isconsonant[i] ? count : 0;
         score += (flags & LEAVE_CHANGE) ? -2*count:0;
         score += count*(int)basicRackScore[i];
         score += (count-1)*(int)duplicateRackScore[i];
      }
   } else {
      for (i = 0; i < MAXLETTER; i++) {
         int count = rack->letters[i]; 
         assert(count >=0 && count <=TILES_PER_MOVE);
         if (!count) continue;
         
         vowels     += isvowel[i] ? count : 0;
         consonants += isconsonant[i] ? count : 0;
         score += (flags & LEAVE_CHANGE) ? -2*count:0;
         score += count*(int)endBasicRackScore[i];
         score += (count-1)*(int)endDuplicateRackScore[i];
      }
   }
   return score + (int)VCMix[vowels * 8 + consonants];
}

