#define items(x) (sizeof(x)/sizeof(x[0]))
#define sign(x) ((x)>0?1:(x)<0?-1:0)

// flags to indicate how to interpret an integer
// blank is zero, a-z are 1-26
#define BLANK     0
#define BLANKBIT  (1 << 5)
#define MAXLETTER (1+26)

// other useful constants
#define TILES_PER_MOVE 7
#define BOARDSIZE      15
#define MIDDLEROW      (((BOARDSIZE + 1) / 2)-1)
#define LEN            (BOARDSIZE + 2)
#define MAX_PLAYERS    4
#define MAX_MOVES 50
#define MAX_INFO 256

typedef enum { STATE_INIT, STATE_INPUT, STATE_FIND_MOVE, STATE_FIND_HINT, STATE_REMOVE_TILES, STATE_MOVE_FOUND, STATE_PLACE_TILES, STATE_MAKE_MOVE, STATE_GAMEOVER } GAME_STATE_T;
typedef enum { HORIZONTAL, VERTICAL } DIRECTION_T;
typedef enum { OPTION_NONE, OPTION_PLAY, OPTION_HINT, OPTION_QUIT, OPTION_PASS, OPTION_CHANGE } OPTION_T;
typedef enum { LEVEL_HUMAN, LEVEL_WORDLENGTH, LEVEL_SUBDICTIONARY, LEVEL_HIGHESTSCORE, LEVEL_JUSTCHECK } LEVEL_T;
static const char *level_names[] = {"PLAYER", "DAVE", "JAMES", "IVAN"}; 

typedef struct rack_s 
{
   signed char letters[MAXLETTER];  // how many of each letter in rack
} RACK_T;
typedef struct sock_s 
{
   signed char letters[MAXLETTER];  // how many of each letter in sock (letters bag)
} SOCK_T;
 
typedef struct board_s {
	int mask[LEN][LEN];
	signed char letter[LEN][LEN];
	signed char score[LEN][LEN];
} BOARD_T;


enum {MOVE_INCOMPLETE=1, MOVE_HINTED=2};

typedef struct move_s
{
   int x, y;
   int score;
   int leave;
   int nletters;
   int start_ticks;
   int current_row, current_col;
   int flags;
   char letter[BOARDSIZE];
   DIRECTION_T direction;   
} MOVE_T;

typedef struct info_s
{
   char string[MAX_INFO]; // null terminated string
   int width;
   int x,y,z;  /* Current location */
   int tx,ty,tz; /* Target location */
} INFO_T;


#define MAX_MENU_ITEMS 10
#define MAX_MENU_LEN 10
typedef struct menu_s
{
   char string[MAX_MENU_ITEMS][MAX_MENU_LEN]; // null terminated string
   int type;
   int numitems;
   int targetitem; 
   int curitem;     // current item << 8
   int maxlen;
   int x,y,z;  /* Current location */
   int updated;
} MENU_T;

typedef struct gui_letter_s
{
   int letter; /* 1 to 26 plus BLANKBIT for a blank */
   int x,y,z;  /* Current location */
   int tx,ty,tz; /* Target location */
   int tilex,tiley; /* Tile location */
   int onrack; /* 0 if on board, 1 if on rack */
} GUI_LETTER_T;

typedef enum {BOARD_TYPE_GAME, BOARD_TYPE_OPTIONS} BOARD_SHOWN_T;

#define TARGET_SCALE 8
typedef struct gui_s {
   int tilex;    /* Location of the current tile under the cursor */
   int tiley;
   int viewangle;   /* Angle of view */
   int distance; /* Height of view above board */
   int viewx;    /* Location that view is currently aimed towards */
   int viewy;    /* scaled by TARGET_SCALE */
   int targetx;  /* Target values for viewx,y */
   int targety;  /* scaled by TARGET_SCALE */
   int speed;
   int bounceh;  /* Bouncing tile height */
   int bouncevh; /* Bounce velocity */
   int rotated;  /* whether screen is rotated */
   int landscape;  /* whether screen is landscape */
   BOARD_SHOWN_T board;
   int rack_scrolling; /* if we are scrollwheeling through rack */
   int rack_removing; /* if we are removing from rack */
} GUI_T; /* Structure containing gui details */

typedef struct gui_rack_s
{
   int numletters;
   GUI_LETTER_T letters[TILES_PER_MOVE];
} GUI_RACK_T; /* Structure containing gui details */


typedef struct player_t
{
   int score;
   int nummoves;
   MOVE_T moves[MAX_MOVES];
   MOVE_T move;
   RACK_T rack;
   LEVEL_T level;
   GUI_RACK_T guirack;
} PLAYER_T;


typedef struct dawg_s 
{
   int len; /* Length of file in bytes */
   int nedges; /* Number of edges */
   int *data;
   int skip;
   int root;
} DAWG_T;


enum {SCRABBLE_SOUNDS=1<<0, SCRABBLE_MUSIC=1<<1, SCRABBLE_MENU=1<<2 };
typedef struct scrabble_s
{
   BOARD_T board;
   BOARD_T optionsboard;
   SOCK_T sock;
   INFO_T info;
   MENU_T menu;
   PLAYER_T player[MAX_PLAYERS];
   int current_player;
   int num_players;
   int passes, changes;
   GAME_STATE_T state;
   GUI_T gui;
   int flags;
   DAWG_T *dawg;
   void *graphics;
} SCRABBLE_T;

#include "dawg.h"
#include "board.h"
#include "search.h"
#include "leave.h"


/** Score for each letter **/
// # a b c d e    f g h i j       k l m n o p  q r s t u v w x y  z
static const char letter_value[] =
{
    0, 1, 3, 3, 2, 1, 4, 2, 4, 1, 8, 5, 1, 3, 1, 1, 3, 10, 1, 1, 1, 1, 4,
    4, 8, 4, 10, 0,	0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,	0, 0, 0, 0,
};

// # a b c d  e f g h i j k l m n o p  q r s t u v w x y  z */
static const char letter_quantity[] =
{
    2, 1*9, 2, 2, 4, 1*12, 2, 3, 2, 1*9, 1, 1, 4, 2, 6, 1*8, 2, 1, 6, 4, 6, 4, 2,
    2, 1, 2, 1, 0, 0 , 0 , 0 ,0
};

static const char letter_name[] = {
    ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '*',
	'*', '*', '*', '*',

    // Same letters, with BLANKBIT set
	' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '*',
    '*', '*', '*', '*'
};

// we pack both the word (second nibble) and letter (first nibble) multipliers into this table
// also due to symmetry, just store the top left quarter
static const char wordLetterMult[(BOARDSIZE+1)/2][(BOARDSIZE+1)/2]   = {
   {0x31,0x11,0x11,0x12,0x11,0x11,0x11,0x31,},
   {0x11,0x21,0x11,0x11,0x11,0x13,0x11,0x11,},
   {0x11,0x11,0x21,0x11,0x11,0x11,0x12,0x11,},
   {0x12,0x11,0x11,0x21,0x11,0x11,0x11,0x12,},
   {0x11,0x11,0x11,0x11,0x21,0x11,0x11,0x11,},
   {0x11,0x13,0x11,0x11,0x11,0x13,0x11,0x11,},
   {0x11,0x11,0x12,0x11,0x11,0x11,0x12,0x11,},
   {0x31,0x11,0x11,0x12,0x11,0x11,0x11,0x21,},
};

#define INLINE 

static INLINE int get_letter_mult(int x, int y) {
	assert(x>=0 && x<BOARDSIZE);
	assert(y>=0 && y<BOARDSIZE);
   if (x+1 > (BOARDSIZE+1)/2) x = BOARDSIZE-1-x;
   if (y+1 > (BOARDSIZE+1)/2) y = BOARDSIZE-1-y;
	return (int)((wordLetterMult[y][x]>>0)&0xf);
}

static INLINE int get_word_mult(int x, int y) {
	assert(x>=0 && x<BOARDSIZE);
	assert(y>=0 && y<BOARDSIZE);
   if (x+1 > (BOARDSIZE+1)/2) x = BOARDSIZE-1-x;
   if (y+1 > (BOARDSIZE+1)/2) y = BOARDSIZE-1-y;
	return (int)((wordLetterMult[y][x]>>4)&0xf);
}

static INLINE char get_letter_value(int index)
{
   assert((index&~BLANKBIT) >=0 && (index&~BLANKBIT) < MAXLETTER);
   return letter_value[index];
}

static INLINE char get_letter_name(int index)
{
   assert((index&~BLANKBIT) >=0 && (index&~BLANKBIT) < MAXLETTER);
   return letter_name[index];
}

static INLINE char get_letter_index(char c)
{
   int index;
   for (index = 0; index < sizeof(letter_name); index++)
      if (letter_name[index] == c)
         return index;
   assert(0);
   return 0;
}

static INLINE char get_letter_tile(char c)
{
   int index;
   for (index = 0; index < sizeof(letter_name); index++)
      if (letter_name[index] == c)
         return index >= 32 ? BLANK:index;
   assert(0);
   return 0;
}


char *stringtoindices(const char *word);
char *indicestostring(const char *word);
char *diction_define(char *word, char *buffer, int buflen);
void scrabble_logic_update(SCRABBLE_T *scrabble);
void scrabble_init(SCRABBLE_T *scrabble);
