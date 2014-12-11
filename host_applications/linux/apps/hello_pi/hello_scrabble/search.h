

int search_start(DAWG_T *dawg, BOARD_T *board, PLAYER_T *player, SOCK_T *sock, int ticks);
int search_change_start(SOCK_T *sock, PLAYER_T *player);
int search_square(DAWG_T *dawg, BOARD_T *board, int x, int y);
int checkword(DAWG_T *dawg, BOARD_T *board, PLAYER_T *player, SOCK_T *sock);

