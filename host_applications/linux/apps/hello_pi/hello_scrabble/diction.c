/*============================================================================
Copyright (c) 2012 Broadcom Europe Ltd. All rights reserved.
Copyright (c) 2001 Alphamosaic. All rights reserved.

Project  :  C6357
Module   :  Scrabble game (scrb)
File     :  $RCSfile: diction.c,v $
Revision :  $Revision: 1.3 $

FILE DESCRIPTION
Dictionary definitions for scrabble game.

MODIFICATION DETAILS
$Log: diction.c,v $
Revision 1.3  2006/06/23 21:03:48  dc4
Removed hardware.h

Revision 1.2  2003/08/20 15:22:08  dc4
Added strupr

Revision 1.1  2003/08/18 10:10:22  dc4
Added number of players, levels, dictionary lookup


=============================================================================*/

/* Project level */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
Functions in other modules accessed by this file.
Specify through module public interface files.
******************************************************************************/

#include "scrabble.h"

static int findword(char *buffer, int len)
{
   int i, start=-1, end=-1;
   for (i=0; i<len; i++) {
      if (buffer[i] == 0) {
         if (start < 0) start = i+1;
         else {end = i; break; }
      }
   }
   if (start < 0 || end < 0) {
      assert(0);
   }
   return start;
}

#ifndef WIN32
static char *strupr(char *string)
{
      char *s;

      if (string)
      {
            for (s = string; *s; ++s)
                  *s = toupper(*s);
      }
      return string;
} 
#endif

char *diction_define(char *word, char *buffer, int buflen)
{
   static char *space;
   char text[BOARDSIZE+1];
   int diff, len, seeklo, seekpt = 0, seekhi;
   int wordlen, red = 0, start=-1, earlier = buflen/3; // we try to catch the earlier definitions
   FILE *fid = fopen("string.fil", "rb");
   if (!fid) return 0;
   
   fseek(fid,0,SEEK_END);
   len=ftell(fid);

   seeklo = 0;
   seekhi = len - buflen;
   
   sprintf(text, " %s ", word);
   strupr(text);
   wordlen = strlen(word) + 1;
   while (1) {
      if (seekpt == (seeklo + seekhi) >> 1) {
         //start = -1;
         //assert(0);
         break;
      }
      start = 0;
      seekpt = (seeklo + seekhi) >> 1;
      fseek(fid,seekpt,SEEK_SET);

      /* Read in buffer of data */
      red = fread(buffer, 1, buflen, fid);
      buffer[buflen-1] = 0;

      start = seekpt ? findword(buffer+earlier, red-earlier):0;
      if (start < 0) {
         assert(0);
         break;      
      } else {
         start += earlier;
      }
      strupr(buffer+start);
      diff = strncmp(text+1, buffer+start, wordlen);
      if (diff == 0) {
         break;
      } else if (diff > 0) { // our word is later in list
         seeklo = seekpt;
      } else { // our word is earlier
         seekhi = seekpt;
      }  
   }   

   fclose(fid);
   if (start < 0) return 0;
   start = seekpt ? findword(buffer, red):0;
   while (1) {
      strupr(buffer+start);
      if ((strncmp(text+1, buffer+start, wordlen) == 0) || strstr(buffer+start, text))
         break;
      // skip definition
      start += strlen(buffer+start)+1;
      if (start >= buflen-1) {
         start = -1;
         break;
      }
   }

   if (start >= 0) {
      space = strchr(buffer+start, '\\');
      if (space) start = space-buffer+2;
   }

   // move to start of buffer
   if (start > 0)
      memmove(buffer, buffer+start, strlen(buffer+start)+1);

   return start >=0 ? buffer:0;
}