#include <ncurses.h>

#ifndef bool
#define false 0
#define true 1
typedef int bool; // or #define bool int
#endif

// Print text on center of std screen
void center(int row, char *title, int colorpair, int colorpair_reset, bool bold);
void center_in_win(WINDOW *win, int row, char *title, int colorpair, int colorpair_reset, bool bold);
void clear_row(int row);
WINDOW* new_center_win(int row, int width, int height, int offset_x);
#define CALC_CENTER_TOPLEFT(win, rows, cols, top, left) (top = calc_center_top(win, rows), left = calc_center_left(win, cols))
int calc_center_top(WINDOW *win, int rows);
int calc_center_left(WINDOW *win, int cols);
