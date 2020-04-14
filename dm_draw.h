#include <ncurses.h>

#ifndef bool
#define false 0
#define true 1
typedef int bool; // or #define bool int
#endif

// Print text on center of std screen
void dm_center(int row, char *title, int colorpair, int colorpair_reset, bool bold);
void dm_center_in_win(WINDOW *win, int row, char *title, int colorpair, int colorpair_reset, bool bold);
void dm_clear_row(int row);
void dm_clear_row_in_win(WINDOW* win, int row);
WINDOW* dm_new_center_win(int row, int width, int height, int offset_x);
#define DM_CALC_CENTER_TOPLEFT(win, rows, cols, top, left) (top = dm_calc_center_top(win, rows), left = dm_calc_center_left(win, cols))
int dm_calc_center_top(WINDOW *win, int rows);
int dm_calc_center_left(WINDOW *win, int cols);
