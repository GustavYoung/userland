void boardreset(BOARD_T *board);
int boardword(BOARD_T *board, RACK_T *rack, MOVE_T *move, int letterstoplace);
char *board_getword(BOARD_T *board, char *word, int x, int y, DIRECTION_T direction);
int boardtext(BOARD_T *board, char *string, int x, int y);
void boardflip(BOARD_T *board);
int rack_init(RACK_T *rack);
int rack_score(RACK_T *rack);
int rack_empty(RACK_T *rack, MOVE_T *move);
int rack_fill(SOCK_T *sock, RACK_T *rack);
int rack_unfill(SOCK_T *sock, RACK_T *rack);
int rack_set(SOCK_T *sock, RACK_T *rack, RACK_T *desired);
int rack_remove_letter(RACK_T *rack, int index);
int sock_fill(SOCK_T *sock);
int get_letter(signed char *letters, int pick);
int count_letters(signed char *letters);
char *checkmove(BOARD_T *board, GUI_RACK_T *guirack, MOVE_T *move);

#define INLINE

static INLINE char GETBOARDLETTER(BOARD_T *board,int x,int y) {
	assert(x+1>=0 && x+1<LEN);
	assert(y+1>=0 && y+1<LEN);
	return board->letter[y+1][x+1];
}
static INLINE void SETBOARDLETTER(BOARD_T *board,int x,int y,int letter) {
	assert(x+1>=0 && x+1<LEN);
	assert(y+1>=0 && y+1<LEN);
	board->letter[y+1][x+1]=letter;
}
static INLINE int GETBOARDMASK(BOARD_T *board,int x,int y) {
	assert(x+1>=0 && x+1<LEN);
	assert(y+1>=0 && y+1<LEN);
	return board->mask[y+1][x+1];
}
static INLINE void SETBOARDMASK(BOARD_T *board,int x,int y,int mask) {
	assert(x+1>=0 && x+1<LEN);
	assert(y+1>=0 && y+1<LEN);
	board->mask[y+1][x+1]=mask;
}
static INLINE int GETBOARDSCORE(BOARD_T *board,int x,int y) {
	assert(x+1>=0 && x+1<LEN);
	assert(y+1>=0 && y+1<LEN);
	return board->score[y+1][x+1];
}
static INLINE void SETBOARDSCORE(BOARD_T *board,int x,int y,int score) {
	assert(x+1>=0 && x+1<LEN);
	assert(y+1>=0 && y+1<LEN);
	board->score[y+1][x+1]=score;
}
