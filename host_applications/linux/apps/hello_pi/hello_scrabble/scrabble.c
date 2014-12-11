#define QUICKSAVEFILE "scrabble.sav"

/*============================================================================
Copyright (c) 2012 Broadcom Europe Ltd. All rights reserved.
Copyright (c) 2001 Alphamosaic. All rights reserved.

Project  :  C6357
Module   :  Scrabble game (scrb)
File     :  $RCSfile: scrabble.c,v $
Revision :  $Revision: 1.72 $

FILE DESCRIPTION
Main program for scrabble game.

=============================================================================*/


/* Project level */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/******************************************************************************
Functions in other modules accessed by this file.
Specify through module public interface files.
******************************************************************************/

//#include "vcinclude/common.h"
//#include "filesys/filesys.h"
#ifdef _VIDEOCORE
#include "vclib/vclib.h"
#include "game.h"
#include "audiopool.h"
#else
typedef void * AUDIOPOOL_HANDLE_T;
typedef enum {GAME_KEY_ANY, GAME_KEY_ACTION, GAME_KEY_A, GAME_KEY_B, GAME_KEY_C, GAME_KEY_D, GAME_KEY_LEFT, GAME_KEY_RIGHT, GAME_KEY_UP, GAME_KEY_DOWN, GAME_KEY_SCROLL_C, GAME_KEY_SCROLL_A } GAME_KEY_T;
#endif

#include "scrabble.h"

/******************************************************************************
Public functions contained in this file.
******************************************************************************/

void info_set(INFO_T *info, char *string, int width, int x, int y, int z, int tx, int ty, int tz);
static void numbertostring(char *string, int number);
static int gui_findtile(GUI_RACK_T *guirack, int x, int y);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

#ifdef FORSIM
#define SCROLLSPEED (32*16)
#define INFOSPEED (32*16)
#define MENUSPEED (8*2)
#else
#define SCROLLSPEED 24
#define INFOSPEED 24
#define MENUSPEED (8)
#endif

#define BOUNCERESTITUTION 0.9375
#define BOUNCEMIN 0 //-10
#define BOUNCEMAX 64 //100 //130
#define BOUNCEGRAVITY 8

#define NUMVERTBUFFER 64
static const char *menu_strings[] = {
"START", "SCORE", "GAME", "AUTO", "QUIT", "HINT", "CHANGE", "PASS", "VIEW", "ZOOM", "MUSIC", "SOUNDS", "OPTIONS", "PLAYERS", "ADD", "REMOVE", "EDIT", "PLAYER0", "PLAYEY1", "PLAYER2", "PLAYER3", 
};
enum {MENU_START, MENU_SCORE, MENU_GAME, MENU_AUTO, MENU_QUIT, MENU_HINT, MENU_CHANGE, MENU_PASS, MENU_VIEW, MENU_ZOOM, MENU_MUSIC, MENU_SOUNDS, MENU_OPTIONS, MENU_PLAYERS, MENU_ADD, MENU_REMOVE, MENU_EDIT, MENU_PLAYER0, MENU_PLAYER1, MENU_PLAYER2, MENU_PLAYER3, };
enum {MENUS_INIT, MENUS_START, MENUS_GAME, MENUS_SCORE, MENUS_OPTIONS, MENUS_PLAYERS, };
unsigned menu_menus[] = {
   (1<<MENU_START)|(1<<MENU_OPTIONS)|(1<<MENU_PLAYERS)|(1<<MENU_AUTO),
   (1<<MENU_START)|(1<<MENU_OPTIONS)|(1<<MENU_PLAYERS)|(1<<MENU_AUTO)|(1<<MENU_SCORE),
   (1<<MENU_QUIT)|(1<<MENU_OPTIONS)|(1<<MENU_SCORE)|(1<<MENU_CHANGE)|(1<<MENU_PASS)|(1<<MENU_HINT),
   (1<<MENU_GAME)|(1<<MENU_OPTIONS),
   (1<<MENU_MUSIC)|(1<<MENU_SOUNDS)|(1<<MENU_VIEW)|(1<<MENU_ZOOM),
   (1<<MENU_ADD)|(1<<MENU_REMOVE)|(1<<MENU_EDIT),
};

enum {SOUND_MIDI, SOUND_APPLAUSE, SOUND_PLACTILE, SOUND_PICKTILE, SOUND_FOGHORN, SOUND_SHUFFLE, SOUND_TRUMPET, SOUND_WAHWAH, SOUND_CHEERING, SOUND_EXCELLENT };
static const char *sound_files[] = {"scrabble.mid", "applause.adp", "plactile.adp", "picktile.adp", "foghorn.adp", "shuffle.adp", "trumpet.adp", "wahwah.adp", "cheering.adp", "excel.adp" };
static AUDIOPOOL_HANDLE_T *sound_handles[items(sound_files)];

static int automove=1;

/******************************************************************************
Private functions in this file.
Declare as static.
******************************************************************************/


static void sample_play(SCRABBLE_T *scrabble, int sound)
{
#ifdef _VIDEOCORE_SOUND
   if (scrabble->flags & SCRABBLE_SOUNDS)
      audiopool_play(sound_handles[sound],AUDIOPOOL_AUTOALLOC,65535,0,0,1,NULL,0,0);
#endif
}

static void midi_play(SCRABBLE_T *scrabble, int sound)
{
#ifdef _VIDEOCORE_SOUND
   if (scrabble->flags & SCRABBLE_MUSIC) {
      int channel = audiopool_play(sound_handles[sound],AUDIOPOOL_AUTOALLOC,65535>>2,0,0,0,NULL,0,0);
      assert(channel==0);
   }
   else
      audiopool_stop(0);
#endif
}



static void info_init(INFO_T *info)
{
   memset(info, 0, sizeof(INFO_T));
}

static void move_init(MOVE_T *move)
{
   memset(move, 0, sizeof(MOVE_T));
}



void gui_maketarget(GUI_T *gui)
{
   int moved = gui->targetx != ((gui->tilex-MIDDLEROW)<<TARGET_SCALE) || gui->targety != ((gui->tiley-MIDDLEROW)<<TARGET_SCALE);
   gui->targetx = (gui->tilex-MIDDLEROW)<<TARGET_SCALE;
   gui->targety = (gui->tiley-MIDDLEROW)<<TARGET_SCALE; 
 
}

void gui_init(SCRABBLE_T *scrabble, GUI_T *gui)
{
   gui->tilex=(BOARDSIZE)>>1;
   gui->tiley=(BOARDSIZE)>>1;
   gui->distance=3000;
   gui->viewangle=(1<<12);
   gui_maketarget(gui);
   gui->viewx=gui->targetx;
   gui->viewy=gui->targety;
   gui->speed = 0;
   gui->board = BOARD_TYPE_GAME;
   gui->rotated = 0;
}



static void stringtoboard(BOARD_T *board, char *string, int x, int y)
{
   char *word, *end, *space;  
   word = string;
   end = word + strlen(string);
   while (word < end) {
      space = strchr(word, ' ');
      if (!space) space = end;
      else *space = 0;                  
      if (x+space-word > BOARDSIZE)
         x = 0, y++;
      if (y < BOARDSIZE)
         x += boardtext(board, word, x, y)+1;
      word = space+1;
   }
}

/*
static void player_init(PLAYER_T *player, LEVEL_T level)
{
   memset(player, 0, sizeof(PLAYER_T));
   rack_init(&player->rack);
   move_init(&player->move);
   gui_rack_init(&player->guirack);
   player->level = level;
}



static void scrabble_reset(SCRABBLE_T *scrabble)
{
   int i;
   //automove = 0;
   info_init(&scrabble->info);
   scrabble->passes = scrabble->changes = 0;
   scrabble->current_player = scrabble->num_players ? rand() % scrabble->num_players : 0;
   sprintf(scrabble->info.string, "%s STARTS", level_names[scrabble->player[scrabble->current_player].level]);
   info_set(&scrabble->info, scrabble->info.string, 256, 8, 5, -1, -12, 5, -1);
   gui_init(scrabble, &scrabble->gui);
   for (i=0; i < MAX_PLAYERS; i++) {
      player_init(&scrabble->player[i], scrabble->player[i].level);
   }
   remove_menus(scrabble);
   boardreset(&scrabble->board);
   boardreset(&scrabble->optionsboard);
   i = sock_fill(&scrabble->sock);
   //assert(i==100);
} 
*/

static OPTION_T gui_usebuttons(SCRABBLE_T *scrabble, PLAYER_T *player, DAWG_T *dawg, GAME_STATE_T state)
{
   
   return OPTION_NONE;
}



void *scrabble_graphics_init(SCRABBLE_T *scrabble);
void scrabble_graphics_update(SCRABBLE_T *scrabble, void *vgraphics);

void scrabble_logic_update(SCRABBLE_T *scrabble)
{

}

void scrabble_init(SCRABBLE_T *scrabble)
{
   int i;

}
